/* Public crash/JIT views compile as strict C89 and preserve typed source bytes. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/crash.h"
#include "flashvm/source_debug.h"
#include "flashvm/debug.h"
#include "flashvm/jit.h"
#include <string.h>


int main(void)
{
    const unsigned char source[] = {0x96, 5, 0, 7, 42, 0, 0, 0, 0x2a, 0};
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    FVM_JitCode *code;
    const void *bytes;
    size_t size, count;
    FVM_Result status, diagnostic;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_configure(c, FVM_CRASH_NONE, 0, 0, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_set_detail(c, FVM_CRASH_DETAIL_FORCE) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_set_detail(c, (FVM_CrashDetail)2) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_configure(c, FVM_CRASH_DEBUGGER, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_info(c, &count, &status, &diagnostic) == FVM_NOT_FOUND && count == 0 && diagnostic == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_crash_clear(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_source_count(c, &count) == FVM_OK && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_clear_symbols(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_frame_count(c, &count) == FVM_NOT_FOUND && count == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (fvm_jit_available())
    {
        if (!(fvm_jit_compile(source, sizeof(source), &code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_source(code, &bytes, &size) == FVM_OK && size == sizeof(source) && !memcmp(bytes, source, size)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_machine_code(code, &bytes, &size) == FVM_OK && bytes && size))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_jit_destroy(code) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
