sim_opt
=======

`sim_opt` is developed to reduce programer's work for argument parsing under command line interface.

Though POSIX provides `getopt()` function and GNU provides `getopt_long()` extension for basic arguments scanning and segment. The argument parsing work like storing the argument as string or convert argument into numerical (int, float i.e.) is still left to be done. Beside, developer also has to write a help for each options. That's really tedious work.

With `sim_opt` your mainly work is to defined a description array for options. 
For example, to allow users to specified the name/color/age for a pet, the developer just need to defined a `cmdl_opt_t` array like this:

    typedef struct pet{
        char *name;
        int  color;
        int  age;
    } pet_t;
    
    #define PET_OPT_M(member) offsetof(pet_t, member)
    cmdl_opt_t pet_opt[] = {
        { 0, "h,help", 0, cmdl_parse_help,         0,      0,  "show help"},
        { 1, "name",  1, cmdl_parse_str,  PET_OPT_M(name),  0,         "pet's name",  },
        { 0, "color", 1, cmdl_parse_int,  PET_OPT_M(color), "0",        "pet's color", },
        { 0, "age",   1, cmdl_parse_int,  PET_OPT_M(age),   "0",       "pet's age",   },
    };

Link the code to `libsim` and then you will got a program like this:

>   $ ./a.exe -h   
>      
    -h,help ,   show help   
    -name <%s>,         pet's name   
    -color <%d>, [0],   pet's color   
    -age <%d>, [0],     pet's age  
>
>   $ ./a.exe -name tom -color 2 -age 3
>
    -h,help =
    -name = 'tom'
    -color = '2'
    -age = '3'

The full sample is in `libsim/sim_test.c`. Built it like this:

>   cd libsim   
>   make   #make libsim.a   
>   cc sim_test.c -o a.exe -L./ -lsim