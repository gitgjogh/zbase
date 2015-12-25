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

#include "zlist.h"
#include "sim_log.h"


#define     ZLIST_NEAREST_BIDX(q, addr) \
        ((((char *)(addr))-((char *)(q->elem_array))) / (q->elem_size))

static zbidx_t  zlist_qidx_2_bidx_in_buf(zlist_t *zl, zqidx_t qidx);
static zbidx_t  zlist_qidx_2_bidx_in_use(zlist_t *zl, zqidx_t qidx);
static zqidx_t  zlist_bidx_2_qidx_in_buf(zlist_t *zl, zbidx_t bidx);
static zqidx_t  zlist_bidx_2_qidx_in_use(zlist_t *zl, zbidx_t bidx);
static int      zlist_is_bidx_in_buf(zlist_t *zl, zqidx_t bidx);
static int      zlist_is_bidx_in_use(zlist_t *zl, zqidx_t bidx);
static zaddr_t  zlist_bidx_2_base_in_buf(zlist_t *zl, zbidx_t bidx);
static zaddr_t  zlist_bidx_2_base_in_use(zlist_t *zl, zbidx_t bidx);
static zqidx_t  zlist_base_2_bidx_in_buf(zlist_t *zl, zaddr_t elem_base);
static zqidx_t  zlist_base_2_bidx_in_use(zlist_t *zl, zaddr_t elem_base);
static zqidx_t  zlist_addr_2_bidx_in_buf(zlist_t *zl, zaddr_t addr);
static zqidx_t  zlist_addr_2_bidx_in_use(zlist_t *zl, zaddr_t addr);
#define         zlist_get_elem_bidx     zlist_qidx_2_bidx_in_use


/* buffer attach would reset the entire context */
zcount_t zlist_buf_attach(zlist_t *zl, zaddr_t buf, uint32_t elem_size, uint32_t depth)
{
    zqidx_t  qidx;
    zbidx_t *qidx_2_bidx = malloc(depth * sizeof(zbidx_t));

    if (!qidx_2_bidx) {
        xerr("<zlist>  %s() failed\n", __FUNCTION__);
        return 0;
    }
    
    zl->depth = depth;
    zl->count = 0;
    zl->elem_size = elem_size;
    zl->qidx_2_bidx = qidx_2_bidx;
    zl->elem_array = buf;
    
    zl->b_allocated = 0;
    zl->b_allow_realloc = 0;

    for (qidx=0; qidx<(int)depth; ++qidx) { 
        qidx_2_bidx[qidx] = qidx;
    }

    return depth;
}

void zlist_buf_detach(zlist_t *zl)
{
    SIM_FREEP(zl->qidx_2_bidx);
    if (!zl->b_allocated) {
        memset(zl, 0, sizeof(zl));
    }
}

zaddr_t zlist_buf_malloc(zlist_t *zl, uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zaddr_t buf = malloc( elem_size * depth );
    if (buf) {
        zcount_t count = zlist_buf_attach(zl, buf, elem_size, depth);
        if (count) {
            zl->b_allocated = 1;
            zl->b_allow_realloc = b_allow_realloc;
            return buf;
        } else {
            SIM_FREEP(buf);
        }
    }
    xerr("%s() failed!\n", __FUNCTION__);
    return 0;
}

zaddr_t zlist_buf_realloc(zlist_t *zl, uint32_t depth, int b_allow_realloc)
{
    zcount_t count = zlist_get_count(zl);
    if (zl->b_allocated && zl->b_allow_realloc) {
        if (depth < count) {
            xerr("data drop is not allowed in %s()!\n", __FUNCTION__);
            return 0;
        }

        /* We are using bidx rather than elem_base. So the map between
           qidx and elem_base won't change after realloc() */
        zbidx_t *bidxq = realloc(zl->qidx_2_bidx, depth * sizeof(zbidx_t));
        zaddr_t  elemq = realloc(zl->elem_array, depth * zl->elem_size);
        
        if (elemq && elemq) {
            zqidx_t qidx;
            for (qidx=zl->depth; qidx<(int)depth; ++qidx) { 
                bidxq[qidx] = qidx;
            }
            zl->depth = depth;
            zl->count = (count <= depth) ? count : depth;
            zl->qidx_2_bidx = bidxq;
            zl->elem_array = elemq;
            zl->b_allocated = 1;
            zl->b_allow_realloc = b_allow_realloc;
            return elemq;
        } else {
            /* Realloc failture leads to no expansion, but no harm to existing
               data. The entire list don't need to be destroied. */
            //SIM_FREEP(bidxq);
            //SIM_FREEP(elemq);
            xerr("%s() failed!\n", __FUNCTION__);
        }
    }
    return 0;
}

