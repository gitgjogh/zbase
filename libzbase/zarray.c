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

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "zarray.h"
#include "sim_log.h"


static zaddr_t  zarray_elem_2_swap(zarray_t *za, zqidx_t qidx);
static zaddr_t  zarray_swap_2_elem(zarray_t *za, zqidx_t qidx);
static int32_t  zarray_safe_cmp(za_cmp_func_t func, zaddr_t base1, zaddr_t base2);
static void     zarray_quick_sort_iter(zarray_t *za, za_cmp_func_t func, 
                                       zqidx_t start, zqidx_t end);



/* buffer attach would reset the entire context */
zcount_t zarray_buf_attach(zarray_t *za, zaddr_t buf, uint32_t elem_size, uint32_t depth)
{
    za->depth = depth;
    za->count = 0;
    za->elem_size = elem_size;
    za->elem_array = buf;
    za->b_allocated = 0;
    za->b_allow_realloc = 0;

    return depth;
}

void zarray_buf_detach(zarray_t *za)
{
    if (!za->b_allocated) {
        zarray_buf_attach(za, 0, 0, 0);
    }
}

zaddr_t zarray_buf_malloc(zarray_t *za, uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zaddr_t buf = malloc( elem_size * depth );
    if (buf) {
        zarray_buf_attach(za, buf, elem_size, depth);
        za->b_allocated = 1;
        za->b_allow_realloc = b_allow_realloc;
        return buf;
    }
    xerr("%s() failed!\n", __FUNCTION__);
    return 0;
}

zaddr_t zarray_buf_realloc(zarray_t *za, uint32_t depth, int b_allow_realloc)
{
    zcount_t count = zarray_get_count(za);
    if (za->b_allocated && za->b_allow_realloc) {
        if (depth < count) {
            xerr("data drop is not allowed in %s()!\n", __FUNCTION__);
            return 0;
        }
        
        zaddr_t buf = realloc(za->elem_array, za->elem_size * depth);
        if (buf) {
            za->depth = depth;
            za->count = (za->count <= depth) ? za->count : depth;
            za->elem_array = buf;
            za->b_allow_realloc = b_allow_realloc;
            return buf;
        }
        xerr("%s() failed!\n", __FUNCTION__);
    }
    return 0;
}

/**
 * Enlarge array buffer.
 * In case of reallocation failure, the old buf is kept unchanged.
 * 
 * @param additional_count  the count of elem to be allocated
 *                          in addition to zarray_get_count()
 * @return new space after buf grow. 
 */
zspace_t zarray_buf_grow(zarray_t *za, uint32_t additional_count)
{
    if (za->b_allocated && za->b_allow_realloc) {
        zcount_t old_depth = zarray_get_depth(za);
        zcount_t new_depth = additional_count + zarray_get_count(za);
        if (new_depth > old_depth) {
            zaddr_t new_addr = zarray_buf_realloc(za, new_depth, 1);
            if (!new_addr) {
                xerr("%s() failed!\n", __FUNCTION__);
            }
        }
    }
    return zarray_get_space(za);
}

void zarray_buf_free(zarray_t *za)
{
    if (za->b_allocated && za->elem_array) {
        SIM_FREEP(za->elem_array);
    }
}

zarray_t *zarray_malloc(uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zaddr_t za = malloc( sizeof(zarray_t) );
    if (!za) {
        xerr("%s() failed!\n", __FUNCTION__);
    } else {
        zaddr_t base = zarray_buf_malloc(za, elem_size, depth, b_allow_realloc);
        if (!base) {
            SIM_FREEP(za);
        }
    }
    return za;
}

zarray_t* zarray_malloc_s(uint32_t elem_size, uint32_t depth)
{
    return zarray_malloc(elem_size, depth, 0);
}

zarray_t* zarray_malloc_d(uint32_t elem_size, uint32_t depth)
{
    return zarray_malloc(elem_size, depth, 1);
}

