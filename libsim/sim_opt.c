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

#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "sim_opt.h"

static int cmdl_init(cmdl_opt_t optv[]);
static int cmdl_check(cmdl_iter_t *iter, void* dst, cmdl_opt_t optv[]);


void iof_cfg(ios_t *f, const char *path, const char *mode)
{
    f->b_used = (path && mode);
    if (path) {
        strncpy(f->path_buf, path, MAX_PATH);
        f->path = f->path_buf;
    }
    if (mode) {
        strncpy(f->mode, mode, MAX_MODE);
    }
}

void ios_cfg(ios_t *ios, int ch, const char *path, const char *mode)
{
    iof_cfg(&ios[ch], path, mode);
}

int ios_nused(ios_t *ios, int nch)
{
    int ch, j;
    for (ch=j=0; ch<nch; ++ch) {
        j += (!!ios[ch].b_used);
    }
    return j;   
}

/**
 *  @param [in] ios - array of io description
 *  @param [out] nop - num of files truely opened
 *  @return num of files failed to be opened
 */
int ios_open(ios_t ios[], int nch, int *nop)
{
    int ch, j, n_err;
    for (ch=j=n_err=0; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used) 
        {
            if (strchr(f->mode, 'a') || strchr(f->mode, 'w') || strchr(f->mode, '+')) 
            {
                FILE *fp = fopen(f->path, "r");
                if (fp) {
                    char yes_or_no = 0;
                    fclose(fp); fp =0;
                    xprint("file `%s` already exist, overwrite? (y/n) : ", f->path);
                    int r = scanf("%c", &yes_or_no);
                    if (r!=1 || yes_or_no != 'y') {
                        xprint("file `%s` would be skipped\n", f->path);
                        continue;
                    }
                }
            }
            
            f->fp = fopen(f->path, f->mode);
            if ( f->fp ) {
                xlog_ios("ch#%d 0x%08x=fopen(%s, %s)\n", ch, f->fp, f->path, f->mode);
                ++ j;
            } else {
                xerr("<ios> error fopen(%s, %s)\n", f->path, f->mode);
                ++ n_err;
            }
        }
    }
    nop ? (*nop = j) : 0;
    return (n_err==0);   
}

int ios_close(ios_t *ios, int nch)
{
    int ch, j;
    for (ch=j=0; ch<nch; ++ch) {
        ios_t *f = &ios[ch];
        if (f->b_used && f->fp) {
            xlog_ios("ch#%d fclose(0x%08x: %s)\n", ch, f->fp, f->path);
            fclose(f->fp);
            f->fp = 0;
            ++ j;
        }
    }
    return j;  
}

int ios_feof(ios_t *p, int ich)
{
    return feof((FILE*)p[ich].fp);
}

char *get_argv(int argc, char *argv[], int i, const char *name)
{
    int s = (argv && i<argc) ? argv[i][0] : 0;
    char *arg = (s != 0 && s != '-') ? argv[i] : 0;
    if (name) {
        xlog_cmdl("-%s[%d] = `%s`\n", SAFE_STR(name,""), i, SAFE_STR(arg,""));
    }
    return arg;
}

int arg_parse_range(int i, int argc, char *argv[], int i_range[2])
{
    char *flag=0;
    char *last=0;
    char *arg = GET_ARGV(i, "range");
    if (!arg) {
        return -1;
    }
    
    i_range[0] = 0;
    i_range[1] = INT_MAX;
    
    /* parse argv : `$base[~$last]` or `$base[+$count]` */
    //i_range[0] = strtoul (arg, &flag, 10);
    flag = get_uint32 (arg, &i_range[0]);
    if (flag==0 || *flag == 0) {       /* no `~$last` or `+$count` */
        return ++i;
    }

    /* get `~$last` or `+$count` */
    if (*flag != '~' && *flag != '+') {
        xerr("<cmdl> Err : Invalid flag\n");
        return -1;
    }
    
    //i_range[1] = strtoul (flag + 1, &last, 10);
    last = get_uint32 (flag + 1, &i_range[1]);
    if (last == 0 || *last != 0 ) {
        xerr("<cmdl> Err : Invalid count/end\n");
        i_range[1] = INT_MAX;
        return -1;
    }
    
    if (*flag == '+') {
        i_range[1] += i_range[0];
    }
    
    return ++i;
}

int arg_parse_str(int i, int argc, char *argv[], char **p)
{
    char *arg = GET_ARGV(i, "string");
    *p = arg ? arg : 0;
    return arg ? ++i : -1;
}

