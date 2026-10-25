/* The postmortem retains the actual failing child mapping and survives mapping destruction. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"
#include "flashvm/crash.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include <cstring>


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    const unsigned char body[] = {0x96, 5,    0,    7,  0, 0,  0, 0,    0x96, 8, 0, 0,  'O', 'b', 'j', 'e',  'c', 't',
                                  0,    0x40, 0x94, 12, 0, 10, 0, 0x96, 5,    0, 7, 42, 0,   0,   0,   0x2a, 0,   0};
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_Object *fn;
    FVM_JitCode *code;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_configure(c, FVM_CRASH_ZIP_CODE, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_create_avm1(c, body, sizeof(body), 0, 0, 0, &fn) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_compile(body, sizeof(body), &code) == FVM_OK && code->blocks.size() == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, fvm_jit_body, code, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value out;
    if (!(fvm_function_invoke(c, fn, 0, 0, 0, &out) == FVM_EXCEPTION))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_CrashFrame frame;
    if (!(fvm_crash_frame(c, 0, &frame) == FVM_OK && frame.jit))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.machine_address == code->blocks[0].code->memory &&
          frame.machine_code_size == code->blocks[0].code->bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(frame.machine_code, code->blocks[0].code->memory, frame.machine_code_size)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.bytecode_size == sizeof(body) && !std::memcmp(frame.bytecode, body, sizeof(body))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_crash_frame(c, 0, &frame) == FVM_OK && frame.machine_code_size))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->crash.active && !c->crash.busy))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_exception_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