void zarray_memzero(zarray_t *za) 
{
    if (za && za->elem_array) {
        memset(za->elem_array, 0, za->elem_size * za->depth);
    }
}

void zarray_clear(zarray_t *za) 
{
    if (za) {
        za->count = 0;
    }
}

void zarray_free(zarray_t *za)
{
    if (za) {
        if (za->b_allocated) { 
            zarray_buf_free(za);
        } else {
            zarray_buf_detach(za);
        }
        free(za); 
    }
}

zcount_t zarray_get_depth(zarray_t *za)
{
    return (za->depth);
}

zcount_t zarray_get_count(zarray_t *za)
{
    return (za->count);
}

zspace_t zarray_get_space(zarray_t *za)
{
    return za->depth - za->count;
}

zaddr_t zarray_qidx_2_base_in_use(zarray_t *za, zqidx_t qidx)
{
    return zarray_is_qdix_in_use(za, qidx) ? ZARRAY_ELEM_BASE(za, qidx) : 0;
}

zaddr_t zarray_qidx_2_base_in_buf(zarray_t *za, zqidx_t qidx)
{
    return zarray_is_qdix_in_buf(za, qidx) ? ZARRAY_ELEM_BASE(za, qidx) : 0;
}

int zarray_is_qdix_in_buf(zarray_t *za, zqidx_t qidx)
{
    return (0<=qidx && qidx < za->depth);
}

int zarray_is_qdix_in_use(zarray_t *za, zqidx_t qidx)
{
    return (0<=qidx && qidx < za->count);
}

#define     ZARRAY_NEAREST_BIDX(za, addr) \
        ((((char *)(addr))-((char *)(za->elem_array))) / (za->elem_size))

zqidx_t zarray_base_2_qidx_in_buf(zarray_t *za, zaddr_t elem_base)
{
    if (zarray_is_addr_in_buf(za, elem_base)) {
        zbidx_t bidx  = ZARRAY_NEAREST_BIDX(za, elem_base);
        if (ZARRAY_ELEM_BASE(za, bidx) == elem_base) {
            return bidx;
        }
    }
    return -1;
}

zqidx_t zarray_base_2_qidx_in_use(zarray_t *za, zaddr_t elem_base)
{
    zbidx_t qidx = zarray_base_2_qidx_in_buf(za, elem_base);
    return zarray_is_qdix_in_use(za, qidx) ? qidx : -1;
}

zqidx_t zarray_addr_2_qidx_in_buf(zarray_t *za, zaddr_t addr)
{
    return zarray_is_addr_in_buf(za, addr) ? ZARRAY_NEAREST_BIDX(za, addr) : -1;
}

zqidx_t zarray_addr_2_qidx_in_use(zarray_t *za, zaddr_t addr)
{
    return zarray_is_addr_in_use(za, addr) ? ZARRAY_NEAREST_BIDX(za, addr) : -1;
}

int zarray_is_addr_in_buf(zarray_t *za, zaddr_t addr)
{
    return (za->elem_array <= addr && addr < ZARRAY_ELEM_BASE(za, za->depth));
}

int zarray_is_addr_in_use(zarray_t *za, zaddr_t addr)
{
    return (za->elem_array <= addr && addr < ZARRAY_ELEM_BASE(za, za->count));
}

int zarray_is_elem_base_in_buf(zarray_t *za, zaddr_t elem_base)
{
    return zarray_base_2_qidx_in_buf(za, elem_base) >= 0;
}

int zarray_is_elem_base_in_use(zarray_t *za, zaddr_t elem_base)
{
    return zarray_base_2_qidx_in_use(za, elem_base) >= 0;
}

zaddr_t zarray_get_front_base(zarray_t *za)
{
    return zarray_qidx_2_base_in_use(za, 0);
}

zaddr_t zarray_get_back_base(zarray_t *za)
{
    return zarray_qidx_2_base_in_use(za, za->count - 1);
}

zaddr_t zarray_get_last_base(zarray_t *za)
{
    return zarray_qidx_2_base_in_buf(za, za->count);
}

