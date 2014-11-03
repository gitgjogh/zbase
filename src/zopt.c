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

#include "zutils.h"
#include "zhash.h"
//#include "zlog.h"
#include "zopt.h"

#ifdef DEBUG
#define cmdl_zlog printf
#else
#define cmdl_zlog printf
#endif

zarg_iter_t zarg_iter(int argc, char **argv)
{
    zarg_iter_t iter = { argc, argv, 0 };
    return      iter;
}

char *zarg_curr(zarg_iter_t *iter)
{
    int idx = iter->idx;
    return (0<=idx && idx<iter->argc) ? iter->argv[idx] : 0;
}

int zarg_curr_idx(zarg_iter_t *iter)
{
    return iter->idx;
}

char *zarg_curr_opt(zarg_iter_t *iter)
{
    char *arg = zarg_curr(iter);
    arg = (arg != 0 && arg[0] == '-') ? (arg+1) : 0;
    return arg;
}

char *zarg_curr_param(zarg_iter_t *iter)
{
    char *arg = zarg_curr(iter);
    arg = (arg != 0 && strchr("-()", arg[0])==NULL) ? arg : 0;
    return arg;
}

int zarg_next(zarg_iter_t *iter)
{
    return (++ iter->idx);
}

int zarg_jump_prog(zarg_iter_t *iter)
{
    return (++ iter->idx);
}

int zarg_err(zarg_iter_t *iter)
{
    return (- iter->idx);
}

zopt_t* zopt_malloc(uint32_t depth_log2)
{
    zopt_t *ps = calloc( 1, sizeof(zopt_t) );
    if (!ps) {
        cmdl_zlog("@err>> %s() failed!\n", __FUNCTION__);
        return 0;
    }
    
    ps->h = ZHASH_MALLOC(zopt_node_t, depth_log2);
    ps->access_stack = ZQUEUE_MALLOC(zopt_node_t*, 32);

    if (!ps->h || !ps->access_stack) {
        cmdl_zlog("@err>> %s() 1 failed!\n", __FUNCTION__);
        zopt_free(ps);
        return 0;
    }

    return ps;
}

void zopt_free(zopt_t *opt)
{
    if (opt) {
        if (opt->h) { free(opt->h); }
        if (opt->access_stack) { free(opt->access_stack); }
        free(opt);
    }
}

int zopt_print_help(zopt_t *opt, zopt_node_t *node)
{
    zq_idx_t node_count = 1;
    zopt_node_t *sub = 0;

    cmdl_zlog("    -%s : %s\n", node->key, node->help);
    if (node->enum_type) {
        zopt_print_enum(opt, node);
    }

    if (node->sub_type) 
    {
        zh_iter_t _iter = zhash_iter(opt->h), *iter = &_iter;

        for (sub=zhash_front(iter); sub!=0; sub=zhash_next(iter))
        {
            if (sub->type == node->sub_type) {
                node_count += zopt_print_help(opt, sub);
            }
        }
    }

    return node_count;
}

int zopt_parse_flag(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node)
{
    char *arg = zarg_curr_param(iter);
    *((int *)node->p_val) = 1;

    if ( arg && (strcmp(arg, "0") == 0 || strcmp(arg, "1") == 0) ) {
        *((int *)node->p_val) = (arg[0] == '1') ? 1 : 0;
        return zarg_next(iter);
    } 
    
    return zarg_curr_idx(iter);
}

int zopt_parse_int(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node)
{
    char *arg = zarg_curr_param(iter);

    if ( arg ) {
        *((int *)node->p_val) = atoi(arg);
        return zarg_next(iter);
    } else {
        cmdl_zlog("@err>> no param for `-%s`\n", node->key);
        *((int *)node->p_val) = 0;
        return zarg_err(iter);
    }
}

int zopt_parse_str(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node)
{
    char *arg = zarg_curr_param(iter);

    if ( arg ) {
        *((char **)node->p_val) = arg;
        return zarg_next(iter);
    } else {
        cmdl_zlog("@err>> no param for `-%s`\n", node->key);
        *((char **)node->p_val) = 0;
        return zarg_err(iter);
    }
}

int zopt_parse_help(zopt_t *opt, zarg_iter_t *argq, zopt_node_t *node)
{
    zopt_print_help(opt, zopt_get_root(opt));
    return 0;
}

void zopt_print_enum(zopt_t *opt, zopt_node_t *node)
{
    zopt_node_t *entry = 0;

    if (node->enum_type) {
        zh_iter_t _iter = zhash_iter(opt->h), *iter = &_iter;

        cmdl_zlog("\t\t");
        for (entry=zhash_front(iter); entry!=0; entry=zhash_next(iter))
        {
            if (entry->type == node->enum_type) {
                cmdl_zlog("%s:%d,", entry->key, entry->enum_val);
            }
        }
        cmdl_zlog("\n");
    }
}

