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

/**
 *  -log1 keylist
 *  -log0 keylist
 *  -logf ( -f path -k keylist -0/-1 )
 */

#ifndef ZLOG_H_
#define ZLOG_H_


#include <stdarg.h>
#include "zdefs.h"
#include "zhash.h"


typedef struct zlog_node {
    ZHASH_COMMON;
    int         b_log1;
    int         b_logf;
    zfile_t     file;
}zlog_node_t;

typedef struct zhash_log {
    zhash_t     *hash_tbl;
    uint32_t    hash_type;

    uint32_t    log1_flag;      /** stdout option - 0:none, 1: selected, 2:all */
    uint32_t    logf_flag;      /** file logs option - 0:none, 1: selected */
}zlog_t;

zlog_t*     zlog_malloc(uint32_t depth_log2);
void        zlog_free(zlog_t *opt);

zcount_t    zlogf_set_key(zlog_t *log, const char *keylist, int b_logf, zfile_t file);
#define     zlogf_add_key(log, keys, f)     zlogf_set_key((log), (keys), (1), (f))
#define     zlogf_del_key(log, keys)        zlogf_set_key((log), (keys), (0), (0))

zcount_t    zlog1_set_key(zlog_t *log, const char *keylist, int b_log1);
#define     zlog1_add_key(log, keys)        zlog1_set_key((log), (keys), (1))
#define     zlog1_del_key(log, keys)        zlog1_set_key((log), (keys), (0))

int         zlog_vprintf (zlog_t *log, const char *fmt, va_list ap);
int         zlog_printf  (zlog_t *log, const char *fmt, ...);


extern      zlog_t*     g_zlog_ptr;
void        zlog_open(zlog_t *log);
int         zlog(const char *fmt, ...);
void        zlog_close();
#define     xtr_i(v)    zlog("@var>> " #v " = %d\n", v)
#define     xtr_s(v)    zlog("@var>> " #v " = %s\n", v)

#endif //#ifndef ZLOG_H_