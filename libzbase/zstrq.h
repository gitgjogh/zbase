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

#ifndef ZSTRQ_H_
#define ZSTRQ_H_

#include "zdefs.h"
#include "zqueue.h"


typedef     char            zsq_char_t;
typedef     zsq_char_t*     zsq_ptr_t;


typedef struct z_string_queue
{
    zspace_t        buf_size;
    zcount_t        buf_used;
    zsq_char_t*     str_buf;

    zcount_t        numstr;
    zsq_ptr_t*      ptr_array;

    int             _b_fixed_size;
}zstrq_t;


zstrq_t*    zstrq_malloc(uint32_t size);
zstrq_t*    zstrq_realloc(zstrq_t *sq, uint32_t new_size);
void        zstrq_free(zstrq_t *sq);
zspace_t    zstrq_get_buf_size(zstrq_t *sq);
zcount_t    zstrq_get_str_count(zstrq_t *sq);       /** num of str in queue */
zspace_t    zstrq_get_buf_space(zstrq_t *sq);       /** size of usable character buf */

zsq_char_t* zstrq_get_str_base(zstrq_t *sq, zqidx_t qidx);    //<! 0<= qidx < count
uint32_t    zstrq_get_str_size(zstrq_t *sq, zqidx_t qidx);    //<! 0<= qidx < count

zaddr_t     zstrq_push_back(zstrq_t *sq,const zsq_char_t* str, uint32_t str_len);
zaddr_t     zstrq_pop_back(zstrq_t *sq, uint32_t *str_len);


/** @return the actually push backed str count */
zcount_t    zstrq_push_back_v(zstrq_t *sq, zcount_t n, 
                              const zsq_char_t* cstr, ...);

zcount_t    zstrq_push_back_multi(zstrq_t *dst, zstrq_t *src, 
                                  zqidx_t src_start, 
                                  zcount_t push_count);
zcount_t    zstrq_push_back_all(zstrq_t *dst, zstrq_t *src);

zcount_t    zstrq_push_back_list(zstrq_t *dst, 
                                 zsq_char_t *strlist, 
                                 zsq_char_t *delemiters);


typedef void  (*zsq_print_func_t)  (zqidx_t idx, zaddr_t elem_base);
void        zstrq_print(zstrq_t *sq, char *sq_name, zsq_print_func_t func,
                        const char *delimiters, const char *terminator);

#endif //ZSTRQ_H_