int arg_parse_strcpy(int i, int argc, char *argv[], char *buf, int nsz)
{
    char *arg = GET_ARGV(i, "string");
    if (arg) {
        strncpy(buf, arg, nsz);
        buf[nsz-1] = 0;
    } else {
        buf[0] = 0;
    }
    return arg ? ++i : -1;
}

int arg_parse_int(int i, int argc, char *argv[], int *p)
{
    char *arg = GET_ARGV(i, "int");
    if (arg) {
        int b_err = str_2_int(arg, p);
        return (++i) * (b_err ? -1 : 1);
    } else {
        return -1;
    }
}

int opt_parse_int(int i, int argc, char *argv[], int *p, int default_val)
{
    char *arg = GET_ARGV(i, "int");
    if (arg) {
        int b_err = str_2_int(arg, p);
        return (++i) * (b_err ? -1 : 1);
    } else {
        *p = default_val;
        return i;
    }
}

int arg_parse_ints(int i, int argc, char *argv[], int n, int *p[])
{
    int j;
    for (j=0; j<n; ++j) {
        char *arg = GET_ARGV(i, "int");
        if (arg) {
            *(p[j]) = atoi(arg);
            ++ i;
        } else {
            return -1;
        }
    }
    return i;
}

int opt_parse_ints(int i, int argc, char *argv[], int n, int *p[])
{
    int j;
    for (j=0; j<n; ++j) {
        char *arg = GET_ARGV(i, "int");
        if (arg) {
            *(p[j]) = atoi(arg);
            ++ i;
        } else {
            *(p[j]) = 0;
        }
    }
    return i;
}

int arg_parse_xlevel(int i, int argc, char *argv[])
{
    int j = i;
    while (i>=j && i<argc)
    {
        char *arg = &argv[i][1];
        if (0==strcmp(arg, "xnon")) {
            ++i;    xlevel(SLOG_NON);
        } else
        if (0==strcmp(arg, "xall")) {
            ++i;    xlevel(SLOG_ALL);
        } else
        if (0==strcmp(arg, "xl") || 0==strcmp(arg, "xlevel")) {
            int level;
            i = arg_parse_int(++i, argc, argv, &level);
            xlevel(level);
        } else
        {
            break;
        }
        j = i;
    }

    return i;
}

const cmdl_ref_t* sim_name_2_ref(const cmdl_ref_t *refs, const char *name)
{
    if (refs == 0 || name == 0) { return 0; }

    for (const cmdl_ref_t *p = refs; p->name; ++p) {
        if (0==strcmp(p->name, name)) {
            return p;
        }
    }
    return 0;
}

/** string val to idx */
const cmdl_ref_t* sim_sval_2_ref(const cmdl_ref_t *refs, const char* sval)
{
    if (refs == 0 || sval == 0) { return 0; }

    for (const cmdl_ref_t *p = refs; p->name; ++p) {
        if (0==strcmp(p->val, sval)) {
            return p;
        }
    }
    return 0;
}

/** int val to idx */
const cmdl_ref_t* ref_ival_2_idx(cmdl_ref_t *refs, int val)
{
    char s[256] = {0};

    if (refs == 0) { return 0; }

#ifdef WIN32
    _snprintf(s, 256, "%d", val);
#else
	snprintf(s, 256, "%d", val);
#endif
    return sim_sval_2_ref(refs, s);
}

const cmdl_enum_t* sim_ival_2_enum(const cmdl_enum_t* enums, int val)
{
    if (enums == 0) { return 0; }

    for (const cmdl_enum_t *p = enums; p->name; ++p) {
        if (p->val == val) {
            return p;
        }
    }
    return 0;
}

const cmdl_enum_t* sim_sval_2_enum(const cmdl_enum_t* enums, const char *sval)
{
    if (enums == 0 || sval == 0) { return 0; }

    int ival = atoi(sval);
    return sim_ival_2_enum(enums, ival);
}

const cmdl_enum_t* sim_name_2_enum(const cmdl_enum_t* enums, const char *name)
{
    if (enums == 0 || name == 0) { return 0; }

    for (const cmdl_enum_t *p = enums; p->name; ++p) {
        if (0 == strcmp(p->name, name)) {
            return p;
        }
    }
    return 0;
}

cmdl_iter_t  cmdl_iter_init(int argc, char **argv, int start)
{
    cmdl_iter_t iter = { argv, argc, start };
    return iter;
}

