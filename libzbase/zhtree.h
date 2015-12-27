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

typedef struct zhtree_node zht_node_t;

#define ZHTREE_NODE_COMMON  union {\
    struct { \
        ZH_NODE_COMMON;         \
        /* tree related */      \
        int         layer;      \
        zht_node_t *parent;     \
        zht_node_t *child;      \
        zht_node_t *left;       \
        zht_node_t *right;      \
    }; \
}

struct zhtree_node { 
    ZHTREE_NODE_COMMON;
};

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
    
//private:    
    zaddr_t      root;
    zaddr_t      wnode;             //<! current working node
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
zaddr_t     zhtree_get_wnode(zhtree_t *h);

/**
 *  @param key_len  - If 0, @key_len is ignored, @key is null end str. 
 *                  - Else, @key_len is forced to be the length for @key 
 */
zaddr_t     zhtree_get_child(zhtree_t *h, zht_node_t *parent, const char *key, uint32_t key_len);
zaddr_t     zhtree_touch_child(zhtree_t *h, zht_node_t *parent, const char *key, uint32_t key_len);

/**
 *  @param path - Could be either relative path or abspath 
 *  @param path_len - If 0, @path_len is ignored, @path is null end str. 
 *                  - Else, @path_len is forced to be the length for @path 
 */
zaddr_t     zhtree_get_node(zhtree_t *h, const char *path, uint32_t path_len);
zaddr_t     zhtree_touch_node(zhtree_t *h, const char *path, uint32_t path_len);

/**
 *  Change wdir according to @path. If @path is invalid, wdir will not change.
 *  @param path_len - If 0, @path_len is ignored, @path is null end str. 
 *                  - Else, @path_len is forced to be the length for @path 
 *  @return - New wdir if @path is valid, or else 0 if @path is invalid.
 */
zaddr_t     zhtree_change_wnode(zhtree_t *h, const char *path, uint32_t path_len);


/** iterators for traverse through node's children link */
typedef struct zhtree_children_iterator {
    zht_node_t  *parent;
    zht_node_t  *curr;
}zht_child_iter_t;

zht_child_iter_t   zht_child_iter_init(zht_node_t *parent);
zaddr_t     zht_child_iter_1st(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_next(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_last(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_prev(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_curr(zht_child_iter_t *iter);

#define FOR_ZHT_CHILD_IN(iter) \
    for (zht_child_iter_1st(&iter); zht_child_iter_curr(&iter); zht_child_iter_next(&iter))
    
#define WHILE_GET_ZHT_CHILD(iter, child) \
    for (child = zht_child_iter_1st(&iter); child; child = zht_child_iter_next(&iter))


/** iterators for traverse through the path from root to last */
typedef struct zhtree_path_iterator {
    zhtree_t    *h;
    zht_node_t  *last;
    zqueue_t    *nodeq;             /** zqueue_t<zht_node_t*> [last ... root] */
    zqidx_t      iter_idx;
}zht_path_t;

zht_path_t  zht_path_open(zhtree_t *h, zht_node_t *node);
int         zht_path_assert(zht_path_t *iter);
void        zht_path_close(zht_path_t *iter);

zaddr_t     zht_path_iter_root(zht_path_t *iter);
zaddr_t     zht_path_iter_next(zht_path_t *iter);
#define     WHILE_ZHT_PATH_DOWN(iter, node) \
    for (node = zht_path_iter_root(iter); node; node = zht_path_iter_next(iter))

zaddr_t     zht_path_iter_last(zht_path_t *iter);
zaddr_t     zht_path_iter_prev(zht_path_t *iter);
#define     WHILE_ZHT_PATH_UP(iter, node) \
    for (node = zht_path_iter_last(iter); node; node = zht_path_iter_prev(iter))

void        zht_path_print(zht_path_t *iter);
void        zht_print_full_path(zhtree_t *h, zht_node_t *node);

/**
 * @return 0 when failed. Or the space (not counting the null end) needed 
 *         for storing the entire route. In the 2nd case, the return value
 *         can be greater than @size if @size of @str is no sufficient.
 */
int         zht_path_snprint(zht_path_t *iter, char *str, int size);
int         zht_snprint_full_path(zhtree_t *h, zht_node_t *node, char *str, int size);



/** iterators */
typedef struct zhtree_iterator {
    zhtree_t    *h;
    zqueue_t    *iterq;         /** zqueue_t<zht_child_iter_t*> [root...] */
    zht_node_t  *curr;
}zht_iter_t;

zht_iter_t  zhtree_iter_open(zhtree_t *h);
int         zhtree_iter_assert(zht_iter_t *iter);
void        zhtree_iter_close(zht_iter_t *iter);

zaddr_t     zhtree_iter_root(zht_iter_t *iter);
zaddr_t     zhtree_iter_next(zht_iter_t *iter);
zaddr_t     zhtree_iter_back(zht_iter_t *iter);
zaddr_t     zhtree_iter_prev(zht_iter_t *iter);

#endif // ZHTREE_H_