/**
 * Enlarge list buffer.
 * In case of reallocation failure, the old buf is kept unchanged.
 * 
 * @param additional_count  the count of elem to be allocated
 *                          in addition to zlist_get_count()
 * @return new space after buf grow. 
 */
zspace_t zlist_buf_grow(zlist_t *zl, uint32_t additional_count)
{
    if (zl->b_allocated && zl->b_allow_realloc) {
        zcount_t old_depth = zlist_get_depth(zl);
        zcount_t new_depth = additional_count + zlist_get_count(zl);
        if (new_depth > old_depth) {
            zaddr_t new_addr = zlist_buf_realloc(zl, new_depth, 1);
            if (!new_addr) {
                xerr("%s() failed!\n", __FUNCTION__);
            }
        }
    }
    return zlist_get_space(zl);
}

void zlist_buf_free(zlist_t *zl)
{
    SIM_FREEP(zl->qidx_2_bidx);
    if (zl->b_allocated && zl->elem_array) {
        SIM_FREEP(zl->elem_array);
    }
}


zlist_t *zlist_malloc(uint32_t elem_size, uint32_t depth, int b_allow_realloc)
{
    zlist_t *zl = malloc(sizeof(zlist_t));
    if (zl) {
        zaddr_t base = zlist_buf_malloc(zl, elem_size, depth, b_allow_realloc);
        if (base) {
            return zl;
        } else {
            SIM_FREEP(zl);
        }
    }
    xerr("<zlist>  %s failed\n", __FUNCTION__);
    return 0;
}

zlist_t* zlist_malloc_s(uint32_t elem_size, uint32_t depth)
{
    return zlist_malloc(elem_size, depth, 0);
}

zlist_t* zlist_malloc_d(uint32_t elem_size, uint32_t depth)
{
    return zlist_malloc(elem_size, depth, 1);
}

void zlist_free(zlist_t *zl)
{
    if (zl)  { 
        zlist_buf_free(zl);
        free(zl);
    }
}

zcount_t zlist_get_depth(zlist_t *zl)
{
    return (zl->depth);
}

zcount_t zlist_get_count(zlist_t *zl)
{
    return (zl->count);
}

zspace_t zlist_get_space(zlist_t *zl)
{
    return (zl->depth - zl->count);
}

int zlist_is_qidx_in_buf(zlist_t *zl, zqidx_t qidx)
{
    return (0<=qidx && qidx < zl->depth);
}

int zlist_is_qidx_in_use(zlist_t *zl, zqidx_t qidx)
{
    return (0<=qidx && qidx < zl->count);
}

zaddr_t zlist_qidx_2_base_in_buf(zlist_t *zl, zqidx_t qidx)
{
    if (zlist_is_qidx_in_buf(zl, qidx)) {
        return ZLIST_ELEM_BASE(zl, zl->qidx_2_bidx[qidx]);
    }
    return 0;
}

zaddr_t zlist_qidx_2_base_in_use(zlist_t *zl, zqidx_t qidx)
{
    if (zlist_is_qidx_in_use(zl, qidx)) {
        return ZLIST_ELEM_BASE(zl, zl->qidx_2_bidx[qidx]);
    }
    return 0;

}

zqidx_t zlist_base_2_qidx_in_buf(zlist_t *zl, zaddr_t elem_base)
{
    zbidx_t bidx = zlist_base_2_bidx_in_buf(zl, elem_base);
    return zlist_bidx_2_qidx_in_buf(zl, bidx);
}

zqidx_t zlist_base_2_qidx_in_use(zlist_t *zl, zaddr_t elem_base)
{
    zbidx_t bidx = zlist_base_2_bidx_in_buf(zl, elem_base);
    return zlist_bidx_2_qidx_in_use(zl, bidx);
}

zqidx_t zlist_addr_2_qidx_in_buf(zlist_t *zl, zaddr_t addr)
{
    zbidx_t bidx = zlist_addr_2_bidx_in_buf(zl, addr);
    return zlist_bidx_2_qidx_in_buf(zl, bidx);
}

zqidx_t zlist_addr_2_qidx_in_use(zlist_t *zl, zaddr_t addr)
{
    zbidx_t bidx = zlist_addr_2_bidx_in_buf(zl, addr);
    return zlist_bidx_2_qidx_in_use(zl, bidx);
}


