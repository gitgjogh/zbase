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

#ifndef ZOPT_H_
#define ZOPT_H_

#include "zhash.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef struct arg_iterator {
    int     argc;
    char  **argv;
    int     idx;
}zarg_iter_t;

zarg_iter_t zarg_iter(int argc, char **argv);
char*       zarg_curr(zarg_iter_t *iter);
int         zarg_curr_idx(zarg_iter_t *iter);
char*       zarg_curr_opt(zarg_iter_t *iter);       //<! if not opt ret 0
char*       zarg_curr_param(zarg_iter_t *iter);     //<! if opt ret 0
int         zarg_next(zarg_iter_t *iter);
int         zarg_err(zarg_iter_t *iter);
int         zarg_jump_prog(zarg_iter_t *iter);      /** jump program name */


typedef int (*zopt_func_t)(struct zhash_parser *opt, 
                           zarg_iter_t *iter, 
                           struct zopt_node *node);

typedef enum {
    ZOPT_TYPE_FLAG = 0,
    ZOPT_TYPE_INT,
    ZOPT_TYPE_STR,
    ZOPT_TYPE_EXT,
}zopt_type_t;

typedef struct zopt_node {
    ZHASH_COMMON;

    zh_type_t   sub_type;
    const char  *help;
    zopt_func_t func;
    void        *p_val;

    zopt_type_t enum_type;
    int32_t     enum_val;
}zopt_node_t;

typedef struct zhash_parser {
    zhash_t*        h;
    zarray_t*       access_stack;
}zopt_t;


zopt_t*     zopt_malloc(uint32_t depth_log2);
void        zopt_free(zopt_t *opt);

zaddr_t     zopt_add_node( zopt_t       *opt,
                           const char   *key,
                           zopt_func_t   func, 
                           void         *p_val, 
                           const char   *help);

zaddr_t     zopt_add_root(zopt_t *opt, const char *key, const char *help);
zaddr_t     zopt_get_root(zopt_t *opt);
zaddr_t     zopt_get_father(zopt_t *opt);

zaddr_t     zopt_start_group(zopt_t *opt, const char *key, const char *help);
zaddr_t     zopt_end_group(zopt_t *opt, const char *key);


int zopt_parse_argcv(zopt_t *opt, int argc, char *argv[]);
int zopt_print_help(zopt_t *opt, zopt_node_t *node);


/**
 *  build-in zopt_func_t
 */
int zopt_parse_node(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);
int zopt_parse_help(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);
int zopt_parse_flag(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);
int zopt_parse_int(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);
int zopt_parse_str(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);

int zopt_parse_enum(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node);
int zopt_create_enum(zopt_t *opt, zopt_node_t *node, const char* desc);
void zopt_print_enum(zopt_t *opt, zopt_node_t *node);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //#ifndef ZOPT_H_