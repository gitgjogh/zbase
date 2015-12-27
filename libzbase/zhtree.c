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

static zht_node_t *zhtree_mount_new_child(zhtree_t *h, zht_node_t *parent, zht_node_t *node);
static zht_node_t *zhtree_find_in_children(zhtree_t *h, zht_node_t *parent, 
                            zh_hval_t hash, const char *key, uint32_t key_len);
static zht_node_t *zhtree_find_in_collision(zhtree_t *h, zht_node_t *parent, 
                            zh_hval_t hash, const char *key, uint32_t key_len);


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
    zh->nodeq = zqueue_malloc_s(node_size, zh->depth);
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
    zh->wnode = root;

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

zaddr_t zhtree_get_wnode(zhtree_t *h)
{
    return h->wnode;
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
        parent ? parent->child : 0
    };
    return iter;
}
zaddr_t zht_child_iter_1st(zht_child_iter_t *iter)
{
    if (iter->parent) {
        return iter->curr = iter->parent->child;
    }
    return (iter->curr = 0);
}
zaddr_t zht_child_iter_next(zht_child_iter_t *iter)
{
    if (iter->parent && iter->curr) {
        iter->curr = iter->curr->right;
        if (iter->curr != iter->parent->child) {
            return iter->curr;
        }
    }
    return (iter->curr = 0);
}
zaddr_t zht_child_iter_last(zht_child_iter_t *iter)
{
    if (iter->parent) {
        return iter->curr = iter->parent->child->left;
    }
    return (iter->curr = 0);
}
zaddr_t zht_child_iter_prev(zht_child_iter_t *iter)
{
    if (iter->parent && iter->curr) {
        if (iter->curr != iter->parent->child) {
            iter->curr = iter->curr->left;
            return iter->curr;
        }
    }
    return (iter->curr = 0);
}
zaddr_t zht_child_iter_curr(zht_child_iter_t *iter)
{
    return iter->curr;
}

int zhtree_is_one_child(zhtree_t *h, zht_node_t *parent, zht_node_t *node)
{
    zht_child_iter_t iter = zht_child_iter_init(parent);
    FOR_ZHT_CHILD_IN(iter) {
        if (zht_child_iter_curr(&iter) == node) {
            return 1;
        }
    }
    return 0;
}

/** 
 *  Similar to @func zhtree_is_one_child(). Currently useless because 
 *  hash is thought to be more efficient. 
 *  Just to show the diffrence between children link and collision link.
 */
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
    zh_link_iter_t iter = zh_link_iter_init((zh_node_t *)head);
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
zaddr_t zhtree_touch_child_internal(zhtree_t *h, zht_node_t *parent,
                        const char  *key, uint32_t key_len, 
                        int b_insert)
{
    uint32_t hash = key_len ? zht_nstr_time33(parent->hash, key, key_len)
                            : zht_cstr_time33(parent->hash, key, &key_len);

    /* we don't use @func zhtree_find_in_children() 
       because hash is thought to be faster */
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
        zhtree_mount_new_child(h, parent, node);
        return node;
    }

    return 0;
}

/**
 * mount new created node to parent
 */
static
zht_node_t *zhtree_mount_new_child(zhtree_t *h, zht_node_t *parent, zht_node_t *node)
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

zaddr_t zhtree_get_child(zhtree_t *h, zht_node_t *parent, 
                        const char *key, uint32_t keylen)
{
    assert(zhtree_is_node_in_use(h, parent));
    return zhtree_touch_child_internal(h, parent, key, keylen, 0);
}

zaddr_t zhtree_touch_child(zhtree_t *h, zht_node_t *parent, 
                        const char *key, uint32_t keylen)
{
    assert(zhtree_is_node_in_use(h, parent));
    return zhtree_touch_child_internal(h, parent, key, keylen, 1);
}

static
zaddr_t zhtree_touch_node_internal(zhtree_t *h, const char *path, 
                                   uint32_t path_len, int b_insert)
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
        parent = zhtree_get_wnode(h);
    }
    
    char *substr = 0;
    str_iter_t iter = str_iter_init(path, path_len);
    WHILE_GET_FIELD(iter, "", "/", substr) 
    {
        int sublen = STR_ITER_GET_SUBLEN(iter);
        if (sublen == 1 && substr[0] == '.') {
            continue;
        } else if (sublen == 2 && substr[0] == '.' && substr[1] == '.') {
            child = child->parent;
        } else {
            child = zhtree_touch_child_internal(h, parent, substr, sublen, b_insert);
        }
        
        if (child) {
            parent = child;
        } else {
            break;
        }
    }
    
    return child;
}

zaddr_t zhtree_get_node(zhtree_t *h, const char *path, uint32_t path_len)
{
    return zhtree_touch_node_internal(h, path, path_len, 0);
}