int zopt_parse_enum(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node)
{
    char *arg = zarg_curr_param(iter);

    if ( arg ) {
        zopt_node_t *entry = zhash_get_node(opt->h, node->enum_type, arg, 0);
        if ( entry ) {
            *((int *)node->p_val) = entry->enum_val;
            return zarg_next(iter);
        } else {
            cmdl_zlog("@err>> `%s` is not valid value for `-%s`\n", arg, node->key);
            return zarg_err(iter);
        }
    } else {
        cmdl_zlog("@err>> no param for `-%s`\n", node->key);
        *((char **)node->p_val) = 0;
        return zarg_err(iter);
    }
}

int zopt_create_enum(zopt_t *opt, zopt_node_t *node, const char* desc)
{
    zh_type_t enum_type = node->enum_type = zhash_reg_new_type(opt->h);

    uint32_t pos, start, str_len, b_found;
    zopt_node_t *entry = 0;
    int32_t last_val = -1;
    uint32_t item_cnt = 0;
    uint32_t new_pos = 0;

    for (pos = start = 0;;) {
        str_len = get_token_pos(desc, " ,=", pos, &start);
        if (str_len<=0) {
            break;
        }
        pos = start + str_len;

        entry = zhash_touch_node(
                opt->h, enum_type, &desc[start], str_len, 1, &b_found);
        if (!entry) {
            break;
        }
        entry->enum_val = ++last_val;
        item_cnt += 1;

        // get '='
        str_len = get_token_pos(desc, " ,", pos, &start);
        if (str_len<=0 || desc[start] != '=') {
            continue;
        }

        // get 'int'
        str_len = get_token_pos(desc, " ,=", pos, &start);
        if (str_len<=0) {
            cmdl_zlog("@err>> no value after `=`\n");

            break;
        }
        pos = start + str_len;
        entry->enum_val = last_val = atoi( &desc[start] );
    }

    return item_cnt;
}

zh_addr_t zopt_add_root(zopt_t *opt, const char *key, const char *help)
{
    zh_type_t  type = zhash_reg_new_type(opt->h);
    zopt_node_t *root = zhash_touch_node(opt->h, type, key, 0, 1, 0);
    root->sub_type = zhash_reg_new_type(opt->h);
    root->help = help;
    root->func = zopt_parse_node;
    root->p_val = 0;
    zqueue_push_back(opt->access_stack, &root);
    return root;
}

zh_addr_t zopt_get_root(zopt_t *opt)
{
    zopt_node_t **root = zqueue_get_front_base(opt->access_stack);
    if (root) {
        return *root;
    }
    return 0;
}

zh_addr_t zopt_get_father(zopt_t *opt)
{
    zopt_node_t **father = zqueue_get_back_base(opt->access_stack);
    if (father) {
        return *father;
    }
    return 0;
}

zh_addr_t zopt_add_node(zopt_t       *opt, 
                        const char   *key,
                        zopt_func_t  func, 
                        void         *p_val, 
                        const char   *help)
{
    int b_found;
    zopt_node_t *father = zopt_get_father(opt);
    zopt_node_t *node = zhash_touch_node(opt->h, 
            father->sub_type, key, 0, 1, &b_found);

    if (node) {
        node->sub_type = 0;
        node->func  = func;
        node->p_val = p_val;
        node->help  = help;
        return node;
    }

    return 0;
}

zh_addr_t zopt_start_group(zopt_t *opt, const char *key, const char *help)
{
    int b_found;
    zopt_node_t *father = 0;
    zopt_node_t *node = 0;

    father = zopt_get_father(opt);
    if (!father) {
        cmdl_zlog("@err>> access statck empty\n");
        return 0;
    }

    node = zhash_touch_node(opt->h, father->sub_type, key, 0, 1, &b_found);
    if (!node) {
        cmdl_zlog("@err>> hash node queue full\n");
        return 0;
    }

    if ( !zqueue_push_back(opt->access_stack, &node) ) {
        cmdl_zlog("@err>> hash node queue full\n");
    }

    if (node) {
        node->sub_type = zhash_reg_new_type(opt->h);
        node->func  = zopt_parse_node;
        node->p_val = 0;
        node->help  = help;

        return node;
    }

    return 0;
}

zh_addr_t zopt_end_group(zopt_t *opt, const char   *key)
{
    zopt_node_t **father = 0;

    father = zqueue_get_back_base(opt->access_stack);
    if (!father) {
        cmdl_zlog("@err>> access statck empty\n");
        return 0;
    }

    if (key && strcmp(key, (*father)->key) != 0 ) {
        cmdl_zlog("@err>> can't end non-last group\n");
        return 0;
    }

    father = zqueue_pop_back(opt->access_stack);
    return *father;
}