int     cmdl_iter_idx (cmdl_iter_t *iter)
{
    return iter->idx;
}

/** argv[ith] */
char*   cmdl_get_ith(cmdl_iter_t *iter, int i)
{
    if (iter->argv && i >= 0 && i < iter->argc) {
        return iter->argv[i];
    }
    return 0;
}

char*   cmdl_iter_1st (cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, iter->idx = iter->start);
}

char*   cmdl_iter_next(cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, ++ iter->idx);
}

char*   cmdl_iter_last(cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, iter->idx = iter->argc - 1);
}

char*   cmdl_iter_prev(cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, -- iter->idx);
}

char*   cmdl_iter_curr(cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, iter->idx);
}

char*   cmdl_peek_next(cmdl_iter_t *iter)
{
    return cmdl_get_ith(iter, 1 + iter->idx);
}

char*   cmdl_peek_ith (cmdl_iter_t *iter, int ith)
{
    return cmdl_get_ith(iter, ith + iter->idx);
}

char*   cmdl_iter_pop(cmdl_iter_t *iter, int b_opt)
{
    char *s = cmdl_iter_curr(iter);
    if (!s || (b_opt && s[0]!='-') || (!b_opt && s[0]=='-')) {
        return 0;
    }
    cmdl_iter_next(iter);
    return s;
}

cmdl_iter_t  cmdl_layer_init(int layer)
{
    cmdl_iter_t iter = { 0, 0, 0, 0, layer };
    return iter;
}

int cmdl_parse_range (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        char *flag=0;
        char *last=0;
        char *arg = cmdl_peek_next(iter);
        if (!arg || arg[0] == '-') {
            xerr("%s has no arg\n", cmdl_iter_curr(iter));
            return CMDL_RET_ERROR;
        } else {
            cmdl_iter_next(iter);
        }

        int *i_range = dst;
        i_range[0] = 0;
        i_range[1] = INT_MAX;

        if (arg[0] != '~' &&  arg[1] != '+') {
            /* parse argv : `$start[~$last]` or `$start[+$count]` */
            flag = get_uint32 (arg, &i_range[0]);
            if (flag==0 || *flag == 0) {       /* no `~$last` or `+$count` */
                return 2;
            }
        }

        /* get `~$last` or `+$count` */
        if (*flag != '~' && *flag != '+') {
            xerr("<cmdl> Err : Invalid flag\n");
            return CMDL_RET_ERROR;
        }
        
        last = get_uint32 (flag + 1, &i_range[1]);
        if (last == 0 || *last != 0 ) {
            xerr("<cmdl> Err : Invalid count/end\n");
            i_range[1] = INT_MAX;
            return -1;
        }
        
        if (*flag == '+') {
            i_range[1] += i_range[0];
        }
        
        return 2;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        int *range = (int *)dst;
        xprint("'%d~%d';\n", range[0], range[1]);
    }
    else if (act == CMDL_PRI_HELP)
    {
        xlprint(iter->layer+1, "`$start[~$last]`  or `[$start]~$last`\n");
        xlprint(iter->layer+1, "`$start[+$count]` or `[$start]+$count`\n");
    }

    return 0;
}

int cmdl_parse_str   (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        char *arg = cmdl_peek_next(iter);
        if (!arg || arg[0] == '-') {
            xerr("%s has no arg\n", cmdl_iter_curr(iter));
            return CMDL_RET_ERROR;
        } else {
            cmdl_iter_next(iter);
        }
        *((char **)dst) = arg;
        return 2;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        xprint("'%s'", *((char **)dst));
    }
    else if (act == CMDL_PRI_ARGFMT) {
        xprint("<%%s>");
    }

    return 0;
}

int cmdl_parse_strcpy(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        char *arg = cmdl_peek_next(iter);
        if (!arg || arg[0] == '-') {
            xerr("%s has no arg\n", cmdl_iter_curr(iter));
            return CMDL_RET_ERROR;
        } else {
            cmdl_iter_next(iter);
        }
        strcpy(*((char **)dst), arg);
        return 2;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        xprint("'%s'", *((char **)dst));
    }
    else if (act == CMDL_PRI_ARGFMT) {
        xprint("<%%s>");
    }

    return 0;
}