static
zqidx_t zlist_base_2_bidx_in_use(zlist_t *zl, zaddr_t elem_base)
{
    zbidx_t bidx = zlist_base_2_bidx_in_buf(zl, elem_base);
    return zlist_bidx_2_qidx_in_use(zl, bidx) >= 0 ? bidx : -1;
}

static
zqidx_t zlist_base_2_bidx_in_buf(zlist_t *zl, zaddr_t elem_base)
{
    if (zlist_is_addr_in_buf(zl, elem_base)) {
        zbidx_t bidx = ZLIST_NEAREST_BIDX(zl, elem_base);
        if (ZLIST_ELEM_BASE(zl, bidx) == elem_base) {
            return bidx;
        }
    }
    return -1;
}

static
zqidx_t zlist_addr_2_bidx_in_buf(zlist_t *zl, zaddr_t addr)
{
    if (zlist_is_addr_in_buf(zl, addr)) {
        return ZLIST_NEAREST_BIDX(zl, addr);
    }
    return -1;
}

static
zqidx_t zlist_addr_2_bidx_in_use(zlist_t *zl, zaddr_t addr)
{
    zbidx_t bidx = zlist_addr_2_bidx_in_buf(zl, addr);
    return zlist_bidx_2_qidx_in_use(zl, bidx) >= 0 ? bidx : -1;
}

int zlist_is_addr_in_buf(zlist_t *zl, zaddr_t addr)
{
    return (zl->elem_array <= addr && addr < ZLIST_ELEM_BASE(zl, zl->depth));
}

int zlist_is_addr_in_use(zlist_t *zl, zaddr_t addr)
{
    zbidx_t bidx = zlist_addr_2_bidx_in_buf(zl, addr);
    return zlist_bidx_2_qidx_in_use(zl, bidx) >= 0;
}

int zlist_is_elem_base_in_buf(zlist_t *zl, zaddr_t elem_base)
{
    return zlist_base_2_qidx_in_buf(zl, elem_base) >= 0;
}

int zlist_is_elem_base_in_use(zlist_t *zl, zaddr_t elem_base)
{
    return zlist_base_2_qidx_in_use(zl, elem_base) >= 0;
}


static
zbidx_t zlist_qidx_2_bidx_in_buf(zlist_t *zl, zqidx_t qidx)
{
    return zlist_is_qidx_in_buf(zl, qidx) ? zl->qidx_2_bidx[qidx] : -1;
}

static
zbidx_t zlist_qidx_2_bidx_in_use(zlist_t *zl, zqidx_t qidx)
{
    return zlist_is_qidx_in_use(zl, qidx) ? zl->qidx_2_bidx[qidx] : -1;
}

static
zqidx_t zlist_bidx_2_qidx_in_buf(zlist_t *zl, zqidx_t bidx)
{
    if (zlist_is_bidx_in_buf(zl, bidx)) {
        zqidx_t qidx = 0;
        for (qidx=0; qidx < zl->depth; ++qidx) {
            if (zl->qidx_2_bidx[qidx] == bidx) {
                return qidx;
            }
        }
    }
    return -1;
}

static
zqidx_t zlist_bidx_2_qidx_in_use(zlist_t *zl, zqidx_t bidx)
{
    if (zlist_is_bidx_in_buf(zl, bidx)) {
        zqidx_t qidx = 0;
        for (qidx=0; qidx < zl->count; ++qidx) {
            if (zl->qidx_2_bidx[qidx] == bidx) {
                return qidx;
            }
        }
    }
    return -1;
}

static
int zlist_is_bidx_in_buf(zlist_t *zl, zqidx_t bidx)
{
    return (0<=bidx && bidx < zl->depth);
}

static
int zlist_is_bidx_in_use(zlist_t *zl, zqidx_t bidx)
{
    return zlist_bidx_2_qidx_in_use(zl, bidx) >= 0;
}

static
zaddr_t zlist_bidx_2_base_in_buf(zlist_t *zl, zbidx_t bidx)
{
    return zlist_is_bidx_in_buf(zl, bidx) ? ZLIST_ELEM_BASE(zl, bidx) : 0;
}

static
zaddr_t zlist_bidx_2_base_in_use(zlist_t *zl, zbidx_t bidx)
{
    return zlist_bidx_2_qidx_in_use(zl, bidx) >= 0 ? ZLIST_ELEM_BASE(zl, bidx) : 0;
}

zaddr_t zlist_get_front_base(zlist_t *zl)
{
    return zlist_get_elem_base(zl, 0);
}

zaddr_t zlist_get_back_base(zlist_t *zl)
{
    return zlist_get_elem_base(zl, zl->count - 1);
}

