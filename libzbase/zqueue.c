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


static zbidx_t  zq_op_push_front_upd(zqueue_t *q);
static zbidx_t  zq_op_pop_front_upd(zqueue_t *q);
static zbidx_t  zq_op_qidx_2_bidx(zqueue_t *q, zqidx_t qidx);
static zqidx_t  zq_op_bidx_2_qidx(zqueue_t *q, zbidx_t bidx);
static zaddr_t  zq_op_qidx_2_base(zqueue_t *q, zbidx_t qidx);
static zqidx_t  zqueue_addr_2_bidx_in_buf(zqueue_t *q, zaddr_t elem_base, int is_base);

static zbidx_t zq_op_push_front_upd(zqueue_t *q)
{
    q->count += 1;
    q->start -= 1;
    if (q->start < 0) {
        q->start = q->depth - 1;
    }
    return q->start;
}

static zbidx_t zq_op_pop_front_upd(zqueue_t *q)
{
    q->count -= 1;
    q->start += 1;
    if (q->start >= q->depth) {
        q->start = 0;
    }
    return q->start;
}

/* you must assert zqueue_is_bidx_in_buf(q, bidx) */
static zqidx_t zq_op_bidx_2_qidx(zqueue_t *q, zbidx_t bidx)
{
    zqidx_t qidx = bidx - q->start;
    qidx += (qidx < 0 ? q->depth : 0);
    return qidx;
}

/* you must assert zqueue_is_qidx_in_buf(q, qidx) */
static zbidx_t zq_op_qidx_2_bidx(zqueue_t *q, zqidx_t qidx)
{
    zbidx_t bidx = qidx + q->start;
    bidx -= (bidx >= q->depth ? q->depth : 0);
    return bidx;
}

/* you must assert zqueue_is_qidx_in_buf(q, qidx) */
static zaddr_t zq_op_qidx_2_base(zqueue_t *q, zbidx_t qidx)
{
    zbidx_t bidx = zq_op_qidx_2_bidx(q, bidx);
    return ZQUEUE_ELEM_BASE(q, bidx);
}

#define     ZQUEUE_NEAREST_BIDX(q, addr) \
        ((((char *)(addr))-((char *)(q->elem_array))) / (q->elem_size))

static
zqidx_t zqueue_addr_2_bidx_in_buf(zqueue_t *q, zaddr_t elem_base, int is_base)
{
    if (zqueue_is_addr_in_buf(q, elem_base)) {
        zbidx_t bidx  = ZQUEUE_NEAREST_BIDX(q, elem_base);
        if ((!is_base) || (ZQUEUE_ELEM_BASE(q, bidx) == elem_base)) {
            return bidx;
        }
    }
    return -1;
}


/* buffer attach would reset the entire context */
zcount_t zqueue_buf_attach(zqueue_t *q, zaddr_t buf, uint32_t elem_size, uint32_t depth)
{
    q->depth = depth;
    q->count = 0;
    q->elem_size = elem_size;
    q->elem_array = buf;
    q->b_allocated = 0;
    q->b_allow_realloc = 0;

    return depth;
}

void zqueue_buf_detach(zqueue_t *q)
{
    if (!q->b_allocated) {
        zqueue_buf_attach(q, 0, 0, 0);
    }
}

zaddr_t zqueue_buf_malloc(zqueue_t *q, uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zaddr_t buf = malloc( elem_size * depth );
    if (buf) {
        zqueue_buf_attach(q, buf, elem_size, depth);
        q->b_allocated = 1;
        q->b_allow_realloc = b_allow_realloc;
        return buf;
    }
    xerr("%s() failed!\n", __FUNCTION__);
    return 0;
}

