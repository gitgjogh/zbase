/*****************************************************************************
 * Copyright 2014 Jeff <ggjogh@gmail.com>
 *****************************************************************************
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*****************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "zhtree.h"
#include "sim_log.h"

static uint32_t zht_nstr_time33(uint32_t type, const char *key, uint32_t key_len);
static uint32_t zht_cstr_time33(uint32_t type, const char *key, uint32_t *key_len);
static zaddr_t zhtree_add_to_children(zhtree_t *h, zht_node_t *parent, zht_node_t *node);

zhtree_t *zhtree_malloc(uint32_t node_size, uint32_t depth_log2)
{
    zhtree_t *zh = calloc( 1, sizeof(zhtree_t) );
    if (!zh) {
        xerr("<zhtree> obj malloc failed\n");
        return 0;
    }

    zh->depth_log2 = depth_log2;
    zh->depth = 1 << depth_log2;

    zh->hash_tbl = calloc(zh->depth, sizeof(zht_head_t));
    zh->nodeq = zqueue_malloc(node_size, zh->depth);
    zqueue_memzero(zh->nodeq);
    zh->strq  = zstrq_malloc(0);

    if (!zh->hash_tbl || !zh->nodeq || !zh->strq) {
        xerr("<zhtree> buf malloc failed!\n");
        zhtree_free(zh);
        return 0;
    }

    static char name[] = "root";
    zht_node_t *root = zqueue_push_back(zh->nodeq, 0);
    if ( root == 0 ) {
        xerr("<zhtree> no room for root!\n");
        zhtree_free(zh);
    }
    memset(root, 0, sizeof(zht_node_t));
    root->key = name;
    zh->root = root;
    zh->wdir = root;

    return zh;
}

void zhtree_free(zhtree_t *h)
{
    if (h) {
        if (h->hash_tbl) { free(h->hash_tbl); }
        if (h->nodeq) { zqueue_free(h->nodeq); }
        if (h->strq) { zstrq_free(h->strq); }
        free(h);
    }
}

static 
uint32_t zht_nstr_time33(uint32_t type, const char *key, uint32_t key_len)
{
    uint32_t i = 0;
    uint32_t hash = type * 33 + '/';
    for (i=0; i<key_len; ++i) {
        hash = hash * 33 + key[i]; 
    } 

    return hash; 
}

static 
uint32_t zht_cstr_time33(uint32_t type, const char *key, uint32_t *key_len)
{
    int      c = 0;
    uint32_t i = 0;
    uint32_t hash = type * 33 + '/';
    for (i=0; c=key[i]; ++i) {
        hash = hash * 33 + c; 
    } 

    if (key_len) { 
        *key_len = i;
    }

    return hash; 
}

zaddr_t zhtree_get_root(zhtree_t *h)
{
    return h->root;
}

int zhtree_ret_flag(zhtree_t *h)
{
    return h->ret_flag;
}

int zhtree_is_node_in_buf(zhtree_t *h, zaddr_t node_base)
{
    return zqueue_is_elem_base_in_buf(h->nodeq, node_base);
}

int zhtree_is_node_in_use(zhtree_t *h, zaddr_t node_base)
{
    return zqueue_is_elem_base_in_use(h->nodeq, node_base);
}

zht_child_iter_t   zht_child_iter_init(zht_node_t *parent)
{
    zht_child_iter_t iter = {
        parent, 
        parent ? parent->child : 0, 
        parent ? parent->child : 0
    };
    return iter;
}
zaddr_t zht_child_iter_1st(zht_child_iter_t *iter)
{
    if (iter->parent) {
        return iter->curr = iter->direct = iter->parent->child;
    }
    return 0;
}
zaddr_t zht_child_iter_next(zht_child_iter_t *iter)
{
    if (iter->curr) {
        iter->curr = iter->curr->right;
        return (iter->curr == iter->direct) ? 0 : iter->curr;
    }
    return 0;
}


int zhtree_is_one_child(zhtree_t *h, zht_node_t *parent, zht_node_t *node)
{
    zht_child_iter_t iter = zht_child_iter_init(parent);
    FOR_ZHT_CHILD_IN(iter) {
        if (ZHT_CHILD_GET_CURR(iter) == node) {
            return 1;
        }
    }
    return 0;
}

static
zht_node_t *zhtree_find_in_children(zhtree_t *h, zht_node_t *parent, 
                            zh_hval_t hash, const char *key, uint32_t key_len)
{
    zht_node_t *node = 0;
    zht_child_iter_t iter = zht_child_iter_init(parent);
    WHILE_GET_ZHT_CHILD(iter, node) 
    {
        if (node->hash==hash && node->parent==parent &&
                node->key[key_len]==0 && strncmp(node->key, key, key_len)==0) 
        {
            return node;
        }
    }
    return 0;
}

static
zht_node_t *zhtree_find_in_collision(zhtree_t *h, zht_node_t *parent, 
                            zh_hval_t hash, const char *key, uint32_t key_len)
{
    zht_node_t *head = h->hash_tbl[GETLSBS(hash, h->depth_log2)];
    zht_node_t *node = 0;
    zh_link_iter_t iter = zh_link_iter_init(head);
    WHILE_GET_COLLISION_NODE(iter, node) 
    {
        if (node->hash==hash && node->parent==parent &&
                node->key[key_len]==0 && strncmp(node->key, key, key_len)==0) 
        {
            return node;
        }
    }
    return 0;
}

static
zaddr_t zhtree_touch_node(zhtree_t *h, zht_node_t *parent,
                        const char  *key, uint32_t key_len, 
                        int b_insert)
{
    uint32_t hash = key_len ? zht_nstr_time33(parent->hash, key, key_len)
                            : zht_cstr_time33(parent->hash, key, &key_len);

    zht_node_t *node = zhtree_find_in_collision(h, parent, hash, key, key_len);
    if (node) {
        h->ret_flag |= ZHASH_FOUND;
        return node;
    } 

    if (b_insert) 
    {
        zsq_char_t *saved_key = zstrq_push_back(h->strq, key, key_len);
        if ( saved_key == 0 ) {
            xerr("<zhtree> key buf overflow!\n");
            h->ret_flag |= ZHASH_KEY_BUF_OVERFLOW;
            return 0;
        }

        node = zqueue_push_back(h->nodeq, 0);
        if ( node == 0 ) {
            xerr("<zhtree> hash table overflow!\n");
            h->ret_flag |= ZHASH_NODE_BUF_OVERFLOW;
            return 0;
        } 
        node->hash = hash;
        node->key  = saved_key;
        
        /* insert to hash collision link */
        zht_node_t *head = h->hash_tbl[GETLSBS(hash, h->depth_log2)];
        node->next = head ? head->next : 0;
        h->hash_tbl[GETLSBS(hash, h->depth_log2)] = node;

        /* insert to children link */
        zhtree_add_to_children(h, parent, node);
        return node;
    }

    return 0;
}