zaddr_t zlist_set_elem_val(zlist_t *zl, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zlist_get_elem_base(zl, qidx);
    if (base && elem_base) {
        memcpy(base, elem_base, zl->elem_size);
        return base;
    }
    return 0;
}

/** internal copy
 */
zaddr_t zlist_set_elem_val_itnl(zlist_t *zl, zqidx_t dst_idx, zqidx_t src_idx)
{
    zaddr_t dst_base = zlist_get_elem_base(zl, dst_idx);
    zaddr_t src_base = zlist_get_elem_base(zl, src_idx);

    if (dst_base && src_base) {
        memcpy(dst_base, src_base, zl->elem_size);
        return dst_base;
    }
    return 0;
}

zaddr_t zlist_pop_elem(zlist_t *zl, zqidx_t qidx)
{
    zaddr_t base = zlist_get_elem_base(zl, qidx);

    if (base) {
        zmem_swap_near_block(&zl->qidx_2_bidx[qidx], sizeof(zqidx_t), 
                              1, zl->count-qidx-1);
        zl->count -= 1;
    }

    return base;
}

zaddr_t zlist_pop_front(zlist_t *zl)
{
    return zlist_pop_elem(zl, 0);
}

zaddr_t zlist_pop_back(zlist_t *zl)
{
    return zlist_pop_elem(zl, zl->count - 1);
}

zaddr_t zlist_insert_elem(zlist_t *zl, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t  base  = 0;
    zspace_t space = zlist_buf_grow(zl, 1);
    zcount_t count = zlist_get_count(zl);

    if (0<=qidx && qidx<=count && space>=1) 
    {
        zmem_swap_near_block(&zl->qidx_2_bidx[qidx], sizeof(zqidx_t), 
                              zl->count-qidx, 1);
        zl->count += 1;

        base = zlist_get_elem_base(zl, qidx);
        if (elem_base) {
            memcpy(base, elem_base, zl->elem_size);
        }

        return base;
    }

    return 0;
}

zaddr_t zlist_push_back(zlist_t *zl, zaddr_t elem_base)
{
    return zlist_insert_elem(zl, zl->count, elem_base);
}

zcount_t zlist_insert_null_elems(
                    zlist_t *dst, zqidx_t insert_before, 
                    zcount_t insert_count)
{
    zspace_t dst_space = zlist_buf_grow(dst, insert_count);
    zcount_t dst_count = zlist_get_count(dst);

    if (0<=insert_before && insert_before<=dst_count && dst_space>=insert_count) 
    {
        zaddr_t  dst_base  = &dst->qidx_2_bidx[insert_before];
        zmem_swap_near_block(dst_base, dst->elem_size, 
                             dst->count - insert_before, 
                             insert_count);
        dst->count += insert_count;
        return insert_count;
    }

    return 0;
}

zcount_t  zlist_delete_multi_elems(
                    zlist_t *dst, zqidx_t delete_from, 
                    zcount_t delete_count)
{
    zspace_t dst_space = zlist_get_space(dst);
    zcount_t dst_count = zlist_get_count(dst);

    if (0<=delete_from && delete_from<=dst_count && dst_count>=delete_count) 
    {
        zaddr_t  dst_base  = &dst->qidx_2_bidx[delete_from];
        zmem_swap_near_block(dst_base, dst->elem_size, 
            delete_count, 
            dst_count - delete_from - delete_count);
        dst->count -= delete_count;
        return delete_count;
    }

    return 0;
}

zcount_t zlist_insert_some_of_others(
                    zlist_t *dst, zqidx_t insert_at, 
                    zlist_t *src, zqidx_t src_start, 
                    zcount_t insert_count)
{
    zcount_t real_insert = zlist_insert_null_elems(dst, insert_at, insert_count);

    if (real_insert > 0) {
        if (src) {
            zqidx_t   qidx;
            for (qidx=0; qidx<insert_count; ++qidx) {
                zaddr_t src_base = zlist_get_elem_base(src, src_start+qidx);
                zaddr_t dst_base = zlist_get_elem_base(dst, insert_at+qidx);

                if (dst_base && src_base) {
                    memcpy(dst_base, src_base, dst->elem_size);
                }
            }
        }
    }

    return real_insert;
}

zcount_t zlist_push_back_some_of_others(zlist_t *dst, zlist_t *src, 
                                        zqidx_t  src_start, 
                                        zcount_t merge_count)
{
    return zlist_insert_some_of_others(dst, zlist_get_count(dst), 
                              src, src_start, merge_count);
}