int zopt_parse_node(zopt_t *opt, zarg_iter_t *iter, zopt_node_t *node)
{
    char *arg;
    int   idx;
    zopt_node_t **root = zqueue_get_front_base(opt->access_stack);
    zopt_node_t *sub = 0;

    if (!node->sub_type) {
        idx = (node->func)(opt, iter, node);
        if (idx < 0) {
            cmdl_zlog("@err>> parse `-%s` err\n", node->key);
            return zarg_err(iter);
        }
        if (idx == 0) {     /** help */
            return idx;
        }
        return zarg_curr_idx(iter);
    }

    if (node != *root) 
    {
        arg = zarg_curr(iter);
        if ( arg && strcmp(arg, "(")==0 ) {
            zarg_next(iter);
        } else {
            cmdl_zlog("@err>> no openning for group\n");
            return zarg_err(iter);
        }
    }

    while ( 1 )
    {
        arg = zarg_curr_opt(iter);
        if (arg) {
            zarg_next(iter);
        } else {
            break;
        }

        sub = zhash_get_node(opt->h, node->sub_type, arg, 0);
        if (!sub) {
            cmdl_zlog("@err>> invalid `-%s`\n", arg);
            return zarg_err(iter);
        } 

        idx = zopt_parse_node(opt, iter, sub);
        if (idx < 0) {
            cmdl_zlog("@err>> parse `-%s` err\n", node->key);
            return zarg_err(iter);
        }
        if (idx == 0) {     /** help */
            return idx;
        }
    }

    if (node != *root) {
        arg = zarg_curr(iter);
        if ( arg && strcmp(arg, ")")==0 ) {
            zarg_next(iter);
        } else {
            cmdl_zlog("@err>> no ending for group\n");
            return zarg_err(iter);
        }
    } 

    return zarg_curr_idx(iter);
}

int zopt_parse_argcv(zopt_t *opt, int argc, char **argv)
{
    int ret =0;
    zarg_iter_t iter = zarg_iter(argc, argv);

    zopt_node_t **root = zqueue_get_front_base(opt->access_stack);
    if (!root) {
        cmdl_zlog("@err>> no root in the aceess stack\n");
        return 0;
    }
    if (zqueue_get_count(opt->access_stack) != 1) {
        cmdl_zlog("@warning>> too many ancestors in the aceess stack\n");
    }

    zarg_jump_prog(&iter);   
    ret = zopt_parse_node(opt, &iter, zopt_get_root(opt));

    return ret;
}


#ifdef ZOPT_TEST

typedef struct pet{
    char *name;
    char *color;
    int  icolor;
    int  age;
}pet_t;

int main(int argc, char **argv)
{
    zopt_t *opt = zopt_malloc(8);
    zopt_node_t *node = 0;
    pet_t dog = {"void", "void", 0};
    pet_t cat = {"void", "void", 0};
    int ret=0;

    zopt_add_root(opt, "main", "");
    zopt_add_node(opt, "help",   zopt_parse_help, 0,  "");

    zopt_start_group(opt, "dog", "dog props");
    zopt_add_node(opt, "name",   zopt_parse_str, &dog.name,  "");
    zopt_add_node(opt, "color",  zopt_parse_str, &dog.color, "");
    node = zopt_add_node(opt, "icolor", zopt_parse_enum, &dog.icolor, "");
    zopt_create_enum(opt, node, "red=0,green,blue, yellow=6");
    zopt_add_node(opt, "age",    zopt_parse_int, &dog.age,   "");
    zopt_end_group(opt, "dog");

    zopt_start_group(opt, "cat", "cat props");
    zopt_add_node(opt, "name",   zopt_parse_str, &cat.name,  "");
    zopt_add_node(opt, "color",  zopt_parse_str, &cat.color, "");
    node = zopt_add_node(opt, "icolor", zopt_parse_enum, &cat.icolor, "");
    zopt_create_enum(opt, node, "red=0,green,blue, yellow=6");
    zopt_add_node(opt, "age",    zopt_parse_int, &cat.age,   "");
    zopt_end_group(opt, "cat");

    zopt_print_help(opt, zopt_get_root(opt));
    ret = zopt_parse_argcv(opt, argc, argv);

    if (ret<0) {
        printf("@err>> parse error, ret = %d\n", ret);
        return 1;
    } else if (ret==0){
        // help
    } else{
        printf("\n\ndog : %s, %s, %d, %d; cat : %s, %s, %d, %d\n", 
            dog.name, dog.color, dog.icolor, dog.age,
            cat.name, cat.color, cat.icolor, cat.age); 
    }
    zopt_free(opt);

    return 0;
}

#endif  //ZOPT_TEST