za_iter_t   zarray_iter(zarray_t *za)
{
    za_iter_t iter = {za, 0};
    return iter;
}

zaddr_t   zarray_front(za_iter_t *iter)
{
    return zarray_get_elem_base(iter->za, iter->iter_idx=0);
}
zaddr_t   zarray_next(za_iter_t *iter)
{
    return zarray_get_elem_base(iter->za, ++ iter->iter_idx);
}
zaddr_t   zarray_back(za_iter_t *iter)
{
    zcount_t count = zarray_get_count(iter->za);
    return zarray_get_elem_base(iter->za, iter->iter_idx=count-1);
}
zaddr_t   zarray_prev(za_iter_t *iter)
{
    return zarray_get_elem_base(iter->za, -- iter->iter_idx);
}

zcount_t zarray_pop_elem(zarray_t *za, zqidx_t qidx, zaddr_t dst_base)
{
    zaddr_t base = zarray_get_elem_base(za, qidx);
    if (base) {
        if (dst_base) {
            memcpy(dst_base, base, za->elem_size);
        }

        /* swap to back */
        zmem_swap_near_block(base, za->elem_size, 1, za->count-qidx-1);
        za->count -= 1;

        return 1;
    }

    return 0;
}

zcount_t zarray_pop_front(zarray_t *za, zaddr_t dst_base)
{
    return zarray_pop_elem(za, 0, dst_base);
}

zcount_t zarray_pop_back(zarray_t *za, zaddr_t dst_base)
{
    return zarray_pop_elem(za, za->count - 1, dst_base);
}

zaddr_t zarray_insert_elem(zarray_t *za, zqidx_t qidx, zaddr_t elem_base)
{
    zspace_t space = zarray_buf_grow(za, 1);
    zcount_t count = zarray_get_count(za);

    if (0<=qidx && qidx<=count && space>=1)  
    {
        zaddr_t  base  = zarray_qidx_2_base_in_buf(za, qidx);
        zmem_swap_near_block(base, za->elem_size, za->count-qidx, 1);
        za->count += 1;

        if (elem_base) {
            memcpy(base, elem_base, za->elem_size);
        }

        return base;
    }

    return 0;
}

zaddr_t zarray_push_front(zarray_t *za, zaddr_t elem_base)
{
    return zarray_insert_elem(za, 0, elem_base);
}

zaddr_t zarray_push_back(zarray_t *za, zaddr_t elem_base)
{
    return zarray_insert_elem(za, za->count, elem_base);
}

zcount_t zarray_insert_null_elems(
                    zarray_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count)
{
    zspace_t dst_space = zarray_buf_grow(dst, insert_count);
    zcount_t dst_count = zarray_get_count(dst);
    
    if (0<=insert_before && insert_before<=dst_count && dst_space>=insert_count) 
    {
        zaddr_t  dst_base  = zarray_qidx_2_base_in_buf(dst, insert_before);
        zmem_swap_near_block(dst_base, dst->elem_size, 
                             dst->count - insert_before, 
                             insert_count);
        dst->count += insert_count;
        return insert_count;
    }

    return 0;
}

