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

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "zqueue.h"
#include "sim_log.h"


static zaddr_t  zqueue_elem_2_swap(zqueue_t *q, zqidx_t qidx);
static zaddr_t  zqueue_swap_2_elem(zqueue_t *q, zqidx_t qidx);
static int32_t  zqueue_safe_cmp(zq_cmp_func_t func, zaddr_t base1, zaddr_t base2);
static void     zqueue_quick_sort_iter(zqueue_t *q, zq_cmp_func_t func, 
                                       zqidx_t start, zqidx_t end);


uint32_t zqueue_size(uint32_t elem_size, uint32_t depth)
{
    uint32_t total_size = sizeof(zqueue_t) + elem_size * (depth + 1);
    return   total_size;
}

zqueue_t* zqueue_placement_new(zaddr_t buf, uint32_t elem_size, uint32_t depth)
{
    zqueue_t *q = (zqueue_t *)buf;

    memset(q, 0, sizeof(zqueue_t));
    q->depth = depth;
    q->count = 0;
    q->elem_size  = elem_size;
    q->elem_swap  = (void *)( (char *)buf + sizeof(zqueue_t) );
    q->elem_array = (void *)( (char *)q->elem_swap + elem_size );

    return q;
}

zqueue_t *zqueue_malloc(uint32_t elem_size, uint32_t depth)
{
    zaddr_t buf = malloc( zqueue_size(elem_size, depth) );
    if (!buf) {
        printf ("%s() failed!\n", __FUNCTION__);
    } else {
        return zqueue_placement_new(buf, elem_size, depth);
    }
    return 0;
}

void zqueue_memzero(zqueue_t *q) 
{
    if (q) {
        memset(q->elem_array, 0, q->elem_size * q->depth);
    }
}

zqueue_t* zqueue_realloc(zqueue_t *q, uint32_t depth)
{
    if ((int)depth > q->depth) {
        uint32_t new_size = zqueue_size(q->elem_size, depth);
        void *new_buf = realloc((void *)q, new_size);
        if (new_buf && new_buf != (void *)q) {
            zqueue_t *nq = zqueue_placement_new(new_buf, q->elem_size, depth);
            zqueue_push_back_all_of_others(nq, q);
            zqueue_free(q);
            return nq;
        }
    }
    return q;
}

void zqueue_free(zqueue_t *q)
{
    if (q) {
        if (q->elem_array) { free(q->elem_array); }
        free(q); 
    }
}

zcount_t zqueue_get_count(zqueue_t *q)
{
    return (q->count);
}

zspace_t zqueue_get_space(zqueue_t *q)
{
    return q->depth - q->count;
}

zaddr_t zqueue_qidx_2_base_in_use(zqueue_t *q, zqidx_t qidx)
{
    return zqueue_is_qdix_in_use(q, qidx) ? ZQUEUE_ELEM_BASE(q, qidx) : 0;
}

zaddr_t zqueue_qidx_2_base_in_buf(zqueue_t *q, zqidx_t qidx)
{
    return zqueue_is_qdix_in_buf(q, qidx) ? ZQUEUE_ELEM_BASE(q, qidx) : 0;
}

int zqueue_is_qdix_in_buf(zqueue_t *q, zqidx_t qidx)
{
    return (0<=qidx && qidx < q->depth);
}

int zqueue_is_qdix_in_use(zqueue_t *q, zqidx_t qidx)
{
    return (0<=qidx && qidx < q->count);
}

#define     ZQUEUE_NEAREST_BIDX(q, addr) \
        ((((char *)(addr))-((char *)(q->elem_array))) / (q->elem_size))

zqidx_t zqueue_base_2_qidx_in_buf(zqueue_t *q, zaddr_t elem_base)
{
    if (zqueue_is_addr_in_buf(q, elem_base)) {
        zbidx_t bidx  = ZQUEUE_NEAREST_BIDX(q, elem_base);
        if (ZQUEUE_ELEM_BASE(q, bidx) == elem_base) {
            return bidx;
        }
    }
    return -1;
}

zqidx_t zqueue_base_2_qidx_in_use(zqueue_t *q, zaddr_t elem_base)
{
    zbidx_t qidx = zqueue_base_2_qidx_in_buf(q, elem_base);
    return zqueue_is_qdix_in_use(q, qidx) ? qidx : -1;
}

zqidx_t zqueue_addr_2_qidx_in_buf(zqueue_t *q, zaddr_t addr)
{
    return zqueue_is_addr_in_buf(q, addr) ? ZQUEUE_NEAREST_BIDX(q, addr) : -1;
}

