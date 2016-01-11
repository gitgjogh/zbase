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
#include <stdio.h>

#include "sim_log.h"


static slog_t  g_slog_obj = { 0 };
// static slog_t *g_slog_ptr = &g_slog_obj;


#ifndef MAX
#define MAX(a,b)        ((a)>(b) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)        ((a)<(b) ? (a) : (b))
#endif


int slog_set_range(slog_t *sl, int minL, int maxL, void *fp)
{
    minL = MAX(minL, SLOG_NON);
    maxL = MIN(maxL, SLOG_ALL);
    if (minL<=maxL) {
        int level;
        for (level = minL; level <= maxL; ++level) {
            sl->fp[level] = fp;
        }
        return (maxL - minL + 1);
    }
    return 0;
}

int slog_init(slog_t *sl, int level)
{
    memset(sl, 0, sizeof(slog_t));
    sl->b_inited = 1;
    if (level != SLOG_NON) { 
        sl->fp[SLOG_NON] = sl->fp[SLOG_ERR] = stderr;
        sl->level = level;
        return slog_set_range(sl, SLOG_ERR+1, level, stdout);
    }
    return 0;
}

void slog_reset(slog_t *sl)
{
    memset(sl, 0, sizeof(slog_t));
}

int xlog_set_range(int minL, int maxL, void *fp)
{
    return slog_set_range(&g_slog_obj, minL, maxL, fp);
}

int xlog_init(int level)
{
    return slog_init(&g_slog_obj, level);
}

void xlog_reset()
{
    slog_reset(&g_slog_obj);
}

int xlog_set_level(int level)
{
    return slog_init(&g_slog_obj, level);
}

int xlog_get_level()
{
    return g_slog_obj.level;
}

int slogv(slog_t *sl, int level, const char *prompt, const char *fmt, va_list ap)
{
    FILE *fp = 0;
    if (sl->b_inited) {
        level = clip(level, SLOG_NON, SLOG_ALL);
        fp = sl->fp[level];
    } else {
        fp = (level <= SLOG_ERR) ? stderr : stdout;
    }
    
    if (fp) {
        if (prompt) {
            fprintf(fp, "@%s>> ", prompt);
        }
        int ret = vfprintf(fp, fmt, ap);
        fflush(fp);
        return ret;
    }

    return 0;
}

int xlogv(int level, const char *prompt, const char *fmt, va_list ap)
{
    return slogv(&g_slog_obj, level, prompt, fmt, ap);
}

int slog (slog_t *sl, int level, const char *prompt, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = slogv(sl, level, prompt, fmt, ap);
    va_end(ap);
    return r;
}
int xlog (int level, const char *prompt, const char *fmt, ...)
{
    int r = 0;
    va_list ap;
    va_start(ap, fmt);
    r = xlogv(level, prompt, fmt, ap);
    va_end(ap);
    return r;
}
