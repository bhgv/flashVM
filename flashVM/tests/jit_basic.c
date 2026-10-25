/* C89 explicit native compilation agrees with interpreter activations and detaches cleanly. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/jit.h"
#include "flashvm/standard.h"
#include <string.h>


int main(void)
{
    const unsigned char bytes[] = {0x96, 3, 0, 0, 'x', 0, 0x1c, 0x96, 3, 0, 0, 'y', 0, 0x1c, 0x47, 0x3e, 0};
    const unsigned char text[] = {0x96, 7, 0, 0, 'h', 'e', 'l', 'l', 'o', 0, 0x3e, 0};
    const char *parameters[] = {"x", "y"};
    FVM_Config cfg;
    FVM_Runtime *rt;
    unsigned profile;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!fvm_jit_available())
    {
        FVM_JitCode *code = 0;
        if (!(fvm_jit_compile(bytes, sizeof(bytes), &code) == FVM_UNSUPPORTED && !code))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_runtime_destroy(rt);
        return 0;
    }
    for (profile = 0; profile < 6; ++profile)
    {
        FVM_Context *c;
        FVM_Object *fn;
        FVM_JitCode *code = 0;
        FVM_Value args[2], out, interpreted;
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
        if (!(fvm_standard_install(c) == FVM_OK &&
              fvm_function_create_avm1(c, bytes, sizeof(bytes), 2, parameters, 0, &fn) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 20);
        fvm_value_number(args + 1, 22);
        if (!(fvm_function_invoke(c, fn, 0, 2, args, &interpreted) == FVM_OK && interpreted.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_compile(bytes, sizeof(bytes), &code) == FVM_OK && fvm_jit_code_size(code) > 0 &&
              fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_invoke(c, fn, 0, 2, args, &out) == FVM_OK && out.type == interpreted.type &&
              out.number == interpreted.number))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK &&
              fvm_function_invoke(c, fn, 0, 2, args, &out) == FVM_OK && out.number == 42))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_create_avm1(c, text, sizeof(text), 0, 0, 0, &fn) == FVM_OK &&
              fvm_jit_compile(text, sizeof(text), &code) == FVM_OK &&
              fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK &&
              fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK &&
              out.type == FVM_VALUE_STRING && !strcmp(out.string, "hello")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(fvm_jit_destroy(0) == FVM_INVALID_ARGUMENT && fvm_jit_code_size(0) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
