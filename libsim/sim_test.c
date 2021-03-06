#include "sim_opt.h"

const cmdl_enum_t color_enum[] =
{
    {"white",   0},
    {"black",   1},
    {"green",   2},
    {"yellow",  6},
    {SIM_NULL_END}
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
    { 0, SIM_NULL_END },
};

int cmdl_pet_parser(cmdl_iter_t *iter, void* dst, CMDL_ACT_e act, cmdl_opt_t *opt)
{
    cmdl_set_enum(pet_opt, "color", color_enum);
    
    return cmdlgrp_default_entry(iter, dst, act, pet_opt);
}

int cmdl_test(int argc, char **argv)
{
    typedef struct cmdl_param {
        pet_t cat, dog;
        int  range[2];
    } cmdl_param_t;
    
#define CMDL_OPT_M(member) offsetof(cmdl_param_t, member)
    cmdl_opt_t cmdl_opt[] =
    {
        { 0, "h,help", 0, cmdl_parse_help,         0,      0,  "show help"},
        { 0, "dog", 1, cmdl_pet_parser,  CMDL_OPT_M(dog),  0,  "dog's prop...", },
        { 0, "cat", 1, cmdl_pet_parser,  CMDL_OPT_M(cat),  0,  "cat's prop...", },
        { 1, "r",   1, cmdl_parse_range, CMDL_OPT_M(range),0,  "just for range test", },
        { 0, SIM_NULL_END, },
    };
    
    cmdl_param_t cfg;
    cmdl_iter_t iter = cmdl_iter_init(argc, argv, 0);
    int r = cmdlgrp_default_entry(&iter, &cfg, CMDL_ACT_PARSE, cmdl_opt);
    if (r == CMDL_RET_HELP) {
        return cmdlgrp_default_entry(&iter, 0, CMDL_PRI_HELP, cmdl_opt);
    } else if (r < 0) {
        xerr("cmdl_parse() failed, ret=%d\n", r);
        return 1;
    }
    
    cmdlgrp_default_entry(&iter, &cfg, CMDL_PRI_RESULT, cmdl_opt);
    
    return 0;
}

#ifdef _SIM_TEST_
int main(int argc, char **argv)
{
    xlog_init(SLOG_DEFAULT);
    return cmdl_test(argc, argv);
}
#endif