zaddr_t zhtree_touch_node(zhtree_t *h, const char *path, uint32_t path_len)
{
    return zhtree_touch_node_internal(h, path, path_len, 1);
}

zaddr_t zhtree_change_wnode(zhtree_t *h, const char *path, uint32_t path_len)
{
    zaddr_t node = zhtree_get_node(h, path, path_len);
    if (node) {
        h->wnode = node;
        return h->wnode;
    } else {
        return 0;
    }
}

zqueue_t *zhtree_create_route(zhtree_t *h, zht_node_t* node)
{
    if (!node || zhtree_is_node_in_use(h, node)) {
        return 0;
    }
    
    #define INIT_ROUTE_DEPTH 64
    
    zqueue_t * q = ZQUEUE_MALLOC_D(zht_node_t*, INIT_ROUTE_DEPTH);
    if (!q) {
        xerr("<zhtree> %s() failed:1!\n", __FUNCTION__);
        return 0;
    }

    while (node) {
        zaddr_t ret = zqueue_push_back(q, &node);
        if (!ret) {
            xerr("<zhtree> %s() failed:2!\n", __FUNCTION__);
            zqueue_free(q);
            return 0;
        }
        if (node != zhtree_get_root(h)) {
            node = node->parent;
        } else {
            break;
        }
    }
    
    return q;
}



/**
 * @return The space (not counting the null end character) need for storing
 *         the entire nodeq.
 */
int zhtree_snprint_route(zqueue_t *nodeq, char *str, int size)
{
    zcount_t last = zqueue_get_count(nodeq) - 1;
    int left = size, need = 0;
    for (; last>=0; --last)
    {
        zaddr_t base = zqueue_get_elem_base(nodeq, last - 1);
        zht_node_t *node = *(zht_node_t**)base;
        int keylen = strlen(node->key);
        if (need < size) {
            if (left > keylen) {
                strcpy(str, node->key);
                str  += keylen;
                left -= keylen;
            } else {
                strncpy(str, node->key, left - 1);
                str  += (left - 1);
                left  = 1;           /* for null terminate */
            }
        }
        need += keylen;
    }
    str[0] = 0;

    return need;
}

zht_path_t zht_path_open(zhtree_t *h, zht_node_t *node)
{
    zht_path_t iter = {h, 0, 0 ,0};
    if (!h || !node || !zhtree_is_node_in_use(h, node)) {
        return iter;
    }

    #define INIT_ROUTE_DEPTH 64
    iter.nodeq = ZQUEUE_MALLOC_D(zht_node_t*, INIT_ROUTE_DEPTH);
    if (!iter.nodeq) {
        xerr("<zhtree> %s() failed:1!\n", __FUNCTION__);
        return iter;
    }

    while (node) {
        zaddr_t ret = zqueue_push_back(iter.nodeq, &node);
        if (!ret) {
            xerr("<zhtree> %s() failed:2!\n", __FUNCTION__);
            zqueue_free(iter.nodeq);
            return iter;
        }
        if (node != zhtree_get_root(h)) {
            node = node->parent;
        } else {
            break;
        }
    }

    iter.h = h;
    iter.last = node;

    return iter;
}

int zht_path_assert(zht_path_t *iter)
{
    if (iter && iter->nodeq) {
        return 1;
    }
    return 0;
}

void zht_path_close(zht_path_t *iter)
{
    if (iter && iter->nodeq) {
        zqueue_free(iter->nodeq);
    }
    iter->nodeq = 0;
}

zaddr_t zht_path_iter_root(zht_path_t *iter)
{
    if (iter->nodeq) {
        iter->iter_idx = zqueue_get_count(iter->nodeq);
    }
    return zht_path_iter_next(iter);
}

zaddr_t zht_path_iter_next(zht_path_t *iter)
{
    if (iter->nodeq) {
        zaddr_t base = zqueue_get_elem_base(iter->nodeq, --iter->iter_idx);
        return base ? *(zht_node_t**)base : 0;
    }
    return 0;
}

zaddr_t zht_path_iter_last(zht_path_t *iter)
{
    iter->iter_idx = -1;
    return zht_path_iter_prev(iter);
}

zaddr_t zht_path_iter_prev(zht_path_t *iter)
{
    if (iter->nodeq) {
        zaddr_t base = zqueue_get_elem_base(iter->nodeq, ++iter->iter_idx);
        return base ? *(zht_node_t**)base : 0;
    }
    return 0;
}

void zht_path_print(zht_path_t *iter)
{
    if (!iter || !iter->nodeq) {
        return;
    }
    
    zht_node_t *node = 0;
    WHILE_ZHT_PATH_DOWN(iter, node)
    {
        xprint("%s", node->key);
        if (node!=iter->last) {
            xprint(".");
        }
    }
}