zqidx_t zqueue_addr_2_qidx_in_use(zqueue_t *q, zaddr_t addr)
{
    return zqueue_is_addr_in_use(q, addr) ? ZQUEUE_NEAREST_BIDX(q, addr) : -1;
}

int zqueue_is_addr_in_buf(zqueue_t *q, zaddr_t addr)
{
    return (q->elem_array <= addr && addr < ZQUEUE_ELEM_BASE(q, q->depth));
}

int zqueue_is_addr_in_use(zqueue_t *q, zaddr_t addr)
{
    return (q->elem_array <= addr && addr < ZQUEUE_ELEM_BASE(q, q->count));
}

int zqueue_is_elem_base_in_buf(zqueue_t *q, zaddr_t elem_base)
{
    return zqueue_base_2_qidx_in_buf(q, elem_base) >= 0;
}

int zqueue_is_elem_base_in_use(zqueue_t *q, zaddr_t elem_base)
{
    return zqueue_base_2_qidx_in_use(q, elem_base) >= 0;
}

zaddr_t zqueue_get_front_base(zqueue_t *q)
{
    return zqueue_qidx_2_base_in_use(q, 0);
}

zaddr_t zqueue_get_back_base(zqueue_t *q)
{
    return zqueue_qidx_2_base_in_use(q, q->count - 1);
}

zaddr_t zqueue_get_last_base(zqueue_t *q)
{
    return zqueue_qidx_2_base_in_buf(q, q->count);
}

zq_iter_t   zqueue_iter(zqueue_t *q)
{
    zq_iter_t iter = {q, 0};
    return iter;
}

zaddr_t   zqueue_front(zq_iter_t *iter)
{
    return zqueue_get_elem_base(iter->q, iter->iter_idx=0);
}
zaddr_t   zqueue_next(zq_iter_t *iter)
{
    return zqueue_get_elem_base(iter->q, ++ iter->iter_idx);
}
zaddr_t   zqueue_back(zq_iter_t *iter)
{
    zcount_t count = zqueue_get_count(iter->q);
    return zqueue_get_elem_base(iter->q, iter->iter_idx=count-1);
}
zaddr_t   zqueue_prev(zq_iter_t *iter)
{
    return zqueue_get_elem_base(iter->q, -- iter->iter_idx);
}

zaddr_t zqueue_pop_elem(zqueue_t *q, zqidx_t qidx)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (base) {
        zmem_swap_near_block(base, q->elem_size, 1, q->count-qidx-1);
        q->count -= 1;

        return zqueue_qidx_2_base_in_buf(q, q->count);
    }

    return 0;
}

zaddr_t zqueue_pop_front(zqueue_t *q)
{
    return zqueue_pop_elem(q, 0);
}

zaddr_t zqueue_pop_back(zqueue_t *q)
{
    return zqueue_pop_elem(q, q->count - 1);
}

zaddr_t zqueue_insert_elem(zqueue_t *q, zqidx_t qidx, zaddr_t elem_base)
{
    zcount_t count = zqueue_get_count(q);
    zspace_t space = zqueue_get_space(q);

    if (0<=qidx && qidx<=count && space>=1)  
    {
        zaddr_t  base  = zqueue_qidx_2_base_in_buf(q, qidx);
        zmem_swap_near_block(base, q->elem_size, q->count-qidx, 1);
        q->count += 1;

        if (elem_base) {
            memcpy(base, elem_base, q->elem_size);
        }

        return base;
    }

    return 0;
}

zaddr_t zqueue_push_front(zqueue_t *q, zaddr_t elem_base)
{
    return zqueue_insert_elem(q, 0, elem_base);
}

zaddr_t zqueue_push_back(zqueue_t *q, zaddr_t elem_base)
{
    return zqueue_insert_elem(q, q->count, elem_base);
}

zcount_t zqueue_insert_null_elems(
                    zqueue_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count)
{
    zspace_t dst_space = zqueue_get_space(dst);
    zcount_t dst_count = zqueue_get_count(dst);
    
    if (0<=insert_before && insert_before<=dst_count && dst_space>=insert_count) 
    {
        zaddr_t  dst_base  = zqueue_qidx_2_base_in_buf(dst, insert_before);
        zmem_swap_near_block(dst_base, dst->elem_size, 
                             dst->count - insert_before, 
                             insert_count);
        dst->count += insert_count;
        return insert_count;
    }

    return 0;
}

