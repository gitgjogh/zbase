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

#include "zhash.h"
#include "zutils.h"
#include "zlog.h"


zlog_t*     g_zlog_ptr  = 0;
zprintf_t   zlog_dbg    = printf;

void zlog_open(zlog_t *log)
{
    g_zlog_ptr = log;
}

void zlog_close()
{
    g_zlog_ptr = 0;
}


zlog_t* zlog_malloc(uint32_t depth_log2)
{
    zlog_t *log = calloc(1, sizeof(zlog_t));
    if (!log) {
        zlog_dbg("@zlog>> %s() failed!\n", __FUNCTION__);
        return 0;
    }

    log->hash_tbl = ZHASH_MALLOC(zlog_node_t, depth_log2);
    if (!log->hash_tbl) {
        zlog_dbg("@err>> %s() 1 failed!\n", __FUNCTION__);
        free(log);
        return 0;
    }
    
    return log;
}

void zlog_free(zlog_t *log)
{
    if (log) {
        if (g_zlog_ptr == log) { g_zlog_ptr = 0; }
        if (log->hash_tbl) { free(log->hash_tbl); }
        free(log);
    }
}

zcount_t zlogf_set_key(zlog_t *log, const char *key_list, int b_logf, zfile_t file)
{
    uint32_t pos, start, str_len;
    zlog_node_t *node = 0;
    zcount_t count = 0;

    for (pos = 0; ;pos = start+str_len) {
        str_len = get_token_pos(key_list, " ,", pos, &start);
        if (str_len<=0) {
            break;
        }

        node = zhash_touch_node(log->hash_tbl,  log->hash_type,
            key_list+start, str_len, 1, 0);
        if (!node) {
            break;
        }
        ++ count;
        node->b_logf = b_logf;
        node->file   = file;
    }
    log->logf_flag = (count > 0) ? 1 : log->logf_flag;

    return count;
}

zcount_t zlog1_set_key(zlog_t *log, const char *key_list, int b_log1)
{
    uint32_t pos, start, str_len;
    zlog_node_t *node = 0;
    zcount_t count = 0;

    for (pos = 0; ;pos = start+str_len) {
        str_len = get_token_pos(key_list, " ,", pos, &start);
        if (str_len<=0) {
            break;
        }

        node = zhash_touch_node(log->hash_tbl,  log->hash_type,
            key_list+start, str_len, 1, 0);
        if (!node) {
            break;
        }
        ++ count;
        node->b_log1 = b_log1;
    }
    log->log1_flag = (count > 0) ? 1 : log->log1_flag;

    return count;
}

static unsigned long zlog_get_key_len(const char *str) 
{ 
    int c = 0, i = 0;
    unsigned long hash = 0;
    if (str && (c=str[0]) && c == '@' ) {
        for (++str, i = 0; (c=str[i]) && c != '>'; ++i) {
        }
    }
    return i; 
}

int zlog_vprintf(zlog_t *log, const char *fmt, va_list ap)
{
    int r = 0;
    int b_log1, b_logf;
    zlog_node_t *node = 0;
    
    if (log->log1_flag == 1 || log->logf_flag == 1) 
    {
        int start = jump_front(fmt, " \n\r\t");
        int key_len = zlog_get_key_len(fmt+start);
        if (key_len > 0) {
            node = zhash_get_node(log->hash_tbl, log->hash_type, fmt+start+1, key_len);
        }
    }

    b_log1 = node ? node->b_log1: 0;
    b_log1 = (log->log1_flag == 0) ? 0 :b_log1;
    b_logf = (log->log1_flag == 2) ? 1 :b_log1;
    if (b_log1) {
        r = vprintf(fmt, ap);
    }

    b_logf = node ? node->b_logf : 0;
    b_logf = (log->log1_flag == 0) ? 0 : b_logf;
    if (b_logf && node && node->file) {
        r = vfprintf(node->file, fmt, ap);
    }

    return r;
}

int zlog_printf(zlog_t *log, const char *fmt, ...)
{
    int r = 0;
    if (log && log->hash_tbl) {
        va_list ap;
        va_start(ap, fmt);
        r = zlog_vprintf(log, fmt, ap);
        va_end(ap);
    }
    return r;
}

//<! zlog_printf(g_zlog_ptr, ##fmt)
int zlog(const char *fmt, ...)
{
    int r = 0;
    if (g_zlog_ptr && g_zlog_ptr->hash_tbl) {
        va_list ap;
        va_start(ap, fmt);
        r = zlog_vprintf(g_zlog_ptr, fmt, ap);
        va_end(ap);
    }
    return r;
}

static void zlog_show(zlog_t *log, const char *prompt)
{
    zh_iter_t _iter = zhash_iter(log->hash_tbl), *iter = &_iter;
    zlog_node_t *node;
    zlog_dbg("\n@zlog>> %s\n", prompt);
    for (node = zhash_front(iter); node; node = zhash_next(iter)) 
    {
        if (node->type == log->hash_type) {
            zlog_dbg("@zlog>> 0x%08x %-10s, type=%d, b_log1=%d, b_logf=%d, file=0x%08x\n",
                node->hash, node->key,  node->type,  node->b_log1, node->b_logf, node->file);
        }
    }
    printf ("\n");
}

#ifdef ZLOG_TEST

int main(int argc, char** argv)
{
    zlog_t *log = zlog_malloc(8);
    zlog_open(log);
    zlog1_add_key(log, "key, key0, key01, key012");

    zlog_show(log, "1");

    zlog("key>> hello %d\n", -2);
    zlog("@key>> hello %d\n", -1);
    zlog("@key0>> hello %d\n", 0);
    zlog("@key01>> hello %d\n", 1);
    zlog("@key012>> hello %d\n", 2);
    zlog("@key0123>> hello %d\n", 3);

    zlog_show(log, "2");

    zlog1_add_key(log, "key01, key0123");
    zlog1_del_key(log, "key0, key012");

    zlog_show(log, "3");

    zlog("key>> hello %d\n", -2);
    zlog("\t@key>> hello %d\n", -1);
    zlog("@key0>> hello %d\n", 0);
    zlog("@key01>> hello %d\n", 1);
    zlog("@key012>> hello %d\n", 2);
    zlog("@key0123>> hello %d\n", 3);

    zlog_show(log, "4");
    
    zlog_close();
    zlog_free(log);
    
    return 0;
}

#endif  // ZSTRQ_TEST