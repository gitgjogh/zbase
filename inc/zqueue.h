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

#ifndef ZQUEUE_H_
#define ZQUEUE_H_

#include "zdefs.h"

typedef struct z_bi_dir_circular_queue
{
    zspace_t  depth;
    zcount_t  count;
    uint32_t  elem_size;
    zaddr_t   elem_array;

//private:
    zaddr_t   elem_swap;
}zqueue_t;


uint32_t    zqueue_size(uint32_t elem_size, uint32_t depth);
#define     ZQUEUE_SIZE(type_t, depth)      zqueue_size(sizeof(type_t), (depth))
zqueue_t*   zqueue_placement_new(zaddr_t buf, uint32_t elem_size, uint32_t depth);
zqueue_t*   zqueue_malloc(uint32_t elem_size, uint32_t depth);
void        zqueue_memzero(zqueue_t *q);
zqueue_t*   zqueue_realloc(zqueue_t *q, uint32_t depth);
#define     ZQUEUE_MALLOC(type_t, depth)    zqueue_malloc(sizeof(type_t), (depth))
void        zqueue_free(zqueue_t *q);
zcount_t    zqueue_get_depth(zqueue_t *q);
zcount_t    zqueue_get_count(zqueue_t *q);
zspace_t    zqueue_get_space(zqueue_t *q);


#define     ZQUEUE_ELEM_BASE(q, bidx)       (((char *)q->elem_array) + (bidx) * q->elem_size)
zaddr_t     zqueue_get_elem_base(zqueue_t *q, zqidx_t qidx);       //<! 0<= qidx < count
zaddr_t     zqueue_get_front_base(zqueue_t *q);        //<! @ret q[0]
zaddr_t     zqueue_get_back_base(zqueue_t *q);         //<! @ret q[count-1]
zaddr_t     zqueue_get_last_base(zqueue_t *q);         //<! @ret q[count]


zaddr_t     zqueue_set_elem_val(zqueue_t *q, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zqueue_set_elem_val_itnl(zqueue_t *q, zqidx_t dst_idx, zqidx_t src_idx);


zaddr_t     zqueue_pop_elem(zqueue_t *q, zqidx_t qidx);
zaddr_t     zqueue_pop_front(zqueue_t *q);
zaddr_t     zqueue_pop_back(zqueue_t *q);

//! insert at count is same to push back
zaddr_t     zqueue_insert_elem(zqueue_t *q, zqidx_t qidx, zaddr_t elem_base);
zaddr_t     zqueue_push_front(zqueue_t *q, zaddr_t elem_base);
zaddr_t     zqueue_push_back(zqueue_t *q, zaddr_t elem_base);


/** @return @delete_count or 0 */
zcount_t    zqueue_delete_multi_elems(
                    zqueue_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count);

/** @return @insert_count or 0 */
zcount_t    zqueue_insert_null_elems(
                    zqueue_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count);

zcount_t    zqueue_insert_some_of_others(
                    zqueue_t *dst, zqidx_t insert_before, 
                    zqueue_t *src, zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zqueue_insert_all_of_others (
                    zqueue_t *dst, zqidx_t insert_before, 
                    zqueue_t *src);
zcount_t    zqueue_push_back_some_of_others(
                    zqueue_t *dst, zqueue_t *src, 
                    zqidx_t src_start, 
                    zcount_t insert_count);
zcount_t    zqueue_push_back_all_of_others(
                    zqueue_t *dst, zqueue_t *src);
#define     zqueue_cat(dst, src)  zqueue_push_back_all_of_others((dst), (src))


typedef int32_t (*zq_cmp_func_t)  (zaddr_t base1, zaddr_t base2);
zaddr_t     zqueue_find_first_match(zqueue_t *q, zaddr_t elem_base, zq_cmp_func_t func);
zaddr_t     zqueue_pop_first_match(zqueue_t *q, zaddr_t elem_base, zq_cmp_func_t func);


int         zqueue_elem_cmp(zqueue_t *q, zq_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base);       //<! q[qidx] - elem_base
int         zqueue_elem_cmp_itnl(zqueue_t *q, zq_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2);    //<! q[qidx_1] - q[qidx_2]   

void        zqueue_quick_sort(zqueue_t *q, zq_cmp_func_t func);
void        zqueue_quick_sort_i32(zqueue_t *q);
void        zqueue_quick_sort_u32(zqueue_t *q);


typedef void  (*zq_print_func_t)  (zqidx_t idx, zaddr_t elem_base);
void        zqueue_print(zqueue_t *q, const char *q_name, zq_print_func_t func,
                    const char *delimiters, const char *terminator);

/** iterators */
typedef struct zqueue_iterator {
    zqueue_t    *q;
    zqidx_t     iter_idx;
}zq_iter_t;

zq_iter_t   zqueue_iter(zqueue_t *q);
zaddr_t     zqueue_front(zq_iter_t *iter);
zaddr_t     zqueue_next(zq_iter_t *iter);
zaddr_t     zqueue_back(zq_iter_t *iter);
zaddr_t     zqueue_prev(zq_iter_t *iter);



#endif //ZQUEUE_H_
