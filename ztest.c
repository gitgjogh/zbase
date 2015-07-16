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

#include "zlist.h"
#include "zopt.h"
#include "zqueue.h"
#include "zstrq.h"
#include "zhash.h"

#include "sim_opt.h"
//#inlcude "sim_log.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a)   (sizeof(a)/sizeof(a[0]))
#endif

int zlist_test(int argc, char** argv);
int zlog_test(int argc, char** argv);
int zopt_test(int argc, char** argv);
int zqueue_test(int argc, char** argv);
int zstrq_test(int argc, char** argv);
int zhash_test(int argc, char** argv);

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
        {"opt",     zopt_test,      ""},
        {"queue",   zqueue_test,    ""},
        {"strq",    zstrq_test,     ""},
        {"hash",    zhash_test,     ""},
    };

    xlog_init(SLOG_DBG-1);
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