int  zht_path_snprint(zht_path_t *iter, char *str, int size)
{
    if (!iter || !iter->nodeq) {
        if (str && size) {
            str[0]=0;
        }
        return 0;
    }
    
    int left = size, need = 0;
    zht_node_t *node = 0;
    WHILE_ZHT_PATH_DOWN(iter, node)
    {
        int keylen = strlen(node->key);
        if (need < size) {
            if (left > keylen) {
                strcpy(str, node->key);
                str  += keylen;
                left -= keylen;
            } else {
                strncpy(str, node->key, left - 1);
                str  += (left - 1);
                left  = 1;          /* for null terminate */
            }
        }
        need += keylen;             /* The space needed */
    }
    str[0] = 0;

    return need;
}

void zht_print_full_path(zhtree_t *h, zht_node_t *node)
{
    zht_path_t path = zht_path_open(h, node);
    if (zht_path_assert(&path)) {
        zht_path_print(&path);
    }
    zht_path_close(&path);
}

int  zht_snprint_full_path(zhtree_t *h, zht_node_t *node, char *str, int size)
{
    int ret = 0;
    zht_path_t path = zht_path_open(h, node);
    if (zht_path_assert(&path)) {
        zht_path_snprint(&path, str, size);
    }
    zht_path_close(&path);
    return ret;
}


zht_iter_t  zhtree_iter_open(zhtree_t *h)
{
    zht_iter_t iter = {h, 0, 0};
    if (!h || !h->root) {
        return iter;
    }

    #define INIT_ROUTE_DEPTH 64
    iter.iterq = ZQUEUE_MALLOC_D(zht_child_iter_t, INIT_ROUTE_DEPTH);
    if (!iter.iterq) {
        xerr("<zhtree> %s() failed:1!\n", __FUNCTION__);
        return iter;
    }

    iter.curr = h->root;

    return iter;
}

int         zhtree_iter_assert(zht_iter_t *iter)
{
    if (iter && iter->iterq) {
        return 1;
    }
    return 0;
}

void        zhtree_iter_close(zht_iter_t *iter)
{
    if (iter && iter->iterq) {
        zqueue_free(iter->iterq);
    }
    iter->iterq = 0;
}

zaddr_t     zhtree_iter_root(zht_iter_t *iter)
{
    zqueue_clear(iter->iterq);
    iter->curr = iter->h->root;
    return iter->curr;
}

static
zaddr_t     zhtree_iter_right_up(zht_iter_t *iter)
{
    if ( zqueue_get_count(iter->iterq) ) 
    {
        zht_child_iter_t *parent_context = zqueue_get_back_base(iter->iterq);
        iter->curr = zht_child_iter_next(parent_context);   // next sibling
        if (iter->curr) {
            return iter->curr;
        } else {
            zqueue_pop_back(iter->iterq);
            return zhtree_iter_right_up(iter);
        }
    }

    return (iter->curr = 0);
}

zaddr_t     zhtree_iter_next(zht_iter_t *iter)
{
    if (iter->curr) {
        return 0;
    }
    
    if ( iter->curr->child ) {
        zht_child_iter_t child_iter = zht_child_iter_init(iter->curr);
        zht_child_iter_t *p_child_iter = zqueue_push_back(iter->iterq, &child_iter);
        iter->curr = zht_child_iter_1st(p_child_iter);
        return iter->curr;
    } else if ( zqueue_get_count(iter->iterq) ) {
        return zhtree_iter_right_up(iter);
    }

    return (iter->curr = 0);
}

zaddr_t     zhtree_iter_back(zht_iter_t *iter)
{
    zqueue_clear(iter->iterq);
    iter->curr = iter->h->root;
    
    while (iter->curr && iter->curr->child)
    {
        zht_child_iter_t child_iter = zht_child_iter_init(iter->curr);
        zht_child_iter_t *p_child_iter = zqueue_push_back(iter->iterq, &child_iter);
        iter->curr = zht_child_iter_last(p_child_iter);
    }
    
    return (iter->curr);
}

zaddr_t     zhtree_iter_prev(zht_iter_t *iter)
{
    if (iter->curr) {
        return 0;
    }
    
    if ( zqueue_get_count(iter->iterq) ) 
    {
        zht_child_iter_t *parent_context = zqueue_get_back_base(iter->iterq);
        iter->curr = zht_child_iter_prev(parent_context);   // prev sibling
        if (!iter->curr) {
            zqueue_pop_back(iter->iterq);
            iter->curr = parent_context->parent;
            return iter->curr;
        } else {
            //goto last of the curr subtree
            while (iter->curr->child) {
                zht_child_iter_t child_iter = zht_child_iter_init(iter->curr);
                zht_child_iter_t *p_child_iter = zqueue_push_back(iter->iterq, &child_iter);
                iter->curr = zht_child_iter_last(p_child_iter);
            }
            return iter->curr;
        }
    }

    return (iter->curr = 0);
}

