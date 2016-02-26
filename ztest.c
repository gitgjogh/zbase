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
#include <assert.h>

#include "zlist.h"
//#include "zopt.h"
#include "zarray.h"
#include "zstrq.h"
#include "zhash.h"
#include "zhtree.h"

#include "sim_opt.h"


#define DEREF_I32(pi)           (*((int *)pi))
#define SET_ITEM(qidx)          (item=qidx, &item)

static
int32_t int_cmpf(zaddr_t cmp_base, zaddr_t elem_base)
{
    return DEREF_I32(cmp_base) - DEREF_I32(elem_base);
}

static
void int_printf(zqidx_t idx, zaddr_t elem_base)
{
    //printf("[%d] : %d, ", idx, DEREF_I32(elem_base));
    printf("%d", DEREF_I32(elem_base));
}

static
void str_printf(zqidx_t idx, zaddr_t elem_base)
{
    //printf("[%d] : %s, ", idx, elem_base);
    printf("%s", elem_base);
}

int zlist_test(int argc, char** argv)
{
    int item, idx;
    zaddr_t ret, popped = &item;

    zlist_t *q1 = ZLIST_MALLOC_D(int, 10);
    zlist_t *q2 = ZLIST_MALLOC_S(int, 20);
    for (idx=1; idx<8; ++idx) {
        zlist_push_back(q1, SET_ITEM(10-idx));
        zlist_push_back(q2, SET_ITEM(10+idx));
    }
    zlist_print(q1, "init with 1~7, size=10", int_printf, ", ", "\n\n");

    ret = zlist_push_back(q1, SET_ITEM(8));    printf("push %d %s \n", item, ret ? "success" : "fail");   
    ret = zlist_push_back(q1, SET_ITEM(9));    printf("push %d %s \n", item, ret ? "success" : "fail");  
    ret = zlist_push_back(q1, SET_ITEM(10));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    zlist_print(q1, "push 8,9,10 ", int_printf, ", ", "\n\n");

    ret = zlist_push_back(q1, SET_ITEM(11));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    ret = zlist_push_back(q1, SET_ITEM(12));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    zlist_print(q1, "push 11,12", int_printf, ", ", "\n\n");

    zlist_pop_front(q1, popped) ? printf("pop front = %d \n", DEREF_I32(popped)) : printf("pop fail \n");
    zlist_pop_front(q1, popped) ? printf("pop front = %d \n", DEREF_I32(popped)) : printf("pop fail \n");
    zlist_print(q1, "pop front 2 items", int_printf, ", ", "\n\n");

    zlist_pop_first_match(q1, SET_ITEM(7), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zlist_print(q1, "pop first match 7", int_printf, ", ", "\n\n");

    zlist_pop_first_match(q1, SET_ITEM(8), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zlist_print(q1, "pop first match 8", int_printf, ", ", "\n\n");

    zlist_pop_first_match(q1, SET_ITEM(11), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zlist_print(q1, "pop first match 11", int_printf, ", ", "\n\n");

    ret = zlist_insert_elem(q1, 0, SET_ITEM(11));     
    printf("push front %d ", item);
    ret ?  printf("= %d \n", DEREF_I32(ret)) : printf("fail\n");
    zlist_print(q1, "push 11 at front ", int_printf, ", ", "\n\n");

    zlist_quick_sort(q1, int_cmpf);
    zlist_print(q1, "quick sort", int_printf, ", ", "\n\n");

    zlist_print(q2, "q2 before cat", int_printf, ", ", "\n\n");
    zlist_insert_all_of_others(q2, 4, q1);
    zlist_print(q2, "zlist_insert_list(q2, 4, q1)", int_printf, ", ", "\n\n");

    zlist_quick_sort(q2, int_cmpf);
    zlist_print(q2, "q2 quick sort", int_printf, ", ", "\n\n");

    zlist_free(q1);
    zlist_free(q2);

    return 0;
}

int zarray_test(int argc, char** argv)
{
    int item, idx;
    zaddr_t ret, popped = &item;

    zarray_t *za = ZARRAY_MALLOC_S(int, 10);
    for(idx=1; idx<8; ++idx)
        zarray_push_back(za, &idx);
    zarray_print(za, "init with 1~7, size=10", int_printf, ", ", "\n\n");

    ret = zarray_push_back(za, SET_ITEM(8));    printf("push %d %s \n", item, ret ? "success" : "fail");   
    ret = zarray_push_back(za, SET_ITEM(9));    printf("push %d %s \n", item, ret ? "success" : "fail");  
    ret = zarray_push_back(za, SET_ITEM(10));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    zarray_print(za, "push 8,9,10 ", int_printf, ", ", "\n\n");

    ret = zarray_push_back(za, SET_ITEM(11));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    ret = zarray_push_back(za, SET_ITEM(12));   printf("push %d %s \n", item, ret ? "success" : "fail"); 
    zarray_print(za, "push 11,12", int_printf, ", ", "\n\n");

    zarray_pop_front(za, popped) ? printf("pop front = %d \n", DEREF_I32(popped)) : printf("\n pop fail \n");
    zarray_print(za, "pop front 2 items", int_printf, ", ", "\n\n");

    zarray_pop_first_match(za, SET_ITEM(7), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zarray_print(za, "pop first match 7", int_printf, ", ", "\n\n");

    zarray_pop_first_match(za, SET_ITEM(8), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zarray_print(za, "pop first match 7", int_printf, ", ", "\n\n");

    zarray_pop_first_match(za, SET_ITEM(11), int_cmpf, popped) 
        ? printf("pop %d = %d \n", item, DEREF_I32(popped)) 
        : printf("pop %d failed\n", item);
    zarray_print(za, "pop first match 7", int_printf, ", ", "\n\n");

    ret = zarray_push_front(za, SET_ITEM(11));     
    printf("push front %d ", item);
    ret ?  printf("= %d \n", DEREF_I32(ret)) : printf("fail \n");
    zarray_print(za, "push 11 at front ", int_printf, ", ", "\n\n");

    zarray_quick_sort(za, int_cmpf);
    zarray_print(za, "quick sort", int_printf, ", ", "\n\n");

    zarray_free(za);

    return 0;
}


int zstrq_test(int argc, char** argv)
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

    zstrq_print(q1, "q1", str_printf, ", ", "\n\n");
    zstrq_print(q2, "q2", str_printf, ", ", "\n\n");

    zstrq_push_back_all(q1, q2);
    zstrq_print(q1, "q1", str_printf, ", ", "\n\n");

    str = zstrq_pop_back(q1, 0);
    printf("pop(q1) = %s\n", str ? str : "[failed]");
    zstrq_print(q1, "q1", str_printf, ", ", "\n\n");

    zstrq_free(q1);
    zstrq_free(q2);

    return 0;
}

typedef struct zh_test_node_t {
    ZH_NODE_COMMON;
    zsq_char_t *p_obj;
}zh_test_node_t;

int zhash_test(int argc, char** argv)
{
    zhash_t *h = ZHASH_MALLOC(zh_test_node_t, 8);
    zh_test_node_t *node;
    int b_found = 0;

    node = zhash_set_node(h, "HOME", 0);
    if (node) {
         node->p_obj = "/home/zjfeng";
    }
    node = zhash_set_node(h, "LIB", 0);
    if (node) {
        node->p_obj  = "/user/share/lib";
    }
    node = zhash_set_node(h, "BIN", 0);
    if (node) {
        node->p_obj = "/user/share/bin";
    }

    node = zhash_get_node(h, "LIB", 0);
    if (node) {
        xprint("<zhash> $LIB = %s\n", node->p_obj);
    } else {
        xprint("<zhash> get LIB fail\n");
        goto main_err_exit;
    }

    node = zhash_get_node(h, "USER", 0);
    if (node) {
        xprint("<zhash> $USER = %s\n", node->p_obj);
    } else {
        xprint("<zhash> get $USER fail\n");
        goto main_err_exit;
    }

main_err_exit:
    zhash_free(h);

    return 0;
}


const opt_enum_t color_enum[] = 
{
    {"white",   0},
    {"black",   1},
    {"green",   2},
    {"yellow",  6},
};

typedef struct pet{
    char *name;
    int  color;     /* @see color_t */
    int  age;
} pet_t;

#define PET_OPT_M(member) offsetof(pet_t, member)
cmdl_opt_t pet_opt[] = 
{
    { 1, "name",  1, cmdl_parse_str,  PET_OPT_M(name),  0,         "pet's name",  },
    { 0, "color", 1, cmdl_parse_int,  PET_OPT_M(color), "%white",  "pet's color", },
    { 0, "age",   1, cmdl_parse_int,  PET_OPT_M(age),   "0",       "pet's age",   },
};

int cmdl_pet_parser(cmdl_iter_t *iter, void* dst, cmdl_act_t act, cmdl_opt_t *opt)
{
    cmdl_set_enum(ARRAY_TUPLE(pet_opt), "color",  ARRAY_TUPLE(color_enum)); 

    if (act == CMDL_ACT_PARSE) {
        return cmdl_parse(iter, dst, ARRAY_TUPLE(pet_opt));
    }
    else if (act == CMDL_ACT_HELP) {
        return cmdl_help(iter, 0, ARRAY_TUPLE(pet_opt));
    } 
    else if (act == CMDL_ACT_RESULT) {
        return cmdl_result(iter, dst, ARRAY_TUPLE(pet_opt));
    }

    return 0;
}

int cmdl_test(int argc, char **argv)
{
    typedef struct cmdl_param {
        pet_t cat, dog;
    } cmdl_param_t;

    #define CMDL_OPT_M(member) offsetof(cmdl_param_t, member)
    cmdl_opt_t cmdl_opt[] = 
    {
        { 0, "h,help", 0, cmdl_parse_help,         0,      0,  "show help"},
        { 1, "dog", 1, cmdl_pet_parser,  CMDL_OPT_M(dog),  0,  "dog's prop...", },
        { 1, "cat", 1, cmdl_pet_parser,  CMDL_OPT_M(cat),  0,  "cat's prop...", },
    };

    cmdl_param_t cfg;
    cmdl_iter_t iter = cmdl_iter_init(argc, argv, 0);
    int r = cmdl_parse(&iter, &cfg, ARRAY_TUPLE(cmdl_opt));
    if (r == CMDL_RET_HELP) {
        return cmdl_help(&iter, 0, ARRAY_TUPLE(cmdl_opt));
    } else if (r < 0) {
        xerr("cmdl_parse() failed, ret=%d\n", r);
        return 1;
    }

    cmdl_result(&iter, &cfg, ARRAY_TUPLE(cmdl_opt));

    return 0;
}

/*
int zopt_test(int argc, char **argv)
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
*/

static
void zht_printf(zqidx_t idx, zaddr_t base)
{
    zht_node_t *node = base;
    printf("%d:%s", idx, node->key);
}

int zhtree_test(int argc, char **argv)
{
    zhtree_t *h = zhtree_malloc(sizeof(zht_node_t), 8);
    zht_node_t *video = zhtree_touch_node(h, "/video", 0);          assert(video);
    zht_node_t *size = zhtree_touch_child(h, video, "size", 0);     assert(size);
    zhtree_touch_child(h, size, "w", 0);
    zhtree_touch_child(h, size, "h", 0);
    
    zhtree_touch_node(h, "/video/./../audio/fmt/name", 0);
    zhtree_touch_node(h, "/audio/fmt/name", 0);
    zhtree_touch_node(h, "/audio/fmt/ar", 0);
    zhtree_change_wnode(h, "/audio/fmt", 0);
    zhtree_touch_node(h, "ac", 0);
    zhtree_touch_node(h, "name", 0);

    zarray_print(h->nodeq, "zhtree->nodeq", zht_printf, ", ", "\n");
    //zhtree_free(h);
    //return 0;

    zht_node_t *node = 0;
    zht_iter_t iter = zhtree_iter_open(h);
    if (zhtree_iter_assert(&iter)) {
        WHILE_GET_ZHT_NODE(&iter, node)
        {
            zht_print_full_path(h, node);
            xprint("\n");
        }
        xprint("\n");

        WHILE_GET_ZHT_NODE_REVERSE(&iter, node)
        {
            zht_print_full_path(h, node);
            xprint("\n");
        }
    }
    zhtree_iter_close(&iter);
    
    zhtree_free(h);

    return 0;
}


int main(int argc, char **argv)
{
    int i=0, j = 0;
    int exit_code = 0;
    
    typedef struct yuv_module {
        const char *name; 
        int (*func)(int argc, char **argv);
        const char *help;
    } yuv_module_t;
    
    const static yuv_module_t sub_main[] = {
        {"list",    zlist_test,     ""},
        //{"opt",     zopt_test,      ""},
        {"cmdl",    cmdl_test,      ""},
        {"array",   zarray_test,    ""},
        {"strq",    zstrq_test,     ""},
        {"hash",    zhash_test,     ""},
        {"zhtree",  zhtree_test,    ""},
    };

    xlog_init(SLOG_DEFULT);
    i = arg_parse_xlevel(1, argc, argv);
    
    xdbg("@cmdl>> argv[%d] = %s\n", i, i<argc ? argv[i] : "?");

    if (i>=argc) {
        printf("No module specified. ");
    } else {
        for (j=0; j<ARRAY_SIZE(sub_main); ++j) {
            if (0==strcmp(argv[i], sub_main[j].name)) {
                return sub_main[j].func(argc-i, argv+i);
            }
        }
        if (strcmp(argv[i], "-h") && strcmp(argv[i], "--help")) {
            printf("`%s` is not support. ", argv[i]);
        }
    }
    
    printf("Use the following modules:\n");
    for (j=0; j<ARRAY_SIZE(sub_main); ++j) {
        printf("\t%-4s ? %s\n", sub_main[j].name, sub_main[j].help);
    }
    return exit_code;
    
    return 0;
}