zcount_t zlist_insert_all_of_others(zlist_t *dst, zqidx_t insert_at, zlist_t *src)
{
    return zlist_insert_some_of_others(dst, insert_at, 
                              src, 0, zlist_get_count(src));
}

zcount_t zlist_push_back_all_of_others(zlist_t *dst, zlist_t *src)
{
    return zlist_insert_some_of_others(dst, zlist_get_count(dst), 
                              src, 0, zlist_get_count(src));
}

static
int32_t zlist_safe_cmp(zl_cmp_func_t func, zaddr_t base1, zaddr_t base2)
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

static 
zqidx_t  zlist_find_first_match_idx(zlist_t *zl, 
                                    zaddr_t elem_base, 
                                    zl_cmp_func_t func)

{
    zqidx_t qidx;
    zcount_t count = zlist_get_count(zl);

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zlist_get_elem_base(zl, qidx);
        if ( 0 == zlist_safe_cmp(func, base, elem_base) )
        {
            return qidx;
        }
    }

    return ZERRIDX;
}

zaddr_t zlist_find_first_match(zlist_t *zl, zaddr_t cmp_base, zl_cmp_func_t func)

{
    zqidx_t qidx = zlist_find_first_match_idx(zl, cmp_base, func);
    
    return zlist_get_elem_base(zl, qidx);
}

zaddr_t zlist_pop_first_match(zlist_t *zl, zaddr_t cmp_base, zl_cmp_func_t func)
{
    zqidx_t qidx = zlist_find_first_match_idx(zl, cmp_base, func);

    return zlist_pop_elem(zl, qidx);
}

int zlist_elem_cmp(zlist_t *zl, zl_cmp_func_t func, zqidx_t qidx, zaddr_t elem_base)
{
    zaddr_t base = zlist_get_elem_base(zl, qidx);

    return zlist_safe_cmp(func, base, elem_base);
}

int zlist_elem_cmp_itnl(zlist_t *zl, zl_cmp_func_t func, zqidx_t qidx_1, zqidx_t qidx_2)
{
    zaddr_t base1 = zlist_get_elem_base(zl, qidx_1);
    zaddr_t base2 = zlist_get_elem_base(zl, qidx_2);

    return zlist_safe_cmp(func, base1, base2);
}

static
void zlist_quick_sort_iter(zlist_t *zl, zl_cmp_func_t func, zqidx_t start, zqidx_t end)
{
    zqidx_t left = start;
    zqidx_t right = end;
    zbidx_t threshold_bidx;
    zaddr_t threshold_base;

    if (start>=end) {
        return;
    }

    threshold_bidx = zlist_get_elem_bidx(zl, right);
    threshold_base = zlist_get_elem_base(zl, right);

    while (left!=right)
    {
        while(left<right && zlist_elem_cmp(zl, func, left, threshold_base) <= 0) {
            ++ left;
        }
        zl->qidx_2_bidx[right] = zl->qidx_2_bidx[left];   // first bigger to right

        while(left<right && zlist_elem_cmp(zl, func, right, threshold_base) >= 0) {
            -- right;
        }
        zl->qidx_2_bidx[left] = zl->qidx_2_bidx[right];   // first smaller to left
    }

    zl->qidx_2_bidx[right] = threshold_bidx;

    zlist_quick_sort_iter(zl, func, start, left-1);    // smaller part
    zlist_quick_sort_iter(zl, func, left+1, end);      // bigger part
}

void zlist_quick_sort(zlist_t *zl, zl_cmp_func_t func)
{
    zcount_t count = zlist_get_count(zl);
    if (count > 2) {
        zlist_quick_sort_iter(zl, func, 0, count-1);
    }
}

void zlist_print_info(zlist_t *zl, const char *zl_name)
{
    xprint("<zlist> %s: count=%d, space=%d, depth=%d\n", 
        zl_name, 
        zlist_get_count(zl),
        zlist_get_space(zl),
        zl->depth);
}

void zlist_print(zlist_t *zl, const char *zl_name, zl_print_func_t func,
                const char *delimiters, const char *terminator)
{
    zqidx_t qidx;
    zcount_t count = zlist_get_count(zl);

    zlist_print_info(zl, zl_name);

    if (func==0) { 
        xerr("<zlist> Invalid print function!\n");
        return; 
    }

    xprint(" [", zl_name);
    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zlist_get_elem_base(zl, qidx);
        func( qidx, base );
        xprint("%s", (qidx < count-1) ? delimiters : "");
    }
    xprint("]%s", terminator);

    return;
}
