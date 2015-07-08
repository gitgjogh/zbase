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

#ifndef ZUTILS_H_
#define ZUTILS_H_


#include "zdefs.h"


#define     ZWRAP_AROUND(depth, bidx)    (((bidx)>=(depth)) ? ((bidx)-(depth)) : (bidx))

void        zmem_swap(zaddr_t base1, zaddr_t base2, 
                      uint32_t elem_size, uint32_t cnt);

void        zmem_swap_near_block(zaddr_t  base, uint32_t elem_size, 
                                 uint32_t cnt1, uint32_t cnt2);

//<! @return strlen(token)
uint32_t    get_token_pos(const char* str, 
                          const char* delemiters,
                          uint32_t search_from,
                          uint32_t *stoken_start);

uint32_t    jump_front(const char* str, const char* jumpset);

#endif //#ifndef ZUTILS_H_