int cmdl_parse_int   (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        char *arg = cmdl_peek_next(iter);
        if (!arg || arg[0]=='-') {
            if (opt->narg==0) {
                arg = SAFE_STR(opt->default_val, "1");
            } else {
                xerr("%s has no arg\n", cmdl_iter_curr(iter));
                return CMDL_RET_ERROR;
            }
        } else {
            cmdl_iter_next(iter);
        }
        int b_err = str_2_int(arg, dst);
        return b_err ? CMDL_RET_ERROR : 2;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        xprint("'%d'", *((int*)dst));
    }
    else if (act == CMDL_PRI_ARGFMT)
    {
        xprint("<%%d>");
    }

    return 0;
}

int cmdl_parse_ints  (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    int i = 0;
    if (act == CMDL_ACT_PARSE)
    {
        for (i=0; i<opt->narg; ++i) {
            char *arg = cmdl_iter_next(iter);
            if (!arg || /*b_err=*/str_2_int(arg, &((int*)dst)[i])) {
                xerr(arg ? "str2i() failed\n" : "noarg\n");
                cmdl_iter_dbg (iter);
                return CMDL_RET_ERROR;
            }
        }
        return opt->narg + 1;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        for (i=0; i<opt->narg; ++i) {
            xprint("%s'%d'", i?"":",", ((int*)dst)[i]);
        }
    }
    else if (act == CMDL_PRI_ARGFMT) {
        xprint("<%%d,%%d,...>");
    }

    return 0;
}

int cmdl_parse_xlevel(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        char *arg = cmdl_iter_curr(iter);
        opt->narg = 0;
        
        if (0==strcmp(arg, "-xnon")) {
            xlog_set_level(SLOG_NON);
            return 1;
        } 

        if (0==strcmp(arg, "-xall")) {
            xlog_set_level(SLOG_ALL);
            return 1;
        } 

        if (0==strcmp(arg, "-xl") || 0==strcmp(arg, "-xlevel")) {
            int level;
            opt->narg = 1;
            int r = cmdl_parse_int(iter, &level, act, opt);
            if (r>=0) {
                xlog_set_level(level);
                return 2;
            }
        }
        
        return CMDL_RET_ERROR;
    }
    else if (act == CMDL_PRI_RESULT)
    {
        xprint("'%d'", xlog_get_level());
    }
    else if (act == CMDL_PRI_HELP)
    {
        xlprint(iter->layer+1, "-xall,             log all\n");
        xlprint(iter->layer+1, "-xnon,             log non\n");
        xlprint(iter->layer+1, "-xl,xlevel <%%d>,   set log level\n");
    }
    
    return 0;
}

int cmdl_parse_help  (cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    if (act == CMDL_ACT_PARSE)
    {
        return CMDL_RET_HELP;
    }

    return 0;
}

/* Get opt by ref_name: user type "-%ref_name" in cmdl */
cmdl_opt_t* cmdl_ref_2_opt (cmdl_opt_t optv[], const char *ref_name)
{
    for (cmdl_opt_t* opt = optv; opt->names; ++opt)
    {
        /* We can't sure if ref or enum is used, or neither. So just try */
        if (sim_name_2_ref(opt->refs, ref_name)) { return opt; }
        if (sim_name_2_enum(opt->enums, ref_name)) { return opt; }
    }
    
    return 0;
}

cmdl_opt_t* cmdl_name_2_opt(cmdl_opt_t optv[], const char *opt_name)
{
    for (cmdl_opt_t* opt = optv; opt->names; ++opt) {
        //xdbg("%d: %s, %s\n", i, name, optv[i].names);
        if (field_in_record(opt_name, opt->names)) {
            return opt;
        }
    }
    return 0;
}

cmdl_opt_t* cmdl_set_ref(cmdl_opt_t optv[], const char *opt_name, const cmdl_ref_t *refs)
{
    cmdl_opt_t* opt = cmdl_name_2_opt(optv, opt_name);
    if (opt) {
        opt->refs = refs;
    }
    return opt;
}

cmdl_opt_t* cmdl_set_enum(cmdl_opt_t optv[], const char *opt_name, const cmdl_enum_t *enums)
{
    cmdl_opt_t* opt = cmdl_name_2_opt(optv, opt_name);
    if (opt) {
        opt->enums = enums;
    }
    return opt;
}

