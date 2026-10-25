/* Native forward/backward jumps, dynamic budgets and loaded underflow agree with interpretation. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/jit.h"
#include "flashvm/standard.h"


static void
check(FVM_Runtime *rt, unsigned profile, const unsigned char *bytes, size_t size, FVM_Result expected, double result)
{
    FVM_Context *c;
    FVM_Object *fn;
    FVM_JitCode *code = 0;
    FVM_Value a, b;
    unsigned char swf[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    if (!(fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (profile)
    {
        swf[3] = (unsigned char)(profile + 3);
        if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_function_create_avm1(c, bytes, size, 0, 0, 0, &fn) == FVM_OK &&
          fvm_function_invoke(c, fn, 0, 0, 0, &a) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(bytes, size, &code) == FVM_OK &&
          fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK &&
          fvm_function_invoke(c, fn, 0, 0, 0, &b) == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (expected == FVM_OK)
    {
        if (!(a.type == b.type))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (a.type == FVM_VALUE_NUMBER)
        {
            if (!(a.number == result && b.number == result))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
}


int main(void)
{
    const unsigned char loop[] = {0x96, 5, 0, 7,    3,    0,    0, 0, 0x96, 5,    0,    7, 1,
                                  0,    0, 0, 0x0b, 0x4c, 0x9d, 2, 0, 0xf1, 0xff, 0x3e, 0};
    const unsigned char forward[] = {0x99, 2, 0, 8, 0, 0x96, 5, 0, 7, 99, 0, 0, 0, 0x96, 5, 0, 7, 42, 0, 0, 0, 0x3e, 0};
    const unsigned char infinite[] = {0x99, 2, 0, 0xfb, 0xff, 0};
    const unsigned char empty_return[] = {0x3e, 0};
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile;
    if (!fvm_jit_available())
    {
        return 0;
    }
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (profile = 0; profile < 6; ++profile)
    {
        check(rt, profile, loop, sizeof(loop), FVM_OK, 0);
        check(rt, profile, forward, sizeof(forward), FVM_OK, 42);
        check(rt, profile, empty_return, sizeof(empty_return), profile ? FVM_OK : FVM_ERROR, 0);
    }
    fvm_runtime_destroy(rt);
    cfg.instruction_limit = 13;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check(rt, 0, loop, sizeof(loop), FVM_ERROR, 0);
    check(rt, 0, infinite, sizeof(infinite), FVM_ERROR, 0);
    fvm_runtime_destroy(rt);
    cfg.instruction_limit = 14;
    if (!(fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    check(rt, 0, loop, sizeof(loop), FVM_OK, 0);
    fvm_runtime_destroy(rt);
    return 0;
}