zcount_t  zarray_delete_multi_elems(
                    zarray_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count)
{
    zspace_t dst_space = zarray_get_space(dst);
    zcount_t dst_count = zarray_get_count(dst);
    zaddr_t  dst_base  = zarray_qidx_2_base_in_use(dst, delete_from);

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

zcount_t zarray_insert_some_of_others(
                    zarray_t *dst, zqidx_t insert_at,
                    zarray_t *src, zqidx_t src_start, 
                    zcount_t insert_count)
{
    zcount_t real_insert = zarray_insert_null_elems(dst, insert_at, insert_count);
    
    if (real_insert > 0) {
        if (src) {
            zqidx_t   qidx;
            for (qidx = 0; qidx<insert_count; ++qidx) {
                zaddr_t src_base = zarray_get_elem_base(src, src_start+qidx);
                zaddr_t dst_base = zarray_get_elem_base(dst, insert_at+qidx);

                if (dst_base && src_base) {
                    memcpy(dst_base, src_base, dst->elem_size);
                }
            }
        }
    }

    return real_insert;
}

zcount_t  zarray_insert_all_of_others(zarray_t *dst, zqidx_t insert_at, zarray_t *src)
{
    return zarray_insert_some_of_others(dst, insert_at, 
                               src, 0, zarray_get_count(src));
}

zcount_t  zarray_push_back_some_of_others(zarray_t *dst, zarray_t *src, 
                                   zqidx_t src_start, 
                                   zcount_t merge_count)
{
    return zarray_insert_some_of_others(dst, zarray_get_count(dst), 
                               src, src_start, merge_count);
}

zcount_t  zarray_push_back_all_of_others(zarray_t *dst, zarray_t *src)
{
    return zarray_insert_some_of_others(dst, zarray_get_count(dst), 
                               src, 0, zarray_get_count(src));
}

zaddr_t zarray_set_elem_val(zarray_t *za, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zarray_get_elem_base(za, qidx);
    if (qidx && elem_base) {
        memcpy(base, elem_base, za->elem_size);
        return base;
    }
    return 0;
}

zaddr_t zarray_set_elem_val_itnl(zarray_t *za, zqidx_t dst_idx, zqidx_t src_idx)
{
    zaddr_t dst_base = zarray_get_elem_base(za, dst_idx);
    zaddr_t src_base = zarray_get_elem_base(za, src_idx);

    if (dst_base && src_base) {
        memcpy(dst_base, src_base, za->elem_size);
        return dst_base;
    }
    return 0;
}

static 
zaddr_t zarray_elem_2_swap(zarray_t *za, zqidx_t qidx)
{
    zaddr_t base = zarray_get_elem_base(za, qidx);
    if (base) {
        memcpy(za->elem_swap, base, za->elem_size);
    }
    return base;
}

static 
zaddr_t zarray_swap_2_elem(zarray_t *za, zqidx_t qidx)
{
    zaddr_t base = zarray_get_elem_base(za, qidx);
    if (base) {
        memcpy(base, za->elem_swap, za->elem_size);
    }
    return base;
}

static
int32_t zarray_safe_cmp(za_cmp_func_t func, zaddr_t base1, zaddr_t base2)
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

zqidx_t zarray_find_first_match_qidx(zarray_t *za, zaddr_t elem_base, za_cmp_func_t func)
{
    zqidx_t qidx;
    zcount_t count = zarray_get_count(za);

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zarray_get_elem_base(za, qidx);
        if ( 0 == zarray_safe_cmp(func, base, elem_base) )
        {
            return qidx;
        }
    }

    return -1;
}

zaddr_t zarray_find_first_match(zarray_t *za, zaddr_t elem_base, za_cmp_func_t func)
{
    zqidx_t qidx = zarray_find_first_match_qidx(za, elem_base, func);
    if (qidx >= 0) {
        return ZARRAY_ELEM_BASE(za, qidx);
    }

    return 0;
}

zcount_t zarray_pop_first_match(zarray_t *za, zaddr_t cmp_base, za_cmp_func_t func, zaddr_t dst_base)
{
    zaddr_t qidx = zarray_find_first_match_qidx(za, cmp_base, func);

    if (qidx >= 0) {
        return zarray_pop_elem(za, qidx, dst_base);
    }

    return 0;
}

int zarray_elem_cmp(zarray_t *za, za_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zarray_get_elem_base(za, qidx);

    return zarray_safe_cmp(func, base, elem_base);
}

int zarray_elem_cmp_itnl(zarray_t *za, za_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2)
{
    zaddr_t base1 = zarray_get_elem_base(za, qidx_1);
    zaddr_t base2 = zarray_get_elem_base(za, qidx_2);

    return zarray_safe_cmp(func, base1, base2);
}

