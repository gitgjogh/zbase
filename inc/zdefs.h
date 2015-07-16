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

#ifndef ZDEFS_H_
#define ZDEFS_H_

#include "sim_utils.h"

typedef     void*           zaddr_t;
typedef     void*           zfunc_t;
typedef     void*           zfile_t;

typedef     int32_t         zcount_t;
typedef     int32_t         zspace_t;           //<! just for readability 
typedef     int32_t         zqidx_t;            //<! logic idx for queue/list/array/...
typedef     int32_t         zbidx_t;            //<! buffer idx
#define     ZERRIDX         (-1) 

#define zmem_swap               mem_swap
#define zmem_swap_near_block    mem_swap_near_block

#endif //#ifndef ZDEFS_H_
