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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "zutils.h"
#include "zstrq.h"

#ifdef DEBUG
#define zhash_dbg printf
#else
#define zhash_dbg printf
#endif

const static uint32_t g_zsq_page = (1<<16);
const static uint32_t g_zsq_max_size = (1<<24);


zstrq_t* zstrq_malloc(uint32_t size)
{
    zstrq_t *sq = calloc( 1, sizeof(zstrq_t) );
    if (!sq) {
        zhash_dbg("@err>> %s() failed!\n", __FUNCTION__);
        return 0;
    } 

    if (size>0) {
        sq->str_buf = malloc(sizeof(zsq_char_t) * size);
    }

    if (sq->str_buf) {
        sq->buf_size = size;
        sq->ptr_array = (zsq_ptr_t *)(sq->str_buf+size) - 1;
    } else {
        zhash_dbg("@zstrq>> warning: sizeof(str_buf)=0\n"); 
        return sq;
    }

    return sq;
}

zstrq_t* zstrq_realloc(zstrq_t *sq, uint32_t new_size)
{
    zqidx_t idx = 0;
    zsq_ptr_t *old_ptr_array = 0;
    zsq_ptr_t *new_ptr_array = 0;

    if (new_size > g_zsq_max_size) {
        zhash_dbg("@err>> new_size exceed max allowed size\n");
        return sq;
    }

    zhash_dbg("@zstrq>> zstrq_realloc(0x%x -> 0x%x)\n", sq->buf_size, new_size);

    if ((int)new_size > sq->buf_size) 
    {
        zsq_char_t *old_buf = sq->str_buf;
        zsq_char_t *new_buf = realloc(old_buf, sizeof(zsq_char_t) * new_size);
        if (!new_buf) {
            zhash_dbg("@err>> realloc str_buf failed\n");
            return sq;
        }

        if (sq->str_buf != new_buf) {
            if (sq->str_buf) {
                memcpy(new_buf, sq->str_buf, sq->buf_used);
                free(sq->str_buf);
            }
            sq->str_buf = new_buf;
        }
        sq->buf_size = new_size;

        // move ptr_array to the new end;
        old_ptr_array = sq->ptr_array;
        new_ptr_array = (zsq_ptr_t *)(new_buf+new_size) - 1;
        for (idx=0; idx<sq->numstr; ++idx) {
            new_ptr_array[-idx] = new_buf + (old_ptr_array[-idx] - old_buf);
        }
        sq->ptr_array = new_ptr_array;
    }

    return sq;
}

void zstrq_free(zstrq_t *sq)
{
    if (sq) {
        if (sq->str_buf) {
            free(sq->str_buf);
        }
        free (sq);
    }
}

zspace_t zstrq_get_buf_size(zstrq_t *sq)
{
    return sq->buf_size;
}

zcount_t zstrq_get_str_count(zstrq_t *sq)
{
    return sq->numstr;
}

zspace_t zstrq_get_buf_space(zstrq_t *sq)
{
    /* reserve one zsq_ptr_t space for future use */
    return sq->buf_size - sq->buf_used - sizeof(zsq_ptr_t) * (1 + sq->numstr);
}

static
zsq_char_t* zstrq_set_str_base(zstrq_t *sq, zqidx_t qidx, zsq_char_t *base)
{
    sq->ptr_array[-qidx] = base;
}

zsq_char_t* zstrq_get_str_base(zstrq_t *sq, zqidx_t qidx)
{
    if (0<=qidx && qidx <= sq->numstr) {
        zsq_char_t* str = sq->ptr_array[-qidx];
        return str;
    }

    return 0;
}

uint32_t    zstrq_get_str_size(zstrq_t *sq, zqidx_t qidx)
{
    if (0<=qidx && qidx <= sq->numstr) {
        zsq_char_t* str = sq->ptr_array[-qidx];
        return strlen(str);
    }

    return 0;
}

zaddr_t  zstrq_push_back(zstrq_t *sq, const zsq_char_t* str, uint32_t str_len)
{
    zsq_char_t  c = 0;
    zspace_t i = 0;
    zspace_t space = 0;
    zsq_char_t *dst =0;

    if (!str) {
        zhash_dbg("@err>> zstrq_push_back: null str\n");
        return 0;
    }

    space = zstrq_get_buf_space(sq) - 1;
    str_len = str_len ? str_len : strlen(str);
    if (!sq->_b_fixed_size && space<(int)str_len) {
        sq = zstrq_realloc(sq, sq->buf_size + g_zsq_page);
        space = zstrq_get_buf_space(sq) - 1;
    }

    dst = sq->str_buf + sq->buf_used;
    for (i=0; (c=str[i]) && i<(int)str_len && i<space; ++i) {
        dst[i] = c;
    } 
    if (i>=space && c!=0) {
        zhash_dbg("@err>> zstrq_push_back: str buf overflow\n");
        return 0;
    }

    dst[i] = 0;
    sq->buf_used += (i+1);
    sq->ptr_array[- sq->numstr] = dst;
    sq->numstr += 1;
    return dst; 
}