static
zaddr_t zhtree_add_to_children(zhtree_t *h, zht_node_t *parent, zht_node_t *node)
{
    assert(zhtree_is_node_in_use(h, parent));
    assert(zhtree_is_node_in_use(h, node));

    zht_node_t* child = parent->child;
    if (child) {
        /* child <--> node <--> right */
        zht_node_t* right = child->right;
        node->left = child;
        node->right = right;
        right->left = node;
        child->right = node;
    } else {
        node->left = node->right = node;
    }
    parent->child = node;
    node->parent = parent;
    node->layer = parent->layer + 1;
    node->child = 0;

    return node;
}

zaddr_t zhtree_get_node(zhtree_t *h, zht_node_t *father, 
                        const char *key, uint32_t keylen)
{
    return zhtree_touch_node(h, father, key, keylen, 0);
}

zaddr_t zhtree_set_node(zhtree_t *h, zht_node_t *father, 
                        const char *key, uint32_t keylen)
{
    return zhtree_touch_node(h, father, key, keylen, 1);
}

static
zaddr_t zhtree_touch_path(zhtree_t *h, const char *path, uint32_t path_len, int b_insert)
{
    if (path==0 || path_len==0) {
        xerr("<zhtree> null path\n");
        return 0;
    }

    int nkey=0, keylen=0, pos=0;
    
    zht_node_t *parent = 0;
    zht_node_t *child = 0;
    if (path[0]=='/') {
        parent = zhtree_get_root(h);
        pos += 1;
    } else {
        parent = zhtree_get_wdir(h);
    }
    
    char *substr = 0;
    str_iter_t iter = str_iter_init(path, 0);
    WHILE_GET_FIELD(iter, "", "/", substr) 
    {
        int sublen = STR_ITER_GET_SUBLEN(iter);
        if (sublen == 1 && substr[0] == '.') {
            continue;
        } else if (sublen == 2 && substr[0] == '.' && substr[1] == '.') {
            child = child->parent;
        } else {
            child = zhtree_touch_node(h, parent, substr, sublen, b_insert);
        }
        
        if (child) {
            parent = child;
        } else {
            break;
        }
    }
    
    return child;
}

zaddr_t zhtree_get_path(zhtree_t *h, const char *path, uint32_t path_len)
{
    return zhtree_touch_path(h, path, path_len, 0);
}

zaddr_t zhtree_set_path(zhtree_t *h, const char *path, uint32_t path_len)
{
    return zhtree_touch_path(h, path, path_len, 1);
}

