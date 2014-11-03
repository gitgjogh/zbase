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

#ifndef uint32_t
#define uint32_t unsigned int
#endif

#ifndef int32_t
#define int32_t int
#endif


typedef     int32_t         zq_count_t;
typedef     zq_count_t      zq_space_t;         //<! just for readability 
typedef     int32_t         zq_idx_t;           //<! logic idx
typedef     void*           zq_addr_t;
#define     ZQ_ERR_IDX      (-1)



typedef struct z_bi_dir_circular_queue
{
    zq_space_t  depth;
    zq_count_t  count;
    uint32_t    elem_size;
    zq_addr_t   elem_array;

//private:
    zq_addr_t   elem_swap;
}zqueue_t;


uint32_t    zqueue_size(uint32_t elem_size, uint32_t depth);
#define     ZQUEUE_SIZE(type_t, depth)      zqueue_size(sizeof(type_t), (depth))
zqueue_t*   zqueue_placement_new(zq_addr_t buf, uint32_t elem_size, uint32_t depth);
zqueue_t*   zqueue_malloc(uint32_t elem_size, uint32_t depth);
zqueue_t*   zqueue_realloc(zqueue_t *q, uint32_t depth);
#define     ZQUEUE_MALLOC(type_t, depth)    zqueue_malloc(sizeof(type_t), (depth))
void        zqueue_free(zqueue_t *q);
zq_count_t  zqueue_get_depth(zqueue_t *q);
zq_count_t  zqueue_get_count(zqueue_t *q);
zq_space_t  zqueue_get_space(zqueue_t *q);


#define     ZQUEUE_ELEM_BASE(q, bidx)       (((char *)q->elem_array) + (bidx) * q->elem_size)
zq_addr_t   zqueue_get_elem_base(zqueue_t *q, zq_idx_t qidx);       //<! 0<= qidx < count
zq_addr_t   zqueue_get_front_base(zqueue_t *q);        //<! @ret q[0]
zq_addr_t   zqueue_get_back_base(zqueue_t *q);         //<! @ret q[count-1]
zq_addr_t   zqueue_get_last_base(zqueue_t *q);         //<! @ret q[count]


zq_addr_t   zqueue_set_elem_val(zqueue_t *q, zq_idx_t qidx, zq_addr_t elem_base);
zq_addr_t   zqueue_set_elem_val_itnl(zqueue_t *q, zq_idx_t dst_idx, zq_idx_t src_idx);


zq_addr_t   zqueue_pop_elem(zqueue_t *q, zq_idx_t qidx);
zq_addr_t   zqueue_pop_front(zqueue_t *q);
zq_addr_t   zqueue_pop_back(zqueue_t *q);

//! insert at count is same to push back
zq_addr_t   zqueue_insert_elem(zqueue_t *q, zq_idx_t qidx, zq_addr_t elem_base);
zq_addr_t   zqueue_push_front(zqueue_t *q, zq_addr_t elem_base);
zq_addr_t   zqueue_push_back(zqueue_t *q, zq_addr_t elem_base);


/** @return @delete_count or 0 */
zq_count_t  zqueue_delete_multi_elems(
                    zqueue_t *dst, zq_idx_t delete_from, 
                    zq_count_t delete_count);

/** @return @insert_count or 0 */
zq_count_t  zqueue_insert_null_elems(
                    zqueue_t *dst, zq_idx_t insert_before, 
                    zq_count_t insert_count);

zq_count_t  zqueue_insert_some_of_others(
                    zqueue_t *dst, zq_idx_t insert_before, 
                    zqueue_t *src, zq_idx_t src_start, 
                    zq_count_t insert_count);
zq_count_t  zqueue_insert_all_of_others (
                    zqueue_t *dst, zq_idx_t insert_before, 
                    zqueue_t *src);
zq_count_t  zqueue_push_back_some_of_others(
                    zqueue_t *dst, zqueue_t *src, 
                    zq_idx_t src_start, 
                    zq_count_t insert_count);
zq_count_t  zqueue_push_back_all_of_others(
                    zqueue_t *dst, zqueue_t *src);
#define     zqueue_cat(dst, src)  zqueue_push_back_all_of_others((dst), (src))


typedef int32_t (*zq_cmp_func_t)  (zq_addr_t base1, zq_addr_t base2);
zq_addr_t   zqueue_find_first_match(zqueue_t *q, zq_addr_t elem_base, zq_cmp_func_t func);
zq_addr_t   zqueue_pop_first_match(zqueue_t *q, zq_addr_t elem_base, zq_cmp_func_t func);


int         zqueue_elem_cmp(zqueue_t *q, zq_cmp_func_t func, zq_idx_t qidx, zq_addr_t elem_base);       //<! q[qidx] - elem_base
int         zqueue_elem_cmp_itnl(zqueue_t *q, zq_cmp_func_t func, zq_idx_t qidx_1, zq_idx_t qidx_2);    //<! q[qidx_1] - q[qidx_2]   

void        zqueue_quick_sort(zqueue_t *q, zq_cmp_func_t func);
void        zqueue_quick_sort_i32(zqueue_t *q);
void        zqueue_quick_sort_u32(zqueue_t *q);


typedef void  (*zq_print_func_t)  (zq_idx_t idx, zq_addr_t elem_base);
void        zqueue_print(char *q_name, zqueue_t *q, zq_print_func_t func);


/** iterators */
typedef struct zqueue_iterator {
    zqueue_t *q;
    zq_idx_t iter_idx;
}zq_iter_t;

zq_iter_t   zqueue_iter(zqueue_t *q);
zq_addr_t   zqueue_front(zq_iter_t *iter);
zq_addr_t   zqueue_next(zq_iter_t *iter);
zq_addr_t   zqueue_back(zq_iter_t *iter);
zq_addr_t   zqueue_prev(zq_iter_t *iter);



#endif //ZQUEUE_H_
