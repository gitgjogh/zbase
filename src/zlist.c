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

#include "zutils.h"
#include "zlist.h"


#ifdef DEBUG
#define zlist_dbg printf
#else
#define zlist_dbg printf
#endif

zlist_t *zlist_malloc(uint32_t elem_size, uint32_t depth)
{
    zlist_t *zl = malloc(sizeof(zlist_t));
    zl_bidx_t *bidxq = malloc(depth * sizeof(zl_bidx_t));
    zl_addr_t  elemq = malloc(depth * elem_size);
    zl_idx_t qidx;

    if (!zl || !bidxq || !elemq) {
        zlist_dbg("@err>> %s failed\n", __FUNCTION__);
        if (bidxq) { free(bidxq); }
        if (elemq) { free(elemq); }
        if (zl) { free(zl); }
        return 0;
    }

    zl->depth = depth;
    zl->count = 0;
    zl->elem_size = elem_size;
    zl->bidx_array = bidxq;
    zl->elem_array = elemq;

    //! bidxq[qidx] = qidx
    for (qidx=0; qidx<(int)depth; ++qidx) { 
        bidxq[qidx] = qidx;
    }

    return zl;
}

zlist_t *zlist_realloc(zlist_t *zl, uint32_t depth) 
{
    if ((int)depth>zl->depth) 
    {
        zl_idx_t qidx;

        zl_bidx_t *bidxq = realloc(zl->bidx_array, depth * sizeof(zl_bidx_t));
        zl_addr_t  elemq = realloc(zl->elem_array, depth * zl->elem_size);

        //! bidxq[qidx] = qidx
        for (qidx=zl->depth; qidx<(int)depth; ++qidx) { 
            bidxq[qidx] = qidx;
        }
        zl->depth = depth;
    }

    return zl;
}

void zlist_free(zlist_t *zl)
{
    if (zl)  { 
        if (zl->bidx_array) { free(zl->bidx_array); }
        if (zl->elem_array) { free(zl->elem_array); }
        if (zl) { free(zl); }
    }
}

zl_count_t zlist_get_depth(zlist_t *zl)
{
    return (zl->depth);
}

zl_count_t zlist_get_count(zlist_t *zl)
{
    return (zl->count);
}

zl_space_t zlist_get_space(zlist_t *zl)
{
    return (zl->depth - zl->count);
}

zl_bidx_t zlist_get_elem_bidx(zlist_t *zl, zl_idx_t qidx)
{
    zl_count_t count = zlist_get_count(zl);

    return (0<=qidx && qidx < count) ? zl->bidx_array[qidx] : ZQ_ERR_IDX;
}

zl_addr_t zlist_get_elem_base(zlist_t *zl, zq_idx_t qidx)
{
    zl_bidx_t bidx = zlist_get_elem_bidx(zl, qidx);
    if (bidx >= 0) {
        return ZLIST_ELEM_BASE(zl, bidx);
    }
    return 0;
}

zl_addr_t zlist_get_front_base(zlist_t *zl)
{
    return zlist_get_elem_base(zl, 0);
}

zl_addr_t zlist_get_back_base(zlist_t *zl)
{
    return zlist_get_elem_base(zl, zl->count - 1);
}

zl_addr_t zlist_set_elem_val(zlist_t *zl, zl_idx_t qidx, zl_addr_t elem_base)
{
    zl_addr_t base = zlist_get_elem_base(zl, qidx);
    if (base && elem_base) {
        memcpy(base, elem_base, zl->elem_size);
        return base;
    }
    return 0;
}

zq_addr_t zlist_set_elem_val_itnl(zlist_t *zl, zl_idx_t dst_idx, zl_idx_t src_idx)
{
    zl_addr_t dst_base = zlist_get_elem_base(zl, dst_idx);
    zl_addr_t src_base = zlist_get_elem_base(zl, src_idx);

    if (dst_base && src_base) {
        memcpy(dst_base, src_base, zl->elem_size);
        return dst_base;
    }
    return 0;
}

