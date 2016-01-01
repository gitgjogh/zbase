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

/**
 * \terminology
 *  -bidx   elem buffer index in the buffer array
 *  -qidx   elem logi index in the abstract list/array
 */

#ifndef ZLIST_H_
#define ZLIST_H_

#include "zdefs.h"


/**
 *  zlist[qidx] = elem_array[ bidx_array[qidx] ]
 */
typedef struct zlist
{
    zspace_t    depth;
    zcount_t    count;
    zbidx_t     *qidx_2_bidx;        /** qidx to bidx */

    uint32_t    elem_size;
    zaddr_t     elem_array;
    int         b_allocated;
    int         b_allow_realloc; 
    
} zlist_t;


zcount_t    zlist_buf_attach(zlist_t *zl, zaddr_t buf, uint32_t elem_size, uint32_t depth);
void        zlist_buf_detach(zlist_t *zl);

zaddr_t     zlist_buf_malloc(zlist_t *zl, uint32_t elem_size, uint32_t depth, int b_allow_realloc);
zaddr_t     zlist_buf_realloc(zlist_t *zl, uint32_t depth, int b_allow_realloc);
void        zlist_buf_free(zlist_t *zl);

/**
 * Enlarge list buffer.
 * In case of reallocation failure, the old buf is kept unchanged.
 * 
 * @param additional_count  the count of elem to be allocated
 *                          in addition to zlist_get_count()
 * @return zlist_get_space() after buf grow. 
 */
zspace_t    zlist_buf_grow(zlist_t *zl, uint32_t additional_count);


zlist_t*    zlist_malloc(uint32_t elem_size, uint32_t depth, int b_allow_realloc);
zlist_t*    zlist_malloc_s(uint32_t elem_size, uint32_t depth);
zlist_t*    zlist_malloc_d(uint32_t elem_size, uint32_t depth);
#define     ZLIST_MALLOC_S(type_t, depth)    zlist_malloc_s(sizeof(type_t), (depth))
#define     ZLIST_MALLOC_D(type_t, depth)    zlist_malloc_d(sizeof(type_t), (depth))
void        zlist_free(zlist_t *zl);


zcount_t    zlist_get_depth(zlist_t *zl);
zcount_t    zlist_get_count(zlist_t *zl);
zspace_t    zlist_get_space(zlist_t *zl);


#define     ZLIST_ELEM_BASE(zl, bidx) \
        ((zaddr_t)(((char *)zl->elem_array) + (bidx) * zl->elem_size))
zaddr_t     zlist_qidx_2_base_in_buf(zlist_t *zl, zqidx_t qidx);
zaddr_t     zlist_qidx_2_base_in_use(zlist_t *zl, zqidx_t qidx);
int         zlist_is_qidx_in_buf(zlist_t *zl, zqidx_t qidx);
int         zlist_is_qidx_in_use(zlist_t *zl, zqidx_t qidx);

/**
 * @param elem_base must be elem_size alignment
 */
zqidx_t     zlist_base_2_qidx_in_buf(zlist_t *zl, zaddr_t elem_base);
zqidx_t     zlist_base_2_qidx_in_use(zlist_t *zl, zaddr_t elem_base);
int         zlist_is_elem_base_in_buf(zlist_t *zl, zaddr_t elem_base);
int         zlist_is_elem_base_in_use(zlist_t *zl, zaddr_t elem_base);

/**
 * @param addr no need to be elem_size alignment
 */
zqidx_t     zlist_addr_2_qidx_in_buf(zlist_t *zl, zaddr_t addr);
zqidx_t     zlist_addr_2_qidx_in_use(zlist_t *zl, zaddr_t addr);
int         zlist_is_addr_in_buf(zlist_t *zl, zaddr_t addr);
int         zlist_is_addr_in_use(zlist_t *zl, zaddr_t addr);

#define     zlist_get_elem_base         zlist_qidx_2_base_in_use
zaddr_t     zlist_get_front_base(zlist_t *zl);        //<! @ret zl[0]
zaddr_t     zlist_get_back_base(zlist_t *zl);         //<! @ret zl[count-1]


zaddr_t     zlist_set_elem_val(zlist_t *zl, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zlist_set_elem_val_itnl(zlist_t *zl, zqidx_t dst_idx, zqidx_t src_idx);


zaddr_t     zlist_pop_elem(zlist_t *zl, zqidx_t qidx);
zaddr_t     zlist_pop_front(zlist_t *zl);
zaddr_t     zlist_pop_back(zlist_t *zl);


//<! insert before qidx, insert at count is same to push back
zaddr_t     zlist_insert_elem(zlist_t *zl, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zlist_push_front(zlist_t *zl, zaddr_t elem_base);
zaddr_t     zlist_push_back(zlist_t *zl, zaddr_t elem_base);


/** @return @delete_count or 0 */
zcount_t    zlist_delete_multi_elems(
                    zlist_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count);

/** @return @insert_count or 0 */
zcount_t    zlist_insert_null_elems(
                    zlist_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count);

zcount_t    zlist_insert_some_of_others(
                    zlist_t *dst, zqidx_t dst_idx, 
                    zlist_t *src, zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zlist_insert_all_of_others(
                    zlist_t *dst, zqidx_t dst_idx, 
                    zlist_t *src);
zcount_t    zlist_push_back_some_of_others(
                    zlist_t *dst, zlist_t *src, 
                    zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zlist_push_back_all_of_others(
                    zlist_t *dst, zlist_t *src);
#define     zlist_cat(dst,src)  zlist_push_back_all_of_others((dst), (src))


typedef     int32_t (*zl_cmp_func_t) (zaddr_t cmp_base, zaddr_t elem_base);
zaddr_t     zlist_find_first_match(zlist_t *zl, zaddr_t cmp_base, zl_cmp_func_t func);
zaddr_t     zlist_pop_first_match(zlist_t *zl, zaddr_t cmp_base, zl_cmp_func_t func);


int         zlist_elem_cmp(zlist_t *zl, zl_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base);       //<! za[qidx] - elem_base
int         zlist_elem_cmp_itnl(zlist_t *zl, zl_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2);    //<! za[qidx_1] - za[qidx_2]   

void        zlist_quick_sort(zlist_t *zl, zl_cmp_func_t func);
void        zlist_quick_sort_i32(zlist_t *zl);
void        zlist_quick_sort_u32(zlist_t *zl);


typedef void  (*zl_print_func_t)  (zqidx_t idx, zaddr_t elem_base);
void        zlist_print(zlist_t *zl, const char *zl_name, zl_print_func_t func,
                    const char *delimiters, const char *terminator);

#endif //ZLIST_H_
