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

#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "sim_utils.h"
#include "sim_log.h"


int is_in_range(int min, int max, int v)
{
    return (((min)<=(max)) && ((min)<=(v)) && ((max)>=(v)));
}

int sat_div(int num, int den)
{
    return (num + den - 1)/den;
}


int bit_sat(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val+pad) & (~pad);
}


int is_bit_aligned(int nbit, int val)
{
    int pad = (1<<nbit) - 1;
    return (val&pad)==0;
}


#define SIM_LZB_BIT(bit)            \
    b   = val >= (1<<(1<<(bit)));   \
    n  += b ?  0  :  (1<<(bit)) ;   \
    val = b ?  (val>>(1<<(bit))) : val;
    
int num_leading_zero_bits_u64(uint64_t val)
{
    int n = 0;
    uint64_t    b;
    
    SIM_LZB_BIT(5)          /* get the 5-th bit of n */
    SIM_LZB_BIT(4)
    SIM_LZB_BIT(3)
    SIM_LZB_BIT(2)
    SIM_LZB_BIT(1)
    SIM_LZB_BIT(0)          /* get the 1-th bit of n */
   
    n  += (val<1);          /* get the carry bit of n */
    return n;
}

int num_leading_zero_bits_u32(uint32_t val)
{
    int n = 0;
    uint32_t    b;
    
    SIM_LZB_BIT(4)          /* get the 4-th bit of n */
    SIM_LZB_BIT(3)
    SIM_LZB_BIT(2)
    SIM_LZB_BIT(1)
    SIM_LZB_BIT(0)          /* get the 1-th bit of n */
   
    n  += (val<1);          /* get the carry bit of n */
    return n;
}


int clip(int v, int minv, int maxv)
{
    v = (v<minv) ? minv : v;
    v = (v>maxv) ? maxv : v;
    return v;
}

/**
 * simple atoi(/[0-9]+/) with the 1st non-digit pointer returned 
 *  @see str_2_uint() which is more delicate 
 */
char* get_uint32 (char *str, uint32_t *out)
{
    char  *curr = str;
    
    if ( curr ) {
        int  c, sum;
        for (sum=0, curr=str; (c = *curr) && c >= '0' && c <= '9'; ++ curr) {
            sum = sum * 10 + ( c - '0' );
        }
        if (out) { *out = sum; }
    }

    return curr;
}

/**
 *  \brief return the pos for 1st-char not in @jumpset
 */
int jump_front(const char* str, const char* jumpset)
{
    int c, start;
    for (start = 0; (c = str[start]); ++start) 
    {
        if ( !strchr(jumpset, c) ) {
            break;
        }
    }

    return start;
}

static 
int get_field_pos1(     char* str, 
                  const char* prejumpset, 
                  const char* delemiters,
                  int       * field_start)
{
    int c, start, end;
    start = end = 0;

    if (!str) {
        return 0;
    }

    if (prejumpset) 
    {
        for (start = 0; (c = str[start]); ++start) {
            if ( !strchr(prejumpset, c) ) {
                break;
            }
        }
    }

    if (c!=0 && field_start) {
        *field_start = start;
    }

    for (end = start; (c = str[end]); ++end) {
        if ( delemiters && strchr(delemiters, c) ) {
            break;
        }
    }
    
    return end-start;
}

static 
int get_field_pos2(     char* str, int len,
                  const char* prejumpset, 
                  const char* delemiters,
                  int       * field_start)
{
    int c, start, end;
    start = end = 0;
    
    if (!str) {
        return 0;
    }

    if (len <= 0) {
        return get_field_pos1(str,      prejumpset, delemiters, field_start);
    }

    if (prejumpset) 
    {
        for (start = 0; (c = str[start]) && start<len; ++start) {
            if ( !strchr(prejumpset, c) ) {
                break;
            }
        }
    }

    if (c!=0 && start<len && field_start) {
        *field_start = start;
    }

    for (end = start; (c = str[end]) && end<len; ++end) {
        if ( delemiters && strchr(delemiters, c) ) {
            break;
        }
    }
    
    return end-start;
}

int get_field_pos(      char* str, int len,
                  const char* prejumpset, 
                  const char* delemiters,
                  int       * field_start)
{
    if (len <= 0) {
        return get_field_pos1(str,      prejumpset, delemiters, field_start);
    } else {
        return get_field_pos2(str, len, prejumpset, delemiters, field_start);
    }
}

char *get_1st_field(char* str,  int len,
                    const char* prejumpset, 
                    const char* delemiters,
                    int       * field_len)
{
    int subpos = 0;
    int sublen = get_field_pos(str, len, prejumpset, delemiters, &subpos);
    if (field_len) {
        *field_len = sublen;
    }
    return sublen ? &str[subpos] : 0;
}