zl_addr_t zlist_pop_elem(zlist_t *zl, zl_idx_t qidx)
{
    zl_addr_t base = zlist_get_elem_base(zl, qidx);

    if (base) {
        zmem_swap_near_block(&zl->bidx_array[qidx], sizeof(zl_idx_t), 
                              1, zl->count-qidx-1);
        zl->count -= 1;
    }

    return base;
}

zl_addr_t zlist_pop_front(zlist_t *zl)
{
    return zlist_pop_elem(zl, 0);
}

zl_addr_t zlist_pop_back(zlist_t *zl)
{
    return zlist_pop_elem(zl, zl->count - 1);
}

zl_addr_t zlist_insert_elem(zlist_t *zl, zl_idx_t qidx, zl_addr_t elem_base)
{
    zl_addr_t  base  = 0;
    zl_count_t count = zlist_get_count(zl);
    zl_space_t space = zlist_get_space(zl);

    if (0<=qidx && qidx<=count && space>=1) 
    {
        zmem_swap_near_block(&zl->bidx_array[qidx], sizeof(zl_idx_t), 
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

zl_addr_t zlist_push_back(zlist_t *zl, zl_addr_t elem_base)
{
    return zlist_insert_elem(zl, zl->count, elem_base);
}

zl_count_t zlist_insert_null_elems(
                    zlist_t *dst, zl_idx_t insert_before, 
                    zl_count_t insert_count)
{
    zl_space_t dst_space = zlist_get_space(dst);
    zl_count_t dst_count = zlist_get_count(dst);

    if (0<=insert_before && insert_before<=dst_count && dst_space>=insert_count) 
    {
        zq_addr_t  dst_base  = &dst->bidx_array[insert_before];
        zmem_swap_near_block(dst_base, dst->elem_size, 
                             dst->count - insert_before, 
                             insert_count);
        dst->count += insert_count;
        return insert_count;
    }

    return 0;
}

zl_count_t  zlist_delete_multi_elems(
                    zlist_t *dst, zl_idx_t delete_from, 
                    zl_count_t delete_count)
{
    zl_space_t dst_space = zlist_get_space(dst);
    zl_count_t dst_count = zlist_get_count(dst);

    if (0<=delete_from && delete_from<=dst_count && dst_count>=delete_count) 
    {
        zq_addr_t  dst_base  = &dst->bidx_array[delete_from];
        zmem_swap_near_block(dst_base, dst->elem_size, 
            delete_count, 
            dst_count - delete_from - delete_count);
        dst->count -= delete_count;
        return delete_count;
    }

    return 0;
}

zl_count_t zlist_insert_some_of_others(
                    zlist_t *dst, zl_idx_t insert_at, 
                    zlist_t *src, zl_idx_t src_start, 
                    zl_count_t insert_count)
{
    zl_count_t real_insert = zlist_insert_null_elems(dst, insert_at, insert_count);

    if (real_insert > 0) {
        if (src) {
            zq_idx_t   qidx;
            for (qidx=0; qidx<insert_count; ++qidx) {
                zl_addr_t src_base = zlist_get_elem_base(src, src_start+qidx);
                zl_addr_t dst_base = zlist_get_elem_base(dst, insert_at+qidx);

                if (dst_base && src_base) {
                    memcpy(dst_base, src_base, dst->elem_size);
                }
            }
        }
    }

    return real_insert;
}

zl_count_t zlist_push_back_some_of_others(zlist_t *dst, zlist_t *src, 
                                 zl_idx_t src_start, 
                                 zl_count_t merge_count)
{
    return zlist_insert_some_of_others(dst, zlist_get_count(dst), 
                              src, src_start, merge_count);
}

zl_count_t zlist_insert_all_of_others(zlist_t *dst, zl_idx_t insert_at, zlist_t *src)
{
    return zlist_insert_some_of_others(dst, insert_at, 
                              src, 0, zlist_get_count(src));
}

zl_count_t zlist_push_back_all_of_others(zlist_t *dst, zlist_t *src)
{
    return zlist_insert_some_of_others(dst, zlist_get_count(dst), 
                              src, 0, zlist_get_count(src));
}

static
int32_t zlist_safe_cmp(zl_cmp_func_t func, zl_addr_t base1, zl_addr_t base2)
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
zl_idx_t zlist_find_first_match_idx(zlist_t *zl, 
                                    zl_addr_t elem_base, 
                                    zl_cmp_func_t func)

{
    zl_idx_t qidx;
    zl_count_t count = zlist_get_count(zl);

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zl_addr_t base = zlist_get_elem_base(zl, qidx);
        if ( 0 == zlist_safe_cmp(func, base, elem_base) )
        {
            return qidx;
        }
    }

    return ZL_ERR_IDX;
}

zl_addr_t zlist_find_first_match(zlist_t *zl, zl_addr_t cmp_base, zl_cmp_func_t func)

{
    zl_idx_t qidx = zlist_find_first_match_idx(zl, cmp_base, func);
    
    return zlist_get_elem_base(zl, qidx);
}

zl_addr_t zlist_pop_first_match(zlist_t *zl, zl_addr_t cmp_base, zl_cmp_func_t func)
{
    zl_idx_t qidx = zlist_find_first_match_idx(zl, cmp_base, func);

    return zlist_pop_elem(zl, qidx);
}

int zlist_elem_cmp(zlist_t *zl, zl_cmp_func_t func, zl_idx_t qidx, zq_addr_t elem_base)
{
    zl_addr_t base = zlist_get_elem_base(zl, qidx);

    return zlist_safe_cmp(func, base, elem_base);
}

int zlist_elem_cmp_itnl(zlist_t *zl, zl_cmp_func_t func, zl_idx_t qidx_1, zl_idx_t qidx_2)
{
    zl_addr_t base1 = zlist_get_elem_base(zl, qidx_1);
    zl_addr_t base2 = zlist_get_elem_base(zl, qidx_2);

    return zlist_safe_cmp(func, base1, base2);
}

static
void zlist_quick_sort_iter(zlist_t *zl, zl_cmp_func_t func, zl_idx_t start, zl_idx_t end)
{
    zl_idx_t left = start;
    zl_idx_t right = end;
    zl_bidx_t threshold_bidx;
    zl_addr_t threshold_base;

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
        zl->bidx_array[right] = zl->bidx_array[left];   // first bigger to right

        while(left<right && zlist_elem_cmp(zl, func, right, threshold_base) >= 0) {
            -- right;
        }
        zl->bidx_array[left] = zl->bidx_array[right];   // first smaller to left
    }

    zl->bidx_array[right] = threshold_bidx;

    zlist_quick_sort_iter(zl, func, start, left-1);    // smaller part
    zlist_quick_sort_iter(zl, func, left+1, end);      // bigger part
}

void zlist_quick_sort(zlist_t *zl, zl_cmp_func_t func)
{
    zl_count_t count = zlist_get_count(zl);
    if (count > 2) {
        zlist_quick_sort_iter(zl, func, 0, count-1);
    }
}

void zlist_print_info(zlist_t *q, char *q_name)
{
    zlist_dbg("@zlist>> %s: count=%d, space=%d, depth=%d\n", 
        q_name, 
        zlist_get_count(q),
        zlist_get_space(q),
        q->depth);
}

void zlist_print(char *q_name, zlist_t *q, zq_print_func_t func)
{
    zq_idx_t qidx;
    zq_count_t count = zlist_get_count(q);

    zlist_print_info(q, q_name);

    if (func==0) { 
        zlist_dbg("@zlist>> Err: Invalid print function!\n");
        return; 
    }

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zq_addr_t base = zlist_get_elem_base(q, qidx);
        func( qidx, base );
    }

    return;
}


