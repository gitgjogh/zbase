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
#include "zutils.h"


void zmem_swap32(void* base1, void* base2, uint32_t sz)
{
    uint32_t m, *p1 = base1, *p2 = base2;
    for ( ; sz>0; --sz, ++p1, ++p2) {
        m = *p1;
        *p1 = *p2;
        *p2 = m;
    }
}

void zmem_swap(void* base1, void* base2, uint32_t elem_size, uint32_t cnt)
{
    uint32_t sz = elem_size * cnt;

    if (((uint32_t)base1&3) || ((uint32_t)base2&3) || (sz&3)) 
    {
        char m, *p1 = base1, *p2 = base2;
        for ( ; sz>0; --sz, ++p1, ++p2) {
            m = *p1;
            *p1 = *p2;
            *p2 = m;
        }
    } else {
        zmem_swap32(base1, base2, sz/4);
    }
}

void zmem_swap_near_block_32(void* base, uint32_t sz, uint32_t shift)
{
    uint32_t i0 = 0;
    uint32_t i1 = 0;
    uint32_t *p = base;
    uint32_t tmp = p[i0];

    if (sz==0 || shift==0 || sz==shift) {
        return;
    }

    do {
        i0 = i1;
        i1 = ZWRAP_AROUND(sz, shift + i0);
        p[i0] = p[i1];
    } while (i1 != 0);
    p[i0] = tmp;
}

/**
 *  swap_pos(,,7,2) : 0123456 78 -> 78 0123456
 */
void zmem_swap_near_block(void* base, uint32_t elem_size, 
                          uint32_t cnt1,  uint32_t cnt2)
{
    uint32_t sz = elem_size * (cnt1 + cnt2); 
    uint32_t shift = elem_size * cnt1;

    if (cnt1==0 || cnt2==0) {
        return;
    }

    if (((uint32_t)base&3) || (sz&3) || (shift&3)) 
    {
        uint32_t i0 = 0;
        uint32_t i1 = 0;
        char *p = base;
        char tmp = p[i0];
        do {
            i0 = i1;
            i1 = ZWRAP_AROUND(sz, shift + i0);
            p[i0] = p[i1];
        } while (i1 != 0);
        p[i0] = tmp;
    }
    else {
        zmem_swap_near_block_32(base, sz/4, shift/4);
    }
}

uint32_t get_token_pos(const char* str, 
                       const char* delemiters,
                       uint32_t search_from,
                       uint32_t *stoken_start)
{
    uint32_t c, start, end;
    for (start = search_from; (c = str[start]); ++start) 
    {
        if ( !strchr(delemiters, c) ) {
            break;
        }
    }

    if (stoken_start) {
        *stoken_start = start;
    }

    for (end = start; (c = str[end]); ++end) 
    {
        if ( strchr(delemiters, c) ) {
            break;
        }
    }

    return end - start;
}