/**
 *  \brief get fields separated by blank or ','
 *  \param [in] record str would be separated
 *  \param [in] arrSz depth for @fieldArr[]
 *  \param [out] fieldArr fieldArr[i] return the i-th field
 *  \return num of fields
 */
int str_2_fields(char *record, int arrSz, char *fieldArr[])
{
    int nkey=0;
    char *substr = 0;
    int   sublen = 0;
    str_iter_t iter = str_iter_init(record, 0);
    WHILE_GET_FIELD(iter, " ,", " ,", substr) {
        if (nkey>0) {
            fieldArr[nkey-1][sublen] = 0;       /* terminate last substr */
        }
        fieldArr[nkey++] = substr;
        sublen = STR_ITER_GET_SUBLEN(iter);
    }
    fieldArr[nkey-1][sublen] = 0;               /* terminate last substr */
    return nkey;
}

str_iter_t  str_iter_init(char *str, int len)
{
    str_iter_t iter = {str, len, 0, 0};
    return iter;
}

static
void  str_iter_reset(str_iter_t *iter)
{
    iter->substr = iter->str;
    iter->sublen = 0;
}

char *str_iter_1st_field(str_iter_t *iter, 
                    const char* prejumpset,
                    const char* delemiters)
{
    str_iter_reset(iter);
    
    if (!iter->str) {
        return 0;
    }
    
    iter->substr = get_1st_field(iter->str, iter->len, prejumpset, delemiters, &iter->sublen);
    
    return iter->substr;
}

char *str_iter_next_field(str_iter_t *iter, 
                    const char* prejumpset,
                    const char* delemiters)
{
    /** check last iter to in case dead loop */
    if (!iter->substr || !iter->sublen) {
        return 0;
    }
    /** whether reach null end or exceed strlen  */
    if ((iter->substr[iter->sublen] == 0) ||
        (iter->len > 0 && (iter->substr + iter->sublen >= iter->str + iter->len))) {
        iter->substr = 0;
        iter->sublen = 0;
        return 0;
    }
    iter->substr += (iter->sublen + 1);         /* jump last delemiters */
    iter->substr  = get_1st_field(iter->substr, iter->len, 
                                prejumpset, delemiters, &iter->sublen);
    return iter->substr;
}

const char *field_in_record(const char *field, const char *record)
{
    int flen = strlen(field);
    char *substr = 0;
    str_iter_t iter = str_iter_init(record, 0);
    WHILE_GET_FIELD(iter, " ,", " ,", substr) {
        if (flen == STR_ITER_GET_SUBLEN(iter) && 
            0 == strncmp(field, substr, flen)) {
            return substr;
        }
    }
    
    return 0;
}



/**
 *  @brief scan for uint64_t without numerical base prefix.
 *  
 *  @param [i] _str
 *  @param [o] ret_
 *  @param [o] end_
 *  @return error flag during scanning
 */
int scan_for_uint64_npre(const char *_str, uint32_t base, uint64_t max, uint64_t *ret_, const char **end_)
{
    uint64_t acc = 0;
    uint32_t c, v;
    uint32_t b_err = 0;
    uint64_t max_div;
    uint64_t max_mod;

    const char *end = _str;
    while (c = *end) 
    {
        v = isdigit(c) ? (c - '0'     ) : (
            islower(c) ? (c - 'a' + 10) : (
            isupper(c) ? (c - 'A' + 10) : 16) );
            
        if (v>=16) {
            xdbg("scan_for_uint64_base%d() stop at `0x%02x`\n", base, c);
            break;
        }

        /* took unwanted digit as err */
        if (v>=base) {
            xerr("`%c` is not charset for base %d\n", c, base);
            b_err |= SCAN_ERR_OUTOFSET;
            break;
        }

        max = max ? max : UINT64_MAX;
        max_div = max / base;
        max_mod = max % base;
        if ( !b_err ) {
            int b_of = (acc > max_div) || (acc == max_div && c > max_mod);
            if (b_of) {
                xerr("`%8s...` to uint64 would overflow\n", _str);
                b_err |= SCAN_ERR_OVERFLOW;
            }
            acc = b_err ? max : acc * base + v;
        }

        ++ end;
    }
    
    if (end<=_str) {
        xerr("`%4s...` not seems int type\n", _str);
        b_err |= SCAN_ERR_NOBEGIN;
    }

    if (ret_) { *ret_ = acc; }
    if (end_) { *end_ = end; }

    return b_err;
}

int scan_for_uint64_pre(const char *_str, uint64_t max, uint64_t *ret_, const char **end_)
{
    uint32_t base = 10;
    
    // first non space;
    while ( isspace(*_str) ) { ++_str; }
    
    if (*_str == '0') {
        uint32_t n = *(_str+1);
        if ( n=='x' || n=='X' ) {
            base = 16;  _str += 2;
        } else 
        if ( n=='b' || n=='B' ) {
            base = 2;   _str += 2;
        } else {
            base = 8;
        }
    }
    return scan_for_uint64_npre(_str, base, max, ret_, end_);
}