static
void zarray_quick_sort_iter(zarray_t *za, za_cmp_func_t func, zqidx_t start, zqidx_t end)
{
    zqidx_t left = start;
    zqidx_t right = end;

    if (start>=end) {
        return;
    }

    zarray_elem_2_swap(za, right);

    while (left!=right)
    {
        while(left<right && zarray_elem_cmp(za, func, left, za->elem_swap) <= 0) {
            ++ left;
        }
        zarray_set_elem_val_itnl(za, right, left);   // first bigger to right

        while(left<right && zarray_elem_cmp(za, func, right, za->elem_swap) >= 0) {
            -- right;
        }
        zarray_set_elem_val_itnl(za, left, right);   // first smaller to left
    }

    zarray_swap_2_elem(za, right);

    zarray_quick_sort_iter(za, func, start, left-1);    // smaller part
    zarray_quick_sort_iter(za, func, left+1, end);      // bigger part
}

void zarray_quick_sort(zarray_t *za, za_cmp_func_t func)
{
    zcount_t count = zarray_get_count(za);
    if (count > 2) {
        long long static_swap[8];
        if (za->elem_size > sizeof(static_swap)) {
            za->elem_swap = malloc(za->elem_size);
        } else {
            za->elem_swap = static_swap;
        }
        
        if (za->elem_swap) {
            zarray_quick_sort_iter(za, func, 0, count-1);
            if (za->elem_size > sizeof(static_swap)) {
                SIM_FREEP(za->elem_swap);
            }
        } else {
            xerr("%s() failed!\n", __FUNCTION__);
        }
    }
}

#define ZARRAY_QUICK_SORT_ITER(type_t, za, start, end)  \
        zarray_quick_sort_iter_##type_t(za, start, end)

#define ZARRAY_QUICK_SORT_ITER_DEFINE(type_t)           \
static void zarray_quick_sort_iter_##type_t             \
(                                                       \
    type_t *za,                                          \
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
    threshold = za[right];                               \
                                                        \
    while (left!=right)                                 \
    {                                                   \
        while(left<right && za[left] <= threshold) {     \
            ++ left;                                    \
        }                                               \
        za[right] = za[left];                             \
                                                        \
        while(left<right && za[right] >= threshold) {    \
            -- right;                                   \
        }                                               \
        za[left] = za[right];                             \
    }                                                   \
                                                        \
    za[right] = threshold;                               \
                                                        \
    zarray_quick_sort_iter_##type_t(za, start, left-1);  \
    zarray_quick_sort_iter_##type_t(za, left+1, end);    \
}

ZARRAY_QUICK_SORT_ITER_DEFINE(int32_t)
void zarray_quick_sort_i32(zarray_t *za)
{
    zcount_t count = zarray_get_count(za);
    if (count > 2) {
        ZARRAY_QUICK_SORT_ITER(int32_t, za->elem_array, 0, count-1);
    }
}

ZARRAY_QUICK_SORT_ITER_DEFINE(uint32_t)
void zarray_quick_sort_u32(zarray_t *za)
{
    zcount_t count = zarray_get_count(za);
    if (count > 2) {
        ZARRAY_QUICK_SORT_ITER(uint32_t, za->elem_array, 0, count-1);
    }
}


void zarray_print_info(zarray_t *za, const char *q_name)
{
    xprint("<zarray> %s: count=%d, space=%d, depth=%d\n", 
        q_name, 
        zarray_get_count(za),
        zarray_get_space(za),
        za->depth);
}

void zarray_print(zarray_t *za, const char *q_name, za_print_func_t func,
                const char *delimiters, const char *terminator)
{
    zqidx_t qidx;
    zcount_t count = zarray_get_count(za);

    if (q_name) {
        zarray_print_info(za, q_name);
    }

    if (func==0) { 
        xerr("<zarray> Invalid print function!\n");
        return; 
    }

    xprint(" [");
    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zarray_get_elem_base(za, qidx);
        func( qidx, base );
        xprint("%s", (qidx < count-1) ? delimiters : "");
    }
    xprint("]%s", terminator);

    return;
}
