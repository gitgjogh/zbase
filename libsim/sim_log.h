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

#ifndef __SIM_LOG_H__
#define __SIM_LOG_H__

#include <stdarg.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


typedef enum slog_level {
    SLOG_NON        = 0,
    SLOG_ERR        ,
    SLOG_WARN       ,
    SLOG_ALERT      ,
    SLOG_NOTICE     ,
    SLOG_INFO       ,
    SLOG_DEFULT     = 30,
    SLOG_PRINT      = SLOG_DEFULT,
    SLOG_DBG        = 50,
    SLOG_CMDL       = 55,
    SLOG_MEM        = 60,
    SLOG_IOS        = 70,
    SLOG_FUNC       = 80,
    SLOG_VAR        = 90,
    SLOG_ALL        = 99,
} slog_level_i;


typedef struct simple_level_log
{
    int         b_inited;
    void       *fp[SLOG_ALL+1];
} slog_t;

int slog_set_range(slog_t *sl, int minL, int maxL, void *fp);
int slog_init(slog_t *sl, int level);
void slog_reset(slog_t *sl);
int slogv(slog_t *sl, int level, const char *prompt, const char *fmt, va_list ap);
int slog (slog_t *sl, int level, const char *prompt, const char *fmt, ...);

int xlog_set_range(int minL, int maxL, void *fp);
int xlog_init(int level);
void xlog_reset();
int xlevel(int level);
int xlogv(int level, const char *prompt, const char *fmt, va_list ap);
int xlog (int level, const char *prompt, const char *fmt, ...);

#define xerr(...)               xlog(SLOG_ERR, "err", __VA_ARGS__)
#define xwarn(...)              xlog(SLOG_WARN, "warn", __VA_ARGS__)
#define xdbg(...)               xlog(SLOG_DBG, "dbg", __VA_ARGS__)
#define xprint(...)             xlog(SLOG_PRINT, 0, __VA_ARGS__)
#define xlog_ios(...)           xlog(SLOG_IOS, "ios", __VA_ARGS__)
#define xlog_cmdl(...)          xlog(SLOG_CMDL, "cmdl", __VA_ARGS__)


static int fcall_layer = 0;
#define ENTER_FUNC()  \
    xlog(SLOG_FUNC, "+++", "%-2d: %s(+)\n", fcall_layer++, __FUNCTION__)
#define LEAVE_FUNC()  \
    xlog(SLOG_FUNC, "---", "%-2d: %s(-)\n\n", --fcall_layer, __FUNCTION__)


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // __SIM_LOG_H__