zcount_t  zqueue_delete_multi_elems(
                    zqueue_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count)
{
    zspace_t dst_space = zqueue_get_space(dst);
    zcount_t dst_count = zqueue_get_count(dst);
    zaddr_t  dst_base  = zqueue_qidx_2_base_in_use(dst, delete_from);

    if (dst_base && dst_count>=delete_count) 
    {
        zmem_swap_near_block(dst_base, dst->elem_size, 
                             delete_count, 
                             dst_count - delete_from - delete_count);
        dst->count -= delete_count;
        return delete_count;
    }

    return 0;
}

zcount_t zqueue_insert_some_of_others(
                    zqueue_t *dst, zqidx_t insert_at,
                    zqueue_t *src, zqidx_t src_start, 
                    zcount_t insert_count)
{
    zcount_t real_insert = zqueue_insert_null_elems(dst, insert_at, insert_count);
    
    if (real_insert > 0) {
        if (src) {
            zqidx_t   qidx;
            for (qidx = 0; qidx<insert_count; ++qidx) {
                zaddr_t src_base = zqueue_get_elem_base(src, src_start+qidx);
                zaddr_t dst_base = zqueue_get_elem_base(dst, insert_at+qidx);

                if (dst_base && src_base) {
                    memcpy(dst_base, src_base, dst->elem_size);
                }
            }
        }
    }

    return real_insert;
}

zcount_t  zqueue_insert_all_of_others(zqueue_t *dst, zqidx_t insert_at, zqueue_t *src)
{
    return zqueue_insert_some_of_others(dst, insert_at, 
                               src, 0, zqueue_get_count(src));
}

zcount_t  zqueue_push_back_some_of_others(zqueue_t *dst, zqueue_t *src, 
                                   zqidx_t src_start, 
                                   zcount_t merge_count)
{
    return zqueue_insert_some_of_others(dst, zqueue_get_count(dst), 
                               src, src_start, merge_count);
}

zcount_t  zqueue_push_back_all_of_others(zqueue_t *dst, zqueue_t *src)
{
    return zqueue_insert_some_of_others(dst, zqueue_get_count(dst), 
                               src, 0, zqueue_get_count(src));
}

zaddr_t zqueue_set_elem_val(zqueue_t *q, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (qidx && elem_base) {
        memcpy(base, elem_base, q->elem_size);
        return base;
    }
    return 0;
}

zaddr_t zqueue_set_elem_val_itnl(zqueue_t *q, zqidx_t dst_idx, zqidx_t src_idx)
{
    zaddr_t dst_base = zqueue_get_elem_base(q, dst_idx);
    zaddr_t src_base = zqueue_get_elem_base(q, src_idx);

    if (dst_base && src_base) {
        memcpy(dst_base, src_base, q->elem_size);
        return dst_base;
    }
    return 0;
}

static 
zaddr_t zqueue_elem_2_swap(zqueue_t *q, zqidx_t qidx)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (base) {
        memcpy(q->elem_swap, base, q->elem_size);
    }
    return base;
}

static 
zaddr_t zqueue_swap_2_elem(zqueue_t *q, zqidx_t qidx)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (base) {
        memcpy(base, q->elem_swap, q->elem_size);
    }
    return base;
}

static
int32_t zqueue_safe_cmp(zq_cmp_func_t func, zaddr_t base1, zaddr_t base2)
{
    if (base1 && base2) {
        return func(base1, base2);
    } else if (base1) {
        return 1;
    } else if (base2) {
        return -1;
    } else {
        return 0;
    }
}

zaddr_t zqueue_find_first_match(zqueue_t *q, zaddr_t elem_base, zq_cmp_func_t func)

{
    zqidx_t qidx;
    zcount_t count = zqueue_get_count(q);

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zqueue_get_elem_base(q, qidx);
        if ( 0 == zqueue_safe_cmp(func, base, elem_base) )
        {
            return base;
        }
    }

    return 0;
}

zaddr_t zqueue_pop_first_match(zqueue_t *q, zaddr_t cmp_base, zq_cmp_func_t func)
{
    zaddr_t front = zqueue_get_front_base(q);
    zaddr_t base = zqueue_find_first_match(q, cmp_base, func);

    if (front && base) {
        zmem_swap(front, base, q->elem_size, 1);
        return zqueue_pop_front(q);
    }

    return 0;
}