static int scan_for_uintN(const char *_str, int Nbit, uint64_t *ret_, const char **end_)
{
    uint64_t ret;
    uint64_t max;
    int b_err = 0;
    const char *end;
 
    // first non space;
    while ( isspace(*_str) ) { ++_str; }

    // sign flag
    if (*_str == '+') {
        ++_str;
    } 
    max = ( Nbit == 64 ? UINT64_MAX : (
            Nbit == 32 ? UINT32_MAX : (
            Nbit == 16 ? UINT16_MAX : (
            Nbit ==  8 ? UINT8_MAX : 0))));

    b_err = scan_for_uint64_pre(_str, max, &ret, &end);

    if (ret_) { *ret_ = ret; }
    if (end_) { *end_ = end; }

    return b_err;
}

static int scan_for_intN(const char *_str, int Nbit, int64_t *ret_, const char **end_)
{
    uint64_t ret;
    uint64_t max;
    uint32_t b_neg = 0;
    int b_err = 0;
    const char *end;
 
    // first non space;
    while ( isspace(*_str) ) { ++_str; }

    // sign flag
    if (*_str == '-' || *_str == '+') {
        b_neg = (*_str == '-');
        ++_str;
    } 
    max = ( Nbit == 64 ? INT64_MAX : (
            Nbit == 32 ? INT32_MAX : (
            Nbit == 16 ? INT16_MAX : (
            Nbit ==  8 ? INT8_MAX : 0))));
    max += (b_neg ? 1 : 0);

    b_err = scan_for_uint64_pre(_str, max, &ret, &end);

    if (ret_) { *ret_ = b_neg ? -ret : ret; }
    if (end_) { *end_ = end; }

    return b_err;
}

