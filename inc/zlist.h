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

#ifndef ZLIST_H_
#define ZLIST_H_

#include "zqueue.h"


typedef     zq_count_t      zl_count_t;
typedef     zl_count_t      zl_space_t;
typedef     zq_addr_t       zl_addr_t;
typedef     zq_idx_t        zl_idx_t;   
typedef     zq_idx_t        zl_bidx_t;
#define     ZL_ERR_IDX      ZQ_ERR_IDX

/**
 *  zlist[qidx] = elem_array[ bidx_array[qidx] ]
 */
typedef struct zlist
{
    zq_space_t  depth;
    zq_count_t  count;
    zl_bidx_t  *bidx_array;

    uint32_t    elem_size;
    zl_addr_t   elem_array;

} zlist_t;


zlist_t*    zlist_malloc(uint32_t elem_size, uint32_t depth);
#define     ZLIST_MALLOC(type_t, depth)    zlist_malloc(sizeof(type_t), (depth))
zlist_t*    zlist_realloc(zlist_t *zl, uint32_t depth);
void        zlist_free(zlist_t *zl);
zl_count_t  zlist_get_depth(zlist_t *zl);
zl_count_t  zlist_get_count(zlist_t *zl);
zl_space_t  zlist_get_space(zlist_t *zl);


#define     ZLIST_ELEM_BASE(q, bidx)       (((char *)q->elem_array) + (bidx) * q->elem_size)
zl_bidx_t   zlist_get_elem_bidx(zlist_t *zl, zl_idx_t qidx);        //<! 0<= qidx < count
zl_addr_t   zlist_get_elem_base(zlist_t *zl, zl_idx_t qidx);        //<! 0<= qidx < count
zl_addr_t   zlist_get_front_base(zlist_t *zl);        //<! @ret zl[0]
zl_addr_t   zlist_get_back_base(zlist_t *zl);         //<! @ret zl[count-1]


zl_addr_t   zlist_set_elem_val(zlist_t *zl, zl_idx_t qidx, zl_addr_t elem_base);
zl_addr_t   zlist_set_elem_val_itnl(zlist_t *zl, zl_idx_t dst_idx, zl_idx_t src_idx);


zl_addr_t   zlist_pop_elem(zlist_t *zl, zl_idx_t qidx);
zl_addr_t   zlist_pop_front(zlist_t *zl);
zl_addr_t   zlist_pop_back(zlist_t *zl);


//<! insert before qidx, insert at count is same to push back
zl_addr_t   zlist_insert_elem(zlist_t *zl, zl_idx_t qidx, zl_addr_t elem_base);
zl_addr_t   zlist_push_front(zlist_t *zl, zl_addr_t elem_base);
zl_addr_t   zlist_push_back(zlist_t *zl, zl_addr_t elem_base);


/** @return @delete_count or 0 */
zl_count_t  zlist_delete_multi_elems(
                    zlist_t *dst, zl_idx_t delete_from, 
                    zl_count_t delete_count);

/** @return @insert_count or 0 */
zl_count_t  zlist_insert_null_elems(
                    zlist_t *dst, zl_idx_t insert_before, 
                    zl_count_t insert_count);

zl_count_t  zlist_insert_some_of_others(
                    zlist_t *dst, zl_idx_t dst_idx, 
                    zlist_t *src, zl_idx_t src_start, 
                    zl_count_t insert_count);
zl_count_t  zlist_insert_all_of_others(
                    zlist_t *dst, zl_idx_t dst_idx, 
                    zlist_t *src);
zl_count_t  zlist_push_back_some_of_others(
                    zlist_t *dst, zlist_t *src, 
                    zl_idx_t src_start, 
                    zl_count_t insert_count);
zl_count_t  zlist_push_back_all_of_others(
                    zlist_t *dst, zlist_t *src);
#define     zlist_cat(dst,src)  zlist_push_back_all_of_others((dst), (src))


typedef  int32_t (*zl_cmp_func_t) (zl_addr_t cmp_base, zl_addr_t elem_base);
zl_addr_t   zlist_find_first_match(zlist_t *zl, zl_addr_t cmp_base, zl_cmp_func_t func);
zl_addr_t   zlist_pop_first_match(zlist_t *zl, zl_addr_t cmp_base, zl_cmp_func_t func);


int         zlist_elem_cmp(zlist_t *zl, zl_cmp_func_t func, zl_idx_t qidx, zq_addr_t elem_base);       //<! q[qidx] - elem_base
int         zlist_elem_cmp_itnl(zlist_t *zl, zl_cmp_func_t func, zl_idx_t qidx_1, zl_idx_t qidx_2);    //<! q[qidx_1] - q[qidx_2]   

void        zlist_quick_sort(zlist_t *zl, zl_cmp_func_t func);
void        zlist_quick_sort_i32(zlist_t *zl);
void        zlist_quick_sort_u32(zlist_t *zl);


typedef void  (*zl_print_func_t)  (zq_idx_t idx, zl_addr_t elem_base);
void        zlist_print(char *zl_name, zlist_t *zl, zl_print_func_t func);

#endif //ZLIST_H_
