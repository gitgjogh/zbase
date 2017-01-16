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

#ifndef __SIM_OPT_H__
#define __SIM_OPT_H__


#include <stddef.h>
#include "sim_utils.h"
#include "sim_log.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define EMPTYSTR                ("")
#define SAFE_STR(s, nnstr)      ((s)?(s):(nnstr))
#define SIM_NULL_END            (0)


#define MAX_MODE    8
typedef struct file_io {
    int     b_used;
    char    path_buf[MAX_PATH];
    char    *path;
    char    mode[MAX_MODE];
    void    *fp;
} ios_t;

void    iof_cfg(ios_t *p, const char *path, const char *mode);
void    ios_cfg(ios_t *p, int ch, const char *path, const char *mode);
int     ios_nused(ios_t *p, int nch);
int     ios_open(ios_t *p, int nch, int *nop);
int     ios_close(ios_t *p, int nch);
int     ios_feof(ios_t *p, int ich);

#define GET_ARGV(idx, name) get_argv(argc, argv, idx, name);
char*   get_argv(int argc, char *argv[], int i, const char *name);

/**
 *  @param[in] i The idx of curr param to be parsed.
 *  @return The idx of the next param to be parsed.
 */
int     arg_parse_range(int i, int argc, char *argv[], int i_range[2]);
int     arg_parse_str(int i, int argc, char *argv[], char **p);
int     arg_parse_strcpy(int i, int argc, char *argv[], char *buf, int nsz);
int     arg_parse_int(int i, int argc, char *argv[], int *p);
int     opt_parse_int(int i, int argc, char *argv[], int *p, int default_val);
int     arg_parse_ints(int i, int argc, char *argv[], int n, int *p[]);
int     opt_parse_ints(int i, int argc, char *argv[], int n, int *p[]);
int     arg_parse_xlevel(int i, int argc, char *argv[]);


typedef struct opt_reference {
    char *name;
    char *val;
} opt_ref_t;

typedef struct opt_enum {
    char *name;
    int   val;
} opt_enum_t;

/* null end (name for) refs[] or enums[] */
const opt_ref_t*  sim_name_2_ref(const opt_ref_t refs[], const char *name);
const opt_ref_t*  sim_sval_2_ref(const opt_ref_t refs[], const char *sval);
const opt_ref_t*  sim_ival_2_ref(const opt_ref_t refs[], int ival);
#define     sim_val_2_ref       sim_sval_2_ref
const opt_enum_t* sim_name_2_enum(const opt_enum_t enums[], const char *name);
const opt_enum_t* sim_sval_2_enum(const opt_enum_t enums[], const char *sval);
const opt_enum_t* sim_ival_2_enum(const opt_enum_t enums[], int ival);
#define     sim_val_2_enum      sim_sval_2_enum

typedef struct cmdl_iter {
    char**  argv;
    int     argc;
    int     start;
    int     idx;

    int     layer;
} cmdl_iter_t;

cmdl_iter_t  cmdl_iter_init(int argc, char **argv, int start);
int     cmdl_iter_idx (cmdl_iter_t *iter);
char*   cmdl_iter_1st (cmdl_iter_t *iter);
char*   cmdl_iter_next(cmdl_iter_t *iter);
char*   cmdl_iter_last(cmdl_iter_t *iter);
char*   cmdl_iter_prev(cmdl_iter_t *iter);
char*   cmdl_iter_curr(cmdl_iter_t *iter);              /** argv[iter->idx] */
char*   cmdl_peek_next(cmdl_iter_t *iter);              /** argv[1 + iter->idx] */
char*   cmdl_peek_ith (cmdl_iter_t *iter, int ith);     /** argv[ith + iter->idx] */
char*   cmdl_iter_pop (cmdl_iter_t *iter, int b_opt);
void    cmdl_layer_prefix(int layer);
void    cmdl_iter_dbg (cmdl_iter_t *iter);

typedef enum {
    CMDL_ACT_PARSE = 0,
    CMDL_ACT_HELP,          ///< print help
    CMDL_ACT_ARGFMT,        ///< print arg format
    CMDL_ACT_RESULT,        ///< print parsing result
} CMDL_ACT_e;

typedef enum {
    CMDL_RET_ERROR_CHECK = -12,
    CMDL_RET_ERROR_INIT = -11,
    CMDL_RET_ERROR = -10,       /* retcode <= CMDL_RET_ERROR should be error */
    CMDL_RET_HELP  = -3,
    CMDL_RET_NOT_OPT = -2,      /* end with non-option, maybe not error */
    CMDL_RET_UNKNOWN = -1,      /* end with unkown option, maybe not error */
} CMDL_RET_e;

typedef struct cmdl_option_description cmdl_opt_t;
/**
 * @param[in,out] iter  cmdl_iter_curr(iter) is the name for current option
 *               befor calling, while the last arg be parsed after calling.
 *             cmdl_iter_next(iter) is the first arg for current option before 
 *               calling, while the next arg/option to be parsed after calling.
 * @param[out]    dst   arg receiver
 * @param[in]     act   see @cmdl_act_t
 * @param[in,out] opt   extra properies used for arg parsing
 * @return number (>=0) of args being parsed, (-1) means error
 */
typedef int (*cmdl_parse_func_t)(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_range (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_str   (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_strcpy(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_int   (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_ints  (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_xlevel(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);
int cmdl_parse_help  (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt);


typedef struct cmdl_option_description {
    int     b_required;
    char*   names;
    int     narg;                       /* (-1) means unknown, (0) means 0 or 1 */
    cmdl_parse_func_t parse;
    size_t  dst_offset;
    char*   default_val;
    char*   help;
    
    const opt_ref_t*  refs;
    const opt_enum_t* enums;
    
///< private props:
    int     n_parse;
    int     argvIdx;
    int     b_default;
    const opt_ref_t*  p_ref;
    const opt_enum_t* p_enum;
} cmdl_opt_t;

cmdl_opt_t* cmdl_getdesc_byref (cmdl_opt_t optv[], const char *ref_name);
cmdl_opt_t* cmdl_getdesc_byname(cmdl_opt_t optv[], const char *opt_name);
cmdl_opt_t* cmdl_set_ref (cmdl_opt_t optv[], const char *opt_name, const opt_ref_t *refs);
cmdl_opt_t* cmdl_set_enum(cmdl_opt_t optv[], const char *opt_name, const opt_enum_t *enums);


int cmdl_parse(cmdl_iter_t *iter, void *dst, cmdl_opt_t optv[]);
int cmdl_help(cmdl_iter_t *iter, void* null, cmdl_opt_t optv[]);
int cmdl_result(cmdl_iter_t *iter, void* dst, cmdl_opt_t optv[]);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // __SIM_OPT_H__
