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
    zht_node_t  *direct;        /** parent->child */
    zht_node_t  *curr;
}zht_child_iter_t;

zht_child_iter_t   zht_child_iter_init(zht_node_t *parent);
zaddr_t     zht_child_iter_1st(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_next(zht_child_iter_t *iter);
zaddr_t     zht_child_iter_curr(zht_child_iter_t *iter);

#define FOR_ZHT_CHILD_IN(iter) \
    for (zht_child_iter_1st(&iter); zht_child_iter_curr(&iter); zht_child_iter_next(&iter))
    
#define WHILE_GET_ZHT_CHILD(iter, child) \
    for (child = zht_child_iter_1st(&iter); child; child = zht_child_iter_next(&iter))


/**
 * create a zqueue<zht_node_*t> route from root to node
 * @return (zht_node_*t) <root ... node>, need to be free after use.
 */
zqueue_t   *zhtree_create_route(zhtree_t *h, zht_node_t *node);
void        zhtree_free_route(zqueue_t *route);
void        zhtree_print_route(zqueue_t *route);
int         zhtree_snprint_route(zqueue_t *route, char *str, int size);

/** iterators for traverse through the route from root to last */
typedef struct zhtree_route_iterator {
    zhtree_t    *h;
    zht_node_t  *last;
    zqueue_t    *route;             /** zqueue_t<last ... root> */
    zqidx_t      iter_idx;
}zht_route_iter_t;

zht_route_iter_t    zht_route_open(zhtree_t *h, zht_node_t *node);
void                zht_route_close(zht_route_iter_t *iter);

zaddr_t     zht_route_iter_root(zht_route_iter_t *iter);
zaddr_t     zht_route_iter_next(zht_route_iter_t *iter);
#define     WHILE_ZHT_ROUTE_DOWN(iter, node) \
    for (node = zht_route_iter_root(&iter); node; node = zht_route_iter_next(&iter))

zaddr_t     zht_route_iter_last(zht_route_iter_t *iter);
zaddr_t     zht_route_iter_prev(zht_route_iter_t *iter);
#define     WHILE_ZHT_ROUTE_UP(iter, node) \
    for (node = zht_route_iter_end(&iter); node; node = zht_route_iter_prev(&iter))



/** iterators */
typedef struct zhtree_iterator {
    zhtree_t    *h;
    zqidx_t      iter_idx;
}zht_iter_t;

zh_iter_t   zhtree_iter(zhtree_t *h);
zaddr_t     zhtree_iter_front(zht_iter_t *iter);
zaddr_t     zhtree_iter_next(zht_iter_t *iter);
zaddr_t     zhtree_iter_back(zht_iter_t *iter);
zaddr_t     zhtree_iter_prev(zht_iter_t *iter);

#endif // ZHTREE_H_