zaddr_t zqueue_buf_realloc(zqueue_t *q, uint32_t depth, int b_allow_realloc)
{
    zcount_t count = zqueue_get_count(q);
    if (q->b_allocated && q->b_allow_realloc) {
        if (depth < count) {
            xerr("data drop is not allowed in %s()!\n", __FUNCTION__);
            return 0;
        }
        
        zaddr_t buf = realloc(q->elem_array, q->elem_size * depth);
        if (buf) {
            zqueue_t _old_q = *q;
            zqueue_t *old_q = &_old_q;

            q->depth = depth;
            q->count = (q->count <= depth) ? q->count : depth;
            q->elem_array = buf;
            q->b_allow_realloc = b_allow_realloc;

            /* move the wrapped area */
            zqidx_t mv_idx = old_q->depth - old_q->start;
            for ( ; mv_idx < old_q->count; ++ mv_idx) {
                zaddr_t new_base = zqueue_get_elem(q, mv_idx);
                zaddr_t old_base = zqueue_get_elem(old_q, mv_idx);
                memcpy(new_base, old_base, sizeof(zqueue_t));
            }
                
            return buf;
        }
        xerr("%s() failed!\n", __FUNCTION__);
    }
    return 0;
}

/**
 * Enlarge queue buffer.
 * In case of reallocation failure, the old buf is kept unchanged.
 * 
 * @param additional_count  the count of elem to be allocated
 *                          in addition to zqueue_get_count()
 * @return new space after buf grow. 
 */
zspace_t zqueue_buf_grow(zqueue_t *q, uint32_t additional_count)
{
    if (q->b_allocated && q->b_allow_realloc) {
        zcount_t old_depth = zqueue_get_depth(q);
        zcount_t new_depth = additional_count + zqueue_get_count(q);
        if (new_depth > old_depth) {
            zaddr_t new_addr = zqueue_buf_realloc(q, new_depth, 1);
            if (!new_addr) {
                xerr("%s() failed!\n", __FUNCTION__);
            }
        }
    }
    return zqueue_get_space(q);
}

void zqueue_buf_free(zqueue_t *q)
{
    if (q->b_allocated && q->elem_array) {
        SIM_FREEP(q->elem_array);
    }
}

zqueue_t *zqueue_malloc(uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zaddr_t q = malloc( sizeof(zqueue_t) );
    if (!q) {
        xerr("%s() failed!\n", __FUNCTION__);
    } else {
        zaddr_t base = zqueue_buf_malloc(q, elem_size, depth, b_allow_realloc);
        if (!base) {
            SIM_FREEP(q);
        }
    }
    return q;
}

zqueue_t* zqueue_malloc_s(uint32_t elem_size, uint32_t depth)
{
    return zqueue_malloc(elem_size, depth, 0);
}

zqueue_t* zqueue_malloc_d(uint32_t elem_size, uint32_t depth)
{
    return zqueue_malloc(elem_size, depth, 1);
}

void zqueue_memzero(zqueue_t *q) 
{
    if (q && q->elem_array) {
        memset(q->elem_array, 0, q->elem_size * q->depth);
    }
}

void zqueue_clear(zqueue_t *q) 
{
    if (q) {
        q->count = 0;
    }
}

void zqueue_free(zqueue_t *q)
{
    if (q) {
        if (q->b_allocated) { 
            zqueue_buf_free(q);
        } else {
            zqueue_buf_detach(q);
        }
        free(q); 
    }
}

zcount_t zqueue_get_depth(zqueue_t *q)
{
    return (q->depth);
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
    if (zqueue_is_qidx_in_use(q, qidx)) {
        return zq_op_qidx_2_base(q, qidx);
    } 
    return 0;
}

zaddr_t zqueue_qidx_2_base_in_buf(zqueue_t *q, zqidx_t qidx)
{
    if (zqueue_is_qidx_in_buf(q, qidx)) {
        return zq_op_qidx_2_base(q, qidx);
    } 
    return 0;
}

int zqueue_is_qidx_in_buf(zqueue_t *q, zqidx_t qidx)
{
    return (0<=qidx && qidx < q->depth);
}

int zqueue_is_qidx_in_use(zqueue_t *q, zqidx_t qidx)
{
    return (0<=qidx && qidx < q->count);
}

int zqueue_is_addr_in_buf(zqueue_t *q, zaddr_t addr)
{
    return (q->elem_array <= addr && addr < ZQUEUE_ELEM_BASE(q, q->depth));
}

zqidx_t zqueue_base_2_qidx_in_buf(zqueue_t *q, zaddr_t elem_base)
{
    zbidx_t bidx = zqueue_addr_2_bidx_in_buf(q, elem_base, 1);
    return bidx>=0 ? zq_op_bidx_2_qidx(q, bidx) : -1;
}