int scan_for_uint64(const char *_str, uint64_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 64, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_uint32(const char *_str, uint32_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 32, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_uint16(const char *_str, uint16_t *ret_, const char **end_)
{
    uint64_t ret;
    int b_err = scan_for_uintN(_str, 16, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int64(const char *_str, int64_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 64, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int32(const char *_str, int32_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 32, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int scan_for_int16(const char *_str, int16_t *ret_, const char **end_)
{
    int64_t ret;
    int b_err = scan_for_intN(_str, 16, &ret, end_);
    if (ret_) { *ret_ = ret; }

    return b_err;
}

int scan_for_float(const char *_str, float *ret_, const char **end_)
{
    double   ret;
    uint64_t r_int=0, r_frac=0;
     int32_t r_exp=0;                       // ret value
    uint32_t n_int=0, n_frac=0, n_exp=0;    // num of digit
    uint32_t e_int =SCAN_ERR_NOBEGIN;       // err flag
    uint32_t e_frac=SCAN_ERR_NOBEGIN;
    uint32_t e_exp =SCAN_ERR_NOBEGIN;
    uint32_t b_neg = 0;
    uint32_t b_err = 0;
    const char *end;
    
    // first non space;
    while ( isspace(*_str) ) { ++_str; }
    // sign flag
    if (*_str == '-' || *_str == '+') {
        b_neg = (*_str == '-');
        ++_str;
    }   
    
    e_int = scan_for_uint64_npre(_str, 10, INT_MAX, &r_int, &end);
    n_int = end - _str; 
    
    if (*end == '.') {
        _str = end + 1;
        e_frac = scan_for_uint64_npre(_str, 10, INT_MAX, &r_frac, &end);
        n_frac = end - _str;
    }
    
    if ( SCAN_ERR_NOBEGIN & (e_int & e_frac) ) {
        b_err = SCAN_ERR_NOBEGIN;
        goto str2float_end;
    } else {
        b_err = ( SCAN_ERR_OVERFLOW & (e_int | e_frac) );
    }
    
    if (*end == 'e' || *end == 'E') {
        _str   = end + 1;
        e_exp  = scan_for_int32(_str, &r_exp, &end);
        e_exp |= (r_exp>37 || r_exp<-37) ? SCAN_ERR_OVERFLOW : 0;
        b_err |= e_exp;
        r_exp  = e_exp ? 0 : r_exp;
    }
    
str2float_end:
    //xdbg("scan_for_float: int=%-10lld, frac=%lf, exp=%lf, ", 
    //    r_int, r_frac / pow(10, n_frac), pow(10, r_exp));

    ret = (double)r_int + (double)r_frac / pow((double)10.0, (double)n_frac);
    ret = ret * pow((double)10.0, (double)r_exp);

    if (ret_) { *ret_ = (float)(b_neg ? -ret : ret); }
    if (end_) { *end_ = end; }
    
    return b_err;
}

int str_2_uint(const char *_str, unsigned int *ret_)
{
    uint64_t ret;
    const char *end;
    int N = sizeof(unsigned int) * 8;
    int b_err = scan_for_uintN(_str, N, &ret, &end);
    if (!b_err && (*end != 0)) {
        xerr("str_2_uint(%s) not nul ending\n", _str);
        b_err |= SCAN_ERR_NNULEND;
    }
    if (ret_) { *ret_ = ret; }

    return b_err;
}
int str_2_int(const char *_str, int *ret_)
{
    int64_t ret;
    const char *end;
    int N = sizeof(unsigned int) * 8;
    int b_err = scan_for_intN(_str, N, &ret, &end);
    if (!b_err && (*end != 0)) {
        xerr("str_2_int(%s) not nul ending\n", _str);
        b_err |= SCAN_ERR_NNULEND;
    }
    if (ret_) { *ret_ = ret; }

    return b_err;
}

void mem_put_lte16(uint8_t *mem, uint32_t val) {
    mem[0] = ((val    ) && 0xff);
    mem[1] = ((val>> 8) && 0xff);
}
  
void mem_put_lte32(uint8_t *mem, uint32_t val) {
    mem[0] = ((val    ) && 0xff);
    mem[1] = ((val>> 8) && 0xff);
    mem[2] = ((val>>16) && 0xff);
    mem[3] = ((val>>24) && 0xff);
}


static void mem_swap32(void* base1, void* base2, uint32_t sz)
{
    uint32_t m, *p1 = base1, *p2 = base2;
    for ( ; sz>0; --sz, ++p1, ++p2) {
        m = *p1;
        *p1 = *p2;
        *p2 = m;
    }
}

void mem_swap(void* base1, void* base2, uint32_t elem_size, uint32_t cnt)
{
    uint32_t sz = elem_size * cnt;

    if (((uint32_t)base1&3) || ((uint32_t)base2&3) || (sz&3)) 
    {
        char m, *p1 = base1, *p2 = base2;
        for ( ; sz>0; --sz, ++p1, ++p2) {
            m = *p1;
            *p1 = *p2;
            *p2 = m;
        }
    } else {
        mem_swap32(base1, base2, sz/4);
    }
}

static void mem_swap_near_block_32(void* base, uint32_t sz, uint32_t shift)
{
    uint32_t i0 = 0;
    uint32_t i1 = 0;
    uint32_t *p = base;
    uint32_t tmp = p[i0];

    if (sz==0 || shift==0 || sz==shift) {
        return;
    }

    do {
        i0 = i1;
        i1 = WRAP_AROUND(sz, shift + i0);
        p[i0] = p[i1];
    } while (i1 != 0);
    p[i0] = tmp;
}

void mem_swap_near_block(void* base, uint32_t elem_size, 
                          uint32_t cnt1,  uint32_t cnt2)
{
    uint32_t sz = elem_size * (cnt1 + cnt2); 
    uint32_t shift = elem_size * cnt1;

    if (cnt1==0 || cnt2==0) {
        return;
    }

    if (((uint32_t)base&3) || (sz&3) || (shift&3)) 
    {
        uint32_t i0 = 0;
        uint32_t i1 = 0;
        char *p = base;
        char tmp = p[i0];
        do {
            i0 = i1;
            i1 = WRAP_AROUND(sz, shift + i0);
            p[i0] = p[i1];
        } while (i1 != 0);
        p[i0] = tmp;
    }
    else {
        mem_swap_near_block_32(base, sz/4, shift/4);
    }
}

/**
 *  \brief scan for the c-style token: [_a-zA-Z][_a-zA-Z0-9]*
 */
/*
int scan_for_token(const char *_str, int b_first_alpha, int *pos1_, int *pos2_, const char **end_)
{
    uint32_t ret1, ret2;
    const char *end = _str;
    uint32_t b_err = 0;
    uint32_t b_open;
    
    // first non space;
    while ( isspace(*end) ) { ++end; }
    ret1 = ret2 = (end - _str);
    b_open = b_first_alpha ? isalpha(*end) : isalnum(*end);
    
    // open
    if (b_open || *end=='_') {
        b_err = 0;
        ret1 = (end++) - _str;
    } else {
        b_err |= SCAN_ERR_NOBEGIN;
        goto scanforidentifier_end;
    }

    //close
    while (*end!=0 && (isalnum(*end) || *end=='_')) { ++end; }
    ret2 = end - _str;

scanforidentifier_end:
    if (pos1_) { *pos1_ = ret1; }
    if (pos2_) { *pos2_ = ret2; }
    if (end_)  { *end_  = end; }
    
    return b_err;
}
*/