zaddr_t  zstrq_pop_back(zstrq_t *sq, uint32_t *str_len)
{
    if (sq->numstr>0) {
        zsq_char_t *base = sq->ptr_array[ 1 - sq->numstr ];
        sq->buf_used  = base - sq->str_buf;
        sq->numstr -= 1;
        if (str_len) {
            *str_len = strlen(base);
        }
        return base;
    }
    return 0;
}

zcount_t zstrq_push_back_v(zstrq_t *sq, zcount_t n, const zsq_char_t* cstr, ...)
{
    zcount_t i = 0;
    zsq_char_t *pstr;
    va_list ap;

    zaddr_t base = zstrq_push_back(sq, cstr, 0);
    if (!base) {
        return 0;
    }

    va_start(ap, cstr);
    for (i=1; i<n; ++i) {
        pstr = va_arg(ap, zsq_char_t*);
        base = zstrq_push_back(sq, pstr, 0);
        if (!base) {
            break;
        }
    }
    va_end(ap);

    return i;
}

zcount_t zstrq_push_back_multi(zstrq_t *dst, zstrq_t *src, 
                                  zqidx_t src_start, 
                                  zcount_t push_count)
{
    zqidx_t qidx = 0;
    zaddr_t src_base = 0;
    zaddr_t dst_base = 0;

    for (qidx=0; qidx<push_count; ++qidx) {
        src_base = zstrq_get_str_base(src, src_start+qidx);
        if (!src_base) {
            break;
        }

        dst_base = zstrq_push_back(dst, src_base, 0);
        if (!dst_base) {
            break;
        }
    }

    return qidx;
}

zcount_t zstrq_push_back_all (zstrq_t *dst, zstrq_t *src)
{
    zcount_t src_count = zstrq_get_str_count(src);

    return zstrq_push_back_multi(dst, src, 0, src_count);
}

zcount_t zstrq_push_back_list(zstrq_t *dst,
                                 zsq_char_t *key_list,
                                 zsq_char_t *delemiters)
{
    uint32_t pos, start, str_len;
    zcount_t ori_count = zstrq_get_str_count(dst);
    zaddr_t base = 0;

    for (pos = 0; ;pos = start+str_len) {
        str_len = get_token_pos(key_list, delemiters, pos, &start);
        if (str_len<=0) {
            break;
        }

        base = zstrq_push_back(dst, key_list+start, str_len);
        if (!base) {
            break;
        }
    }

    return zstrq_get_str_count(dst) - ori_count;
}


void zstrq_print(char *q_name, zstrq_t *sq, zsq_print_func_t func)
{
    zqidx_t qidx;
    zcount_t count = zstrq_get_str_count(sq);

    zhash_dbg("@zlist>> %s: buf_size=%d, count=%d, buf_used=%d+%d*4+4=%d, space=%d\n", 
        q_name, 
        sq->buf_size, 
        count,
        sq->buf_used,
        count,
        sq->buf_used + 4 * count + 4,
        zstrq_get_buf_space(sq)
        );

    if (func==0) { 
        zhash_dbg("@zlist>> Err: Invalid print function!\n");
        return; 
    }

    for (qidx = 0; qidx < count; ++ qidx)
    {
        zaddr_t base = zstrq_get_str_base(sq, qidx);
        func( qidx, base );
    }

    return;
}


#ifdef ZSTRQ_TEST

#define     DEREF_I32(pi)       (*((int *)pi))
#define     SET_ITEM(qidx)         (item=qidx, &item)

static
int32_t cmp_func(zaddr_t cmp_base, zaddr_t elem_base)
{
    return DEREF_I32(cmp_base) - DEREF_I32(elem_base);
}

static
void print_func(zqidx_t idx, zaddr_t elem_base)
{
    zhash_dbg("@zstrq>> [%8d] : %s\n", idx, elem_base);
}

int main(int argc, char** argv)
{
    zsq_char_t *str;
    zstrq_t *q1 = zstrq_malloc(0);
    zstrq_t *q2 = zstrq_malloc(0);
    
    zstrq_push_back_v(q1, 10, 
        "Don't","let","the","fear","for",
        "losing","keep","you","from","trying.");

    zstrq_push_back_list(q2, 
        "(You only live once,)(but if you do it right,)(once  is enough.)",
        " )(");

    zstrq_print("q1", q1, print_func);
    zstrq_print("q2", q2, print_func);

    zstrq_push_back_all(q1, q2);
    zstrq_print("q1", q1, print_func);

    str = zstrq_pop_back(q1, 0);
    printf("poped = %s\n", str ? str : "[failed]");
    zstrq_print("q1", q1, print_func);

    zstrq_free(q1);
    zstrq_free(q2);

    return 0;
}

#endif  // ZSTRQ_TEST