/*****************************************************************************
 * Copyright 2015 Jeff <ggjogh@gmail.com>
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

#ifndef ZARRAY_H_
#define ZARRAY_H_

#include "zdefs.h"

typedef struct z_array
{
    zspace_t  depth;
    zcount_t  count;
    uint32_t  elem_size;
    zaddr_t   elem_array;
    int       b_allocated;
    int       b_allow_realloc;        

//private:
    zaddr_t   elem_swap;
}zarray_t;

zcount_t    zarray_buf_attach(zarray_t *za, zaddr_t buf, uint32_t elem_size, uint32_t depth);
void        zarray_buf_detach(zarray_t *za);

zaddr_t     zarray_buf_malloc(zarray_t *za, uint32_t elem_size, uint32_t depth, int b_allow_realloc);
zaddr_t     zarray_buf_realloc(zarray_t *za, uint32_t depth, int b_allow_realloc);
void        zarray_buf_free(zarray_t *za);

/**
 * Enlarge array buffer.
 * In case of reallocation failure, the old buf is kept unchanged.
 * 
 * @param additional_count  the count of elem to be allocated
 *                          in addition to zarray_get_count()
 * @return zarray_get_space() after buf grow. 
 */
zspace_t    zarray_buf_grow(zarray_t *za, uint32_t additional_count);


zarray_t*   zarray_malloc(uint32_t elem_size, uint32_t depth, int b_allow_realloc);
zarray_t*   zarray_malloc_s(uint32_t elem_size, uint32_t depth);
zarray_t*   zarray_malloc_d(uint32_t elem_size, uint32_t depth);
#define     ZARRAY_MALLOC_S(type_t, depth)    zarray_malloc_s(sizeof(type_t), (depth))
#define     ZARRAY_MALLOC_D(type_t, depth)    zarray_malloc_d(sizeof(type_t), (depth))
void        zarray_free(zarray_t *za);

void        zarray_memzero(zarray_t *za);
void        zarray_clear(zarray_t *za);

zcount_t    zarray_get_depth(zarray_t *za);
zcount_t    zarray_get_count(zarray_t *za);
zspace_t    zarray_get_space(zarray_t *za);


#define     ZARRAY_ELEM_BASE(za, bidx) \
        ((zaddr_t)(((char *)za->elem_array) + (bidx) * za->elem_size))
zaddr_t     zarray_qidx_2_base_in_buf(zarray_t *za, zqidx_t qidx);
zaddr_t     zarray_qidx_2_base_in_use(zarray_t *za, zqidx_t qidx);
int         zarray_is_qdix_in_buf(zarray_t *za, zqidx_t qidx);
int         zarray_is_qdix_in_use(zarray_t *za, zqidx_t qidx);

/**
 * @param elem_base must be elem_size alignment
 */
zqidx_t     zarray_base_2_qidx_in_buf(zarray_t *za, zaddr_t elem_base);
zqidx_t     zarray_base_2_qidx_in_use(zarray_t *za, zaddr_t elem_base);
int         zarray_is_elem_base_in_buf(zarray_t *za, zaddr_t elem_base);
int         zarray_is_elem_base_in_use(zarray_t *za, zaddr_t elem_base);

/**
 * @param addr no need to be elem_size alignment
 */
zqidx_t     zarray_addr_2_qidx_in_buf(zarray_t *za, zaddr_t addr);
zqidx_t     zarray_addr_2_qidx_in_use(zarray_t *za, zaddr_t addr);
int         zarray_is_addr_in_buf(zarray_t *za, zaddr_t addr);
int         zarray_is_addr_in_use(zarray_t *za, zaddr_t addr);


#define     zarray_get_elem_base        zarray_qidx_2_base_in_use
zaddr_t     zarray_get_front_base(zarray_t *za);        //<! @ret za[0]
zaddr_t     zarray_get_back_base(zarray_t *za);         //<! @ret za[count-1]
zaddr_t     zarray_get_last_base(zarray_t *za);         //<! @ret za[count]


zaddr_t     zarray_set_elem_val(zarray_t *za, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zarray_set_elem_val_itnl(zarray_t *za, zqidx_t dst_idx, zqidx_t src_idx);


zaddr_t     zarray_pop_elem(zarray_t *za, zqidx_t qidx);
zaddr_t     zarray_pop_front(zarray_t *za);
zaddr_t     zarray_pop_back(zarray_t *za);

//! insert at count is same to push back
zaddr_t     zarray_insert_elem(zarray_t *za, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zarray_push_front(zarray_t *za, zaddr_t elem_base);
zaddr_t     zarray_push_back(zarray_t *za, zaddr_t elem_base);


/** @return @delete_count or 0 */
zcount_t    zarray_delete_multi_elems(
                    zarray_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count);

/** @return @insert_count or 0 */
zcount_t    zarray_insert_null_elems(
                    zarray_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count);

zcount_t    zarray_insert_some_of_others(
                    zarray_t *dst, zqidx_t insert_before, 
                    zarray_t *src, zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zarray_insert_all_of_others (
                    zarray_t *dst, zqidx_t insert_before, 
                    zarray_t *src);
zcount_t    zarray_push_back_some_of_others(
                    zarray_t *dst, zarray_t *src, 
                    zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zarray_push_back_all_of_others(
                    zarray_t *dst, zarray_t *src);
#define     zarray_cat(dst, src)  zarray_push_back_all_of_others((dst), (src))


typedef int32_t (*za_cmp_func_t)  (zaddr_t base1, zaddr_t base2);
zaddr_t     zarray_find_first_match(zarray_t *za, zaddr_t elem_base, za_cmp_func_t func);
zaddr_t     zarray_pop_first_match(zarray_t *za, zaddr_t elem_base, za_cmp_func_t func);


int         zarray_elem_cmp(zarray_t *za, za_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base);       //<! za[qidx] - elem_base
int         zarray_elem_cmp_itnl(zarray_t *za, za_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2);    //<! za[qidx_1] - za[qidx_2]   

void        zarray_quick_sort(zarray_t *za, za_cmp_func_t func);
void        zarray_quick_sort_i32(zarray_t *za);
void        zarray_quick_sort_u32(zarray_t *za);


typedef void  (*za_print_func_t)  (zqidx_t idx, zaddr_t elem_base);
void        zarray_print(zarray_t *za, const char *q_name, za_print_func_t func,
                    const char *delimiters, const char *terminator);

/** iterators */
typedef struct zarray_iterator {
    zarray_t    *za;
    zqidx_t     iter_idx;
}za_iter_t;

za_iter_t   zarray_iter(zarray_t *za);
zaddr_t     zarray_front(za_iter_t *iter);
zaddr_t     zarray_next(za_iter_t *iter);
zaddr_t     zarray_back(za_iter_t *iter);
zaddr_t     zarray_prev(za_iter_t *iter);



#endif //ZARRAY_H_