int zqueue_elem_cmp(zqueue_t *q, zq_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);

    return zqueue_safe_cmp(func, base, elem_base);
}

int zqueue_elem_cmp_itnl(zqueue_t *q, zq_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2)
{
    zaddr_t base1 = zqueue_get_elem_base(q, qidx_1);
    zaddr_t base2 = zqueue_get_elem_base(q, qidx_2);

    return zqueue_safe_cmp(func, base1, base2);
}

static
void zqueue_quick_sort_iter(zqueue_t *q, zq_cmp_func_t func, zqidx_t start, zqidx_t end)
{
    zqidx_t left = start;
    zqidx_t right = end;

    if (start>=end) {
        return;
    }

    zqueue_elem_2_swap(q, right);

    while (left!=right)
    {
        while(left<right && zqueue_elem_cmp(q, func, left, q->elem_swap) <= 0) {
            ++ left;
        }
        zqueue_set_elem_val_itnl(q, right, left);   // first bigger to right

        while(left<right && zqueue_elem_cmp(q, func, right, q->elem_swap) >= 0) {
            -- right;
        }
        zqueue_set_elem_val_itnl(q, left, right);   // first smaller to left
    }

    zqueue_swap_2_elem(q, right);

    zqueue_quick_sort_iter(q, func, start, left-1);    // smaller part
    zqueue_quick_sort_iter(q, func, left+1, end);      // bigger part
}

void zqueue_quick_sort(zqueue_t *q, zq_cmp_func_t func)
{
    zcount_t count = zqueue_get_count(q);
    if (count > 2) {
        zqueue_quick_sort_iter(q, func, 0, count-1);
    }
}

#define ZQUEUE_QUICK_SORT_ITER(type_t, q, start, end)  \
        zqueue_quick_sort_iter_##type_t(q, start, end)

#define ZQUEUE_QUICK_SORT_ITER_DEFINE(type_t)           \
static void zqueue_quick_sort_iter_##type_t             \
(                                                       \
    type_t *q,                                          \
    zqidx_t start,                                     \
    zqidx_t end                                        \
)                                                       \
{                                                       \
    zqidx_t left = start;                              \
    zqidx_t right = end;                               \
    type_t   threshold;                                 \
                                                        \
    if (start>=end) {                                   \
        return;                                         \
    }                                                   \
                                                        \
    threshold = q[right];                               \
                                                        \
    while (left!=right)                                 \
    {                                                   \
        while(left<right && q[left] <= threshold) {     \
            ++ left;                                    \
        }                                               \
        q[right] = q[left];                             \
                                                        \
        while(left<right && q[right] >= threshold) {    \
            -- right;                                   \
        }                                               \
        q[left] = q[right];                             \
    }                                                   \
                                                        \
    q[right] = threshold;                               \
                                                        \
    zqueue_quick_sort_iter_##type_t(q, start, left-1);  \
    zqueue_quick_sort_iter_##type_t(q, left+1, end);    \
}

ZQUEUE_QUICK_SORT_ITER_DEFINE(int32_t)
void zqueue_quick_sort_i32(zqueue_t *q)
{
    zcount_t count = zqueue_get_count(q);
    if (count > 2) {
        ZQUEUE_QUICK_SORT_ITER(int32_t, q->elem_array, 0, count-1);
    }
}

ZQUEUE_QUICK_SORT_ITER_DEFINE(uint32_t)
void zqueue_quick_sort_u32(zqueue_t *q)
{
    zcount_t count = zqueue_get_count(q);
    if (count > 2) {
        ZQUEUE_QUICK_SORT_ITER(uint32_t, q->elem_array, 0, count-1);
    }
}


void zqueue_print_info(zqueue_t *q, const char *q_name)
{
    xprint("<zqueue> %s: count=%d, space=%d, depth=%d\n", 
        q_name, 
        zqueue_get_count(q),
        zqueue_get_space(q),
        q->depth);
}

void zqueue_print(zqueue_t *q, const char *q_name, zq_print_func_t func,
                const char *delimiters, const char *terminator)
{
    zqidx_t qidx;
    zcount_t count = zqueue_get_count(q);

    zqueue_print_info(q, q_name);

    if (func==0) { 
        xerr("<zqueue> Invalid print function!\n");
        return; 
    }

    xprint(" [", q_name);
    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zqueue_get_elem_base(q, qidx);
        func( qidx, base );
        xprint("%s", (qidx < count-1) ? delimiters : "");
    }
    xprint("]%s", terminator);

    return;
}