static
int cmdl_parse_opt(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t* opt)
{
    cmdl_iter_t _iter2;
    cmdl_iter_t *iter2 = iter;
    int   fieldCount = 0;
    char *fieldArr[256] = {cmdl_iter_curr(iter)};
    char  fieldBuf[1024] = {0};

    char *arg = cmdl_peek_next(iter);
    xlog_cmdl("[L%d] parse_opt(-%s) : %s\n", iter->layer, opt->names, SAFE_STR(arg, "?"));
    
    if (arg && arg[0] == '%') 
    {
        /* expand ref or enum */
        if (opt->refs) {
            opt->p_ref = sim_name_2_ref(opt->refs, &arg[1]);
            if (opt->p_ref == 0) {
                xerr("can't find -%s.%s as ref\n", opt->names, arg);
                return CMDL_RET_ERROR;
            }
            
            strncpy(fieldBuf, opt->p_ref->val, 1024);
            fieldCount = str_2_fields(fieldBuf, 255, fieldArr+1);
            if (fieldCount>=255) {
                xerr("strspl() overflow\n");
                return CMDL_RET_ERROR;
            }

            _iter2 = cmdl_iter_init(fieldCount+1, fieldArr, 0);
            iter2 = &_iter2;
            iter2->layer = iter->layer;
        } else if (opt->enums) {
            opt->p_enum = sim_name_2_enum(opt->enums, &arg[1]);
            if (opt->p_enum == 0) {
                xerr("can't find -%s.%s as enum\n", opt->names, arg);
                return CMDL_RET_ERROR;
            }
            *((int *)dst) = opt->p_enum->val;
            return 2;
        } else {
            xerr("can't expand -%s.%s neither as ref nor enum\n", opt->names, arg);
            return CMDL_RET_ERROR;
        }
    }

    xlog_cmdl("[L%d] parse(-%s) : %s\n", iter2->layer, opt->names, SAFE_STR(arg, "?"));
    int r = opt->parse(iter2, dst, CMDL_ACT_PARSE, opt);
    if (r < 0) {
        return r;
    }
    
    return (arg && arg[0] == '%') ? 1 : r;
}

int cmdl_init(cmdl_opt_t optv[])
{
    ENTER_FUNC();
    
    int n_err = 0;
    for (cmdl_opt_t *opt = optv; opt->names; ++opt)
    {
        // check
        if (!opt->names || !opt->names[0] ) {
            xerr("null optiion name\n");
            -- n_err;
        }
        if (!opt->parse) {
            xerr("-%s.parse() has no implementation\n", opt->names);
            -- n_err;
        }
        
        // init
        opt->n_parse    = 0;
        opt->argvIdx    = -1;
        opt->b_default  = 0;
        opt->p_ref      = 0;
        opt->p_enum     = 0;
    }

    LEAVE_FUNC();
    
    return n_err;
}

int cmdlgrp_parse(cmdl_iter_t *iter, void* dst, cmdl_opt_t optv[])
{
    ENTER_FUNC();

    int ret = cmdl_init(optv);
    if (ret < 0) {
        xerr("invalid cmdl description defined (%d)\n\n", ret);
        return CMDL_RET_ERROR_INIT;
    }

    int old_idx = cmdl_iter_idx(iter);

    ++ iter->layer;
    while (cmdl_iter_next(iter))
    {
        char *arg = cmdl_iter_curr(iter);
        xlog_cmdl("[L%d] argv[%d]=(%s): \n", iter->layer, cmdl_iter_idx(iter), arg);
        
        if (arg[0]!='-') {
            xlog_cmdl("argv[%d] (%s) is not opt\n", cmdl_iter_idx(iter), arg);
            cmdl_iter_prev(iter);
            ret = CMDL_RET_NOT_OPT;
            break;
        }
        
        cmdl_opt_t *p_opt = 0;
        cmdl_iter_t *iter2 = iter;
        char *ref_expand[2] = {0};
        cmdl_iter_t _iter2 = cmdl_iter_init(2, ref_expand, 0);
        
        if (arg[1] == '%') {                                        /* -%ref or -%enum */
            p_opt = cmdl_ref_2_opt(optv, &arg[2]);
        } else {                                                    /* -opt arg ... */
            p_opt = cmdl_name_2_opt(optv, &arg[1]);
        }
        if (p_opt == 0) {
            xlog_cmdl("argv[%d] (%s) unrecognized\n", cmdl_iter_idx(iter), arg);
            cmdl_iter_prev(iter);
            ret = CMDL_RET_UNKNOWN;
            break;
        }

        ++ p_opt->n_parse;
        p_opt->argvIdx = cmdl_iter_idx(iter);
        
        if (arg[1] == '%') {
            ref_expand[0] = arg;        /* use original opt */
            ref_expand[1] = arg + 1;    /* use `%ref' or `%enum' as arg */
            iter2 = &_iter2;
            iter2->layer = iter->layer;
        } 

        ret = cmdl_parse_opt(iter2, (char *)dst + p_opt->dst_offset, CMDL_ACT_PARSE, p_opt);
        if (ret <= CMDL_RET_HELP) {
            break;
        }
    }
    -- iter->layer;

    if (ret > CMDL_RET_HELP) {
        ret = cmdl_check(iter, dst, optv);
        if (ret < 0) {
            xerr("cmdl_check() failed (%d)\n\n", ret);
            return CMDL_RET_ERROR_CHECK;
        }
    }
   
    LEAVE_FUNC();
    
    return (ret<=0)? ret : cmdl_iter_idx(iter) - old_idx;
}

