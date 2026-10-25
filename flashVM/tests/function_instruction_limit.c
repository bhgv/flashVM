#include <stdio.h>
#include <stdlib.h>
#include "flashvm/avm1.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"


int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *fn = 0;
    FVM_Value v;
    const unsigned char body[] = {0x96, 2,    0,    3, 3, 0x17, 0x17, 0x96, 2,    0, 3, 3,
                                  0x17, 0x17, 0x96, 2, 0, 3,    3,    0x17, 0x17, 0}; /* Nine nonterminal actions. */
    const unsigned char caller[] = {0x96, 5, 0, 7, 0, 0, 0, 0, 0x96, 4, 0, 0, 'f', 'n', 0, 0x3d, 0x17, 0};
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 11;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_value_undefined(&v);
    v.type = FVM_VALUE_OBJECT;
    v.object = fn;
    if (!(fvm_set_global(c, "fn", &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Both frames fit individually; their combined instruction count does not. */
    if (!(fvm_avm1_execute(c, caller, sizeof(caller)) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &v) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
