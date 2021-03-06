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

#include "zhash.h"
#include "sim_log.h"


zhash_t *zhash_malloc(uint32_t node_size, uint32_t depth_log2)
{
    zhash_t *zh = calloc( 1, sizeof(zhash_t) );
    if (!zh) {
        xerr("<zhash> obj malloc failed\n");
        return 0;
    }

    zh->depth_log2 = depth_log2;
    zh->depth = 1 << depth_log2;

    zh->hash_tbl = calloc(zh->depth, sizeof(zh_head_t));
    zh->nodeq = zarray_malloc_s(node_size, zh->depth);
    zarray_memzero(zh->nodeq);
    zh->strq  = zstrq_malloc(0);

    if (!zh->hash_tbl || !zh->nodeq || !zh->strq) {
        xerr("<zhash> buf malloc failed!\n");
        zhash_free(zh);
        return 0;
    }

    return zh;
}

void zhash_free(zhash_t *h)
{
    if (h) {
        if (h->hash_tbl) { free(h->hash_tbl); }
        if (h->nodeq) { zarray_free(h->nodeq); }
        if (h->strq) { zstrq_free(h->strq); }
        free(h);
    }
}

static 
uint32_t zh_nstr_time33(uint32_t type, const char *key, uint32_t key_len)
{
    uint32_t i = 0;
    uint32_t hash = type;
    for (i=0; i<key_len; ++i) {
        hash = hash * 33 + key[i]; 
    } 

    return hash; 
}

static 
uint32_t zh_cstr_time33(uint32_t type, const char *key, uint32_t *key_len)
{
    int      c = 0;
    uint32_t i = 0;
    uint32_t hash = type;
    for (i=0; (c=key[i]); ++i) {
        hash = hash * 33 + c; 
    } 

    if (key_len) { 
        *key_len = i;
    }

    return hash; 
}

static
zh_node_t *zh_find_in_collision(zhash_t *h, zh_hval_t hash, 
                            const char *key, uint32_t key_len)
{
    zh_node_t *head = h->hash_tbl[GETLSBS(hash, h->depth_log2)];
    zh_node_t *node = 0;
    for (node = head; node; node = node->next) 
    {
        if (node->hash==hash && node->key[key_len]==0 
                && strncmp(node->key, key, key_len)==0) 
        {
            return node;
        }
    }
    return 0;
}

static
zaddr_t     zhash_touch_node(zhash_t    *h, 
                            const char  *key, 
                            uint32_t     key_len,
                            int          b_insert)
{
    uint32_t hash = key_len ? zh_nstr_time33(0, key, key_len)
                            : zh_cstr_time33(0, key, &key_len);

    zh_node_t *node = zh_find_in_collision(h, hash, key, key_len);
    if (node) {
        h->ret_flag |= ZHASH_FOUND;
        return node;
    } 

    if (b_insert) 
    {
        zsq_char_t *saved_key = 0;

        if ( zarray_get_space(h->nodeq) <= 0 ) {
            xerr("<zhash> hash table overflow!\n");
            h->ret_flag |= ZHASH_NODE_BUF_OVERFLOW;
            return 0;
        } 

        saved_key = zstrq_push_back(h->strq, key, key_len);
        if ( saved_key == 0 ) {
            xerr("<zhash> key buf overflow!\n");
            h->ret_flag |= ZHASH_KEY_BUF_OVERFLOW;
            return 0;
        }
        
        node = zarray_push_back(h->nodeq, 0);
        node->hash = hash;
        node->key  = saved_key;

        /* insert to front */
        zh_node_t *head = h->hash_tbl[GETLSBS(hash, h->depth_log2)];
        node->next = head ? head->next : 0;
        h->hash_tbl[GETLSBS(hash, h->depth_log2)] = node;

        return node;
    }

    return 0;
}

zaddr_t     zhash_get_node(zhash_t *h, const char *key, uint32_t keylen)
{
    return zhash_touch_node(h, key, keylen, 0);
}

zaddr_t     zhash_set_node(zhash_t *h, const char *key, uint32_t keylen)
{
    return zhash_touch_node(h, key, keylen, 1);
}

int         zhash_ret_flag(zhash_t *h)
{
    return h->ret_flag;
}

zh_link_iter_t   zh_link_iter_init(zh_node_t *head)
{
    zh_link_iter_t iter = {head, head};
    return iter;
}
zaddr_t zh_link_iter_1st(zh_link_iter_t *iter)
{
    return iter->curr = iter->head;
}
zaddr_t zh_link_iter_next(zh_link_iter_t *iter)
{
    return iter->curr ? (iter->curr = iter->curr->next) : 0;
}
zaddr_t zh_link_iter_curr(zh_link_iter_t *iter)
{
    return iter->curr;
}

zh_iter_t   zhash_iter(zhash_t *h)
{
    zh_iter_t iter = {h, 0};
    return iter;
}
zaddr_t zhash_iter_curr(zh_iter_t *iter)
{
    return zarray_get_elem_base(iter->h->nodeq, iter->iter_idx);
}

zaddr_t zhash_iter_front(zh_iter_t *iter) 
{
    return zarray_get_elem_base(iter->h->nodeq, iter->iter_idx=0);
}
zaddr_t zhash_iter_back(zh_iter_t *iter)  
{ 
    zcount_t count = zarray_get_count(iter->h->nodeq);
    return zarray_get_elem_base(iter->h->nodeq, iter->iter_idx=count-1);
}
zaddr_t zhash_iter_next(zh_iter_t *iter)  
{ 
    return zarray_get_elem_base(iter->h->nodeq, ++ iter->iter_idx);
}
zaddr_t zhash_iter_prev(zh_iter_t *iter)  
{ 
    return zarray_get_elem_base(iter->h->nodeq, -- iter->iter_idx);
}