/**
 * @param [in] iter - just for layer trace
 */
int cmdl_check(cmdl_iter_t *iter, void* dst, cmdl_opt_t optv[])
{
    int n_err = 0;
    char *argv[2] = {0};
    
    ENTER_FUNC();

    for (cmdl_opt_t *opt = optv; opt->names; ++opt)
    {
        if (opt->n_parse > 0) {
            continue;
        }
        xdbg("<cmdl> -%-10s has never been set\n", opt->names);
        if (opt->b_required) {
            xerr("%s not specified\n", opt->names);
            -- n_err;
        } else if (opt->default_val) {
            xlog_cmdl("try default_val for -%s\n", opt->names);
            argv[0] = opt->names;
            argv[1] = opt->default_val;
            cmdl_iter_t iter2 = cmdl_iter_init(2, argv, 0);
            iter2.layer = 1 + iter->layer;
            int r = cmdl_parse_opt(&iter2, (char *)dst + opt->dst_offset, CMDL_ACT_PARSE, opt);
            if (r < 0) {
                xerr("%s fail to use default_val\n", opt->names);
                -- n_err;
            } 
        }
    }
    
    LEAVE_FUNC();
    
    return n_err;
}


void cmdl_layer_prefix(int layer)
{
    int i;
    for (i=0; i<layer; ++i) {
        xprint("    ");
    }
}

void cmdl_iter_dbg (cmdl_iter_t *_iter)
{
    cmdl_iter_t iter = *_iter;
    for (cmdl_iter_1st(&iter); cmdl_iter_curr(&iter); cmdl_iter_next(&iter)) {
        xdbg("%s\n", cmdl_iter_curr(&iter));
    }
}

/**
 * @param iter just for layer trace
 */
int cmdlgrp_print_help(cmdl_iter_t *iter, void* null, cmdl_opt_t optv[])
{
    ENTER_FUNC();

    ++ iter->layer;
    for (cmdl_opt_t *opt = optv; opt->names; ++opt)
    {
        xlprint(iter->layer, "-%s ",opt->names);
        opt->parse(iter, 0, CMDL_PRI_ARGFMT, opt);
        if (opt->default_val) {
            xprint(", [%s]", opt->default_val);
        }
        xprint(", \t%s\n", SAFE_STR(opt->help, ""));
        opt->parse(iter, 0, CMDL_PRI_HELP, opt);

        if (opt->refs) {
            for (const cmdl_ref_t *r = opt->refs; r->name; ++r) {
                xlprint(iter->layer+1, "%%%-10s \t `%s`\n", r->name, r->val);
            }
        }
        if (opt->enums) {
            for (const cmdl_enum_t *e = opt->enums; e->name; ++e) {
                xlprint(iter->layer+1, "%%%-10s \t `%d`\n", e->name, e->val);
            }
        }
    }
    -- iter->layer;
    
    LEAVE_FUNC();
    
    return 0;
}

/**
 * @param iter just for layer trace
 */
int cmdlgrp_print_result(cmdl_iter_t *iter, void* dst, cmdl_opt_t optv[])
{
    ENTER_FUNC();

    xprint("\n");
    ++ iter->layer;
    for (cmdl_opt_t *opt = optv; opt->names; ++opt)
    {
        xlprint(iter->layer, "-%s = ", opt->names);
        opt->parse(iter, (char *)dst + opt->dst_offset, CMDL_PRI_RESULT, opt);

        //FIXME: The following way to get ref.name or enum.name make no sense when:
        //  1. optv[] was defined in local scope. 
        //  2. optv[] was reused by diff params.
        if (opt->p_ref) {
            xprint(" (%%%s)", opt->p_ref->name);
        }
        
        if (opt->p_enum) {
            xprint(" (%%%s)", opt->p_enum->name);
        }
        xprint("\n");
    }
    -- iter->layer;
    
    LEAVE_FUNC();
    
    return 0;
}
