#include "sim_opt.h"

int main(int argc, char **argv)
{
    xlog_init(SLOG_DEFULT);
    
    typedef struct pet{
        char *name;
        int  color;     /* @see color_t */
        int  age;
    } pet_t;
    
    #define PET_OPT_M(member) offsetof(pet_t, member)
    cmdl_opt_t pet_opt[] = {
        { 0, "h,help", 0, cmdl_parse_help,         0,      0,  "show help"},
        { 1, "name",  1, cmdl_parse_str,  PET_OPT_M(name),  0,         "pet's name",  },
        { 0, "color", 1, cmdl_parse_int,  PET_OPT_M(color), "0",        "pet's color", },
        { 0, "age",   1, cmdl_parse_int,  PET_OPT_M(age),   "0",       "pet's age",   },
    };

    pet_t cfg;
    cmdl_iter_t iter = cmdl_iter_init(argc, argv, 0);
    int r = cmdl_parse(&iter, &cfg, ARRAY_TUPLE(pet_opt));
    if (r == CMDL_RET_HELP) {
        return cmdl_help(&iter, 0, ARRAY_TUPLE(pet_opt));
    } else if (r < 0) {
        xerr("cmdl_parse() failed, ret=%d\n", r);
        return 1;
    }

    cmdl_result(&iter, &cfg, ARRAY_TUPLE(pet_opt));

    return 0;
}