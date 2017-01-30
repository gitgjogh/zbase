#include "sim_opt.h"

const opt_enum_t color_enum[] =
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
    
    if (act == CMDL_ACT_PARSE) {
        return cmdl_parse(iter, dst, pet_opt);
    }
    else if (act == CMDL_ACT_HELP) {
        return cmdl_help(iter, 0, pet_opt);
    }
    else if (act == CMDL_ACT_RESULT) {
        return cmdl_result(iter, dst, pet_opt);
    }
    
    return 0;
}

int cmdl_test(int argc, char **argv)
{
    typedef struct cmdl_param {
        pet_t cat, dog;
    } cmdl_param_t;
    
#define CMDL_OPT_M(member) offsetof(cmdl_param_t, member)
    cmdl_opt_t cmdl_opt[] =
    {
        { 0, "h,help", 0, cmdl_parse_help,         0,      0,  "show help"},
        { 1, "dog", 1, cmdl_pet_parser,  CMDL_OPT_M(dog),  0,  "dog's prop...", },
        { 1, "cat", 1, cmdl_pet_parser,  CMDL_OPT_M(cat),  0,  "cat's prop...", },
        { 0, SIM_NULL_END, },
    };
    
    cmdl_param_t cfg;
    cmdl_iter_t iter = cmdl_iter_init(argc, argv, 0);
    int r = cmdl_parse(&iter, &cfg, cmdl_opt);
    if (r == CMDL_RET_HELP) {
        return cmdl_help(&iter, 0, cmdl_opt);
    } else if (r < 0) {
        xerr("cmdl_parse() failed, ret=%d\n", r);
        return 1;
    }
    
    cmdl_result(&iter, &cfg, cmdl_opt);
    
    return 0;
}