#ifdef ZLIST_TEST

#define     DEREF_I32(pi)       (*((int *)pi))
#define     SET_ITEM(qidx)         (item=qidx, &item)

static
int32_t cmp_func(zq_addr_t cmp_base, zq_addr_t elem_base)
{
    return DEREF_I32(cmp_base) - DEREF_I32(elem_base);
}

static
void print_func(zq_idx_t idx, zq_addr_t elem_base)
{
    zlist_dbg("@zlist>> [%8d] : %d\n", idx, DEREF_I32(elem_base));
}

int main(int argc, char** argv)
{
    int item, idx;
    zq_addr_t ret, popped;

    zlist_t *q1 = ZLIST_MALLOC(int, 10);
    zlist_t *q2 = ZLIST_MALLOC(int, 20);
    for (idx=1; idx<8; ++idx) {
        zlist_push_back(q1, SET_ITEM(10-idx));
        zlist_push_back(q2, SET_ITEM(10+idx));
    }
    zlist_print("init with 1~7, size=10", q1, print_func);

    ret = zlist_push_back(q1, SET_ITEM(8));    printf("\n push %d %s \n\n", item, ret ? "success" : "fail");   
    ret = zlist_push_back(q1, SET_ITEM(9));    printf("\n push %d %s \n\n", item, ret ? "success" : "fail");  
    ret = zlist_push_back(q1, SET_ITEM(10));   printf("\n push %d %s \n\n", item, ret ? "success" : "fail"); 

    zlist_print("push 8,9,10 ", q1, print_func);

    ret = zlist_push_back(q1, SET_ITEM(11));   printf("\n push %d %s \n\n", item, ret ? "success" : "fail"); 
    ret = zlist_push_back(q1, SET_ITEM(12));   printf("\n push %d %s \n\n", item, ret ? "success" : "fail"); 

    zlist_print("push 11,12", q1, print_func);

    popped = zlist_pop_front(q1);     
    popped ? printf("\n pop front = %d \n\n", DEREF_I32(popped)) : printf("\n pop fail \n\n");

    popped = zlist_pop_front(q1);     
    popped ? printf("\n pop front = %d \n\n", DEREF_I32(popped)) : printf("\n pop fail \n\n");

    zlist_print("pop front 2 items", q1, print_func);

    popped = zlist_pop_first_match(q1, SET_ITEM(7),  cmp_func);     
    printf("\n pop %d ", item);
    popped ?  printf("= %d", DEREF_I32(popped)) : printf("fail");
    printf("\n\n");

    zlist_print("pop first match 7", q1, print_func);

    popped = zlist_pop_first_match(q1, SET_ITEM(8),  cmp_func);     
    printf("\n pop %d ", item);
    popped ?  printf("= %d", DEREF_I32(popped)) : printf("fail");
    printf("\n\n");

    zlist_print("pop first match 8", q1, print_func);

    popped = zlist_pop_first_match(q1, SET_ITEM(11), cmp_func);     
    printf("\n pop %d ", item);
    popped ?  printf("= %d", DEREF_I32(popped)) : printf("fail");
    printf("\n\n");

    zlist_print("pop first match 11", q1, print_func);

    ret = zlist_insert_elem(q1, 0, SET_ITEM(11));     
    printf("\n push front %d ", item);
    ret ?  printf("= %d", DEREF_I32(ret)) : printf("fail");
    printf("\n\n");

    zlist_print("push 11 at front ", q1, print_func);

    zlist_quick_sort(q1, cmp_func);
    printf("\n\n");
    zlist_print("quick sort", q1, print_func);

    zlist_print("q2 before cat", q2, print_func);
    zlist_insert_all_of_others(q2, 4, q1);
    zlist_print("zlist_insert_list(q2, 4, q1)", q2, print_func);

    zlist_quick_sort(q2, cmp_func);
    printf("\n\n");
    zlist_print("q2 quick sort", q2, print_func);

    zlist_free(q1);
    zlist_free(q2);

    return 0;
}

#endif  // ZLIST_TEST