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


#include "sim_utils.h"
#include "sim_log.h"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define EMPTYSTR                ("")
#define SAFE_STR(s, nnstr)      ((s)?(s):(nnstr))


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
 *  \param [in] i The idx of curr param to be parsed. 
 *  \return The idx of the next param to be parsed.
 */
int     arg_parse_range(int i, int argc, char *argv[], int i_range[2]);
int     arg_parse_str(int i, int argc, char *argv[], char **p);
int     arg_parse_strcpy(int i, int argc, char *argv[], char *buf, int nsz);
int     arg_parse_int(int i, int argc, char *argv[], int *p);
int     opt_parse_int(int i, int argc, char *argv[], int *p, int default_val);
int     arg_parse_ints(int i, int argc, char *argv[], int n, int *p[]);
int     opt_parse_ints(int i, int argc, char *argv[], int n, int *p[]);
int     arg_parse_xlevel(int i, int argc, char *argv[]);


enum option_type {
    OPT_T_BOOL      =   'b',
    OPT_T_INTI      =   'd',
    OPT_T_INTX      =   'x',
    OPT_T_STR       =   'sp',
    OPT_T_STRCPY    =   'sa',
};

typedef struct opt_reference {
    char *name;
    char *val;
} opt_ref_t;

typedef struct opt_enum {
    char *name;
    int   val;
} opt_enum_t;

int ref_name_2_idx(int nref, const opt_ref_t *refs, const char *name);
int ref_sval_2_idx(int nref, const opt_ref_t *refs, const char* val);
int ref_ival_2_idx(int nref, const opt_ref_t *refs, int val);
int enum_val_2_idx(int nenum, const opt_enum_t* enums, int val);
int enum_name_2_idx(int nenum, const opt_enum_t* enums, const char *name);
const char *enum_val_2_name(int nenum, const opt_enum_t* enums, int val);

typedef struct option_description {
    char*   name;
    int     type;
    int     narg;
    void*   pval;
    char*   default_val;
    char*   help;
    
    short   nref;
    short   nenum;
    union {
        const opt_ref_t* refs;
        const opt_enum_t* enums;
    };
    
//<! private:
    int     n_parse;
    int     argvIdx;
    int     b_default;
    int     i_ref;
    int     i_enum;
} opt_desc_t;

int cmdl_set_ref(int optc, opt_desc_t optv[], 
                const char *name, int n_ref, const opt_ref_t *refs);
int cmdl_set_enum(int optc, opt_desc_t optv[], 
                const char *name, int n_enum, const opt_enum_t *enums);

int cmdl_getdesc_byref (int optc, opt_desc_t optv[], const char *name);
int cmdl_getdesc_byname(int optc, opt_desc_t optv[], const char *name);
int cmdl_init(int optc, opt_desc_t optv[]);
int cmdl_parse(int i, int argc, char *argv[], int optc, opt_desc_t optv[]);
int cmdl_parse_opt(int i, int argc, char *argv[], opt_desc_t *opt);
int cmdl_help(int optc, opt_desc_t optv[]);
int cmdl_check(int optc, opt_desc_t optv[]);
int cmdl_result(int optc, opt_desc_t optv[]);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  // __SIM_OPT_H__