zqidx_t zqueue_base_2_qidx_in_use(zqueue_t *q, zaddr_t elem_base)
{
    zbidx_t qidx = zqueue_base_2_qidx_in_buf(q, elem_base);
    return zqueue_is_qidx_in_use(q, qidx) ? qidx : -1;
}

zqidx_t zqueue_addr_2_qidx_in_buf(zqueue_t *q, zaddr_t addr)
{
    zbidx_t bidx = zqueue_addr_2_bidx_in_buf(q, addr, 0);
    return bidx>=0 ? zq_op_bidx_2_qidx(q, bidx) : -1;
}

zqidx_t zqueue_addr_2_qidx_in_use(zqueue_t *q, zaddr_t addr)
{
    zbidx_t qidx = zqueue_addr_2_qidx_in_buf(q, addr);
    return zqueue_is_qidx_in_use(q, qidx) ? qidx : -1;
}

int zqueue_is_addr_in_use(zqueue_t *q, zaddr_t addr)
{
    zbidx_t qidx = zqueue_base_2_qidx_in_buf(q, addr);
    return zqueue_is_qidx_in_use(q, qidx);
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

zcount_t zqueue_pop_front(zqueue_t *q, zaddr_t dst_base)
{
    zaddr_t base = zqueue_qidx_2_base_in_use(q, 0);
    if (base) {
        zq_op_pop_front_upd(q);
        if (dst_base) {
            memcpy(dst_base, base, sizeof(q->elem_size);
        }
        return 1;
    }
    return 0;
}

zcount_t zqueue_pop_back(zqueue_t *q, zaddr_t dst_base)
{
    zaddr_t base = zqueue_qidx_2_base_in_use(q, q->count - 1);
    if (base) {
        q->count -= 1;
        if (dst_base) {
            memcpy(dst_base, base, sizeof(q->elem_size);
        }
        return 1;
    }
    return 0;
}

zaddr_t zqueue_push_front(zqueue_t *q, zaddr_t elem_base)
{
    zspace_t space = zqueue_buf_grow(q, 1);
    if (space > 0) {
        zq_op_push_front_upd(q);
        zaddr_t base = zqueue_get_front(q);
        if (elem_base) {
            memcpy(base, elem_base, sizeof(q->elem_size));
        }
        return base;
    }
    return 0;
}

zaddr_t zqueue_push_back(zqueue_t *q, zaddr_t elem_base)
{
    zspace_t space = zqueue_buf_grow(q, 1);
    if (space > 0) {
        q->count += 1;
        zaddr_t base = zqueue_get_back(q);
        if (elem_base) {
            memcpy(base, elem_base, sizeof(q->elem_size));
        }
        return base;
    }
    return 0;
}

zcount_t zqueue_push_back_some_of_others(zqueue_t *dst, zqueue_t *src, 
                                   zqidx_t src_start, zcount_t merge_count)
{
    zcount_t src_count = zqueue_get_count(src);
    
    if (0<=src_start && src_start < src_count) {
        zspace_t dst_space = zqueue_buf_grow(dst, merge_count);
        zqidx_t  src_end = src_start + merge_count;
        if (dst_space>=merge_count) {
            for ( ; src_start < src_end; ++src_start) {
                zqueue_push_back(dst, zqueue_get_elem_base(src, src_start));
            }
        }
    }
}

zcount_t zqueue_push_back_all_of_others(zqueue_t *dst, zqueue_t *src)
{
    return zqueue_push_back_some_of_others(dst, src, 0, zqueue_get_count(src));
}

zcount_t zqueue_pop_elem(zqueue_t *q, zqidx_t qidx, zaddr_t dst_base)
{
    zaddr_t src_base = zqueue_get_elem_base(q, qidx);
    if (src_base) {
        if (dst_base) {
            memcpy(dst_base, src_base, sizeof(q->elem_size);
        }
        if (qidx > q->count / 2) {
            for ( ; qidx < q->count - 1; ++ qidx) {
                zaddr_t base0 = zqueue_get_elem_base(q, qidx);
                zaddr_t base1 = zqueue_get_elem_base(q, qidx + 1);
                memcpy(base0, base1, sizeof(q->elem_size);
            }
            -- q->count;
        } else {
            for ( ; qidx > 0; -- qidx) {
                zaddr_t base0 = zqueue_get_elem_base(q, qidx);
                zaddr_t base1 = zqueue_get_elem_base(q, qidx - 1);
                memcpy(base0, base1, sizeof(q->elem_size);
                zq_op_pop_front_upd(q);
            }
        }
        
        return 1;
    }

    return 0;
}

zaddr_t zqueue_insert_elem(zqueue_t *q, zqidx_t qidx, zaddr_t src_base)
{
    zspace_t space = zqueue_buf_grow(q, 1);
    zcount_t count = zqueue_get_count(q);

    if (0<=qidx && qidx<=count && space>=1)  
    {
        int i;
        if (qidx > q->count / 2) {
            ++ q->count;
            for (i=q->count-1; i>qidx; --i) {
                zaddr_t base0 = zqueue_get_elem_base(q, i);
                zaddr_t base1 = zqueue_get_elem_base(q, i-1);
                memcpy(base0, base1, sizeof(q->elem_size);
            }
            
        } else {
            zq_op_push_front_upd(q)
            for (i=0; i<=qidx; ++i) {
                zaddr_t base0 = zqueue_get_elem_base(q, i);
                zaddr_t base1 = zqueue_get_elem_base(q, i + 1);
                memcpy(base0, base1, sizeof(q->elem_size);
            }
        }
        
        zaddr_t dst_base  = zqueue_qidx_2_base_in_buf(q, qidx+1);
        if (src_base) {
            memcpy(dst_base, src_base, sizeof(q->elem_size);
        }
        return dst_base;
    }

    return 0;
}

zaddr_t zqueue_set_elem_val(zqueue_t *q, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (base && elem_base) {
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
    if (q->elem_swap && base) {
        memcpy(q->elem_swap, base, q->elem_size);
    }
    return base;
}

static 
zaddr_t zqueue_swap_2_elem(zqueue_t *q, zqidx_t qidx)
{
    zaddr_t base = zqueue_get_elem_base(q, qidx);
    if (q->elem_swap && base) {
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

zqidx_t zqueue_find_first_match_qidx(zqueue_t *q, zaddr_t elem_base, zq_cmp_func_t func)
{
    zqidx_t qidx;
    zcount_t count = zqueue_get_count(q);

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zqueue_get_elem_base(q, qidx);
        if ( 0 == zqueue_safe_cmp(func, base, elem_base) )
        {
            return qidx;
        }
    }

    return -1;
}

zaddr_t zqueue_find_first_match(zqueue_t *q, zaddr_t elem_base, zq_cmp_func_t func)
{
    zqidx_t qidx = zqueue_find_first_match_qidx(q, elem_base, func);
    if(qidx > 0) {
        return zqueue_get_elem_base(q, qidx);
    }

    return 0;
}

zcount_t zqueue_pop_first_match(zqueue_t *q, zaddr_t cmp_base, zq_cmp_func_t func, zaddr_t dst_base)
{
    zqidx_t qidx = zqueue_find_first_match_qidx(q, elem_base, func);
    if(qidx > 0) {
        return zqueue_pop_elem(q, qidx, dst_base);
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
        long long static_swap[8];
        if (q->elem_size > sizeof(static_swap)) {
            q->elem_swap = malloc(q->elem_size);
        } else {
            q->elem_swap = static_swap;
        }
        
        if (q->elem_swap) {
            zqueue_quick_sort_iter(q, func, 0, count-1);
            if (q->elem_size > sizeof(static_swap)) {
                SIM_FREEP(q->elem_swap);
            }
        } else {
            xerr("%s() failed!\n", __FUNCTION__);
        }
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

    if (q_name) {
        zqueue_print_info(q, q_name);
    }

    if (func==0) { 
        xerr("<zqueue> Invalid print function!\n");
        return; 
    }

    xprint(" [");
    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zqueue_get_elem_base(q, qidx);
        func( qidx, base );
        xprint("%s", (qidx < count-1) ? delimiters : "");
    }
    xprint("]%s", terminator);

    return;
}
