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

#ifndef ZHASH_H_
#define ZHASH_H_

#include "zdefs.h"
#include "zqueue.h"
#include "zstrq.h"

typedef     uint32_t        zh_hash_t;      /* type for hash value */
typedef     int32_t         zh_type_t;

#define ZH_NODE_COMMON  union {\
    struct {\
        zaddr_t     next;   \
        zh_hash_t   hash;   \
        char       *key;    \
    };\
}

typedef struct zhash_node { 
    ZH_NODE_COMMON;
}zh_node_t;

typedef zh_node_t* zh_head_t;

typedef struct zhash
{
    uint32_t    depth_log2;
    uint32_t    depth;
    zh_head_t  *hash_tbl;

    uint32_t    node_size;      //<! elem_size
    zqueue_t   *nodeq;          //<! elem_buf
    zstrq_t    *strq;           //<! key_buf
    
    uint32_t    ret_flag;
}zhash_t;


#define     ZHASH_FOUND                 (1<<0)    
#define     ZHASH_NODE_BUF_OVERFLOW     (1<<1)
#define     ZHASH_KEY_BUF_OVERFLOW      (1<<2)
#define     ZHASH_OBJ_BUF_OVERFLOW      (1<<3)
int         zhash_ret_flag(zhash_t *h);
zhash_t*    zhash_malloc(uint32_t node_size, uint32_t depth_log2);
#define     ZHASH_MALLOC(type_t, log2)  zhash_malloc(sizeof(type_t), (log2))
void        zhash_free(zhash_t *h);


/**
 *  @param key_len  - If 0, @key_len is ignored, @key is null end str. 
 *                  - Else, @key_len is forced to be the length for @key 
 */
zaddr_t     zhash_get_node(zhash_t *h, const char *key, uint32_t key_len);
zaddr_t     zhash_set_node(zhash_t *h, const char *key, uint32_t key_len);


/** iterators */
typedef struct zhash_iterator {
    zhash_t *h;
    zqidx_t iter_idx;
}zh_iter_t;

zh_iter_t   zhash_iter(zhash_t *h);
zaddr_t     zhash_front(zh_iter_t *iter);
zaddr_t     zhash_back(zh_iter_t *iter);
zaddr_t     zhash_next(zh_iter_t *iter);
zaddr_t     zhash_prev(zh_iter_t *iter);

#endif // ZHASH_H_