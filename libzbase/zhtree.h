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

#ifndef ZHTREE_H_
#define ZHTREE_H_

#include "zhash.h"


#define ZHTREE_NODE_COMMON  union {\
    struct { \
        ZH_NODE_COMMON;         \
        /* tree related */      \
        int         layer;      \
        zaddr_t     parent;     \
        zaddr_t     child;      \
        zaddr_t     left;       \
        zaddr_t     right;      \
    }; \
}

typedef struct zhtree_node { 
    ZHTREE_NODE_COMMON;
}zht_node_t;

typedef zht_node_t* zht_head_t;

typedef struct zhtree
{
    uint32_t     depth_log2;
    uint32_t     depth;
    zht_head_t  *hash_tbl;

    uint32_t     node_size;         //<! elem_size
    zqueue_t    *nodeq;             //<! vector<zht_node_t> nodeq; #elem_buf
    zstrq_t     *strq;              //<! key_buf
    
    uint32_t     ret_flag;
    
    zaddr_t      root;
    zqueue_t    *cwd;               //<! vector<zht_node_t*> cwd; #current working directory
}zhtree_t;


#define     ZHTREE_FOUND                 (1<<0)    
#define     ZHTREE_NODE_BUF_OVERFLOW     (1<<1)
#define     ZHTREE_KEY_BUF_OVERFLOW      (1<<2)
#define     ZHTREE_OBJ_BUF_OVERFLOW      (1<<3)

zhtree_t*   zhtree_malloc(uint32_t node_size, uint32_t depth_log2);
#define     ZHTREE_MALLOC(type_t, log2)  zhtree_malloc(sizeof(type_t), (log2))
void        zhtree_free(zhtree_t *h);

int         zhtree_ret_flag(zhtree_t *h);
zaddr_t     zhtree_get_root(zhtree_t *h);
zaddr_t     zhtree_get_working_dir(zhtree_t *h);

/**
 *  @param key_len  - If 0, @key_len is ignored, @key is null end str. 
 *                  - Else, @key_len is forced to be the length for @key 
 */
zaddr_t     zhtree_get_node(zhtree_t *h, zht_node_t *father, const char *key, uint32_t key_len);
zaddr_t     zhtree_set_node(zhtree_t *h, zht_node_t *father, const char *key, uint32_t key_len);

/**
 *  @param path_len - If 0, @path_len is ignored, @path is null end str. 
 *                  - Else, @path_len is forced to be the length for @path 
 */
zaddr_t     zhtree_get_path(zhtree_t *h, const char *path, uint32_t path_len);
zaddr_t     zhtree_set_path(zhtree_t *h, const char *path, uint32_t path_len);

/** iterators for traverse through node's children link */
typedef struct zhtree_children_iterator {
    zht_node_t  *parent;
    zht_node_t  *direct;        /** parent->child */
    zht_node_t  *curr;
}zht_child_iter_t;

zht_child_iter_t   zht_child_iter_init(zht_node_t *parent);
zaddr_t     zht_child_iter_1st(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_next(zht_child_iter_t *iter);
#define     ZHT_CHILD_GET_CURR(iter)        ((zht_node_t * const)(iter.curr))
int         zht_is_in_children_link(zhtree_t *h, zht_node_t *parent, zht_node_t *node);

#define FOR_ZHT_CHILD_IN(iter) \
    for (zht_child_iter_1st(&iter); ZHT_CHILD_GET_CURR(iter); zht_child_iter_next(&iter))
    
#define WHILE_GET_ZHT_CHILD(iter, child) \
    for (child = zht_child_iter_1st(&iter); child; child = zht_child_iter_next(&iter))
    



/** iterators */
typedef struct zhtree_iterator {
    zhtree_t    *h;
    zqidx_t      iter_idx;
}zht_iter_t;

zh_iter_t   zhtree_iter(zhtree_t *h);
zaddr_t     zhtree_front(zht_iter_t *iter);
zaddr_t     zhtree_back(zht_iter_t *iter);
zaddr_t     zhtree_next(zht_iter_t *iter);
zaddr_t     zhtree_prev(zht_iter_t *iter);

#endif // ZHTREE_H_