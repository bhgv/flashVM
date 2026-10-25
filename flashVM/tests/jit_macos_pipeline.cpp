/* Run Darwin-framed AVM1 and collecting callbacks through real Linux RW/RX syscalls. */
#include <stdio.h>
#include <stdlib.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "../src/jit/internal.hpp"
#include <cstring>

#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
#include <sys/mman.h>
#include <unistd.h>

#define FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_SUPPORTED 1
#define FVM_MAC_CODE_SDK_H


static void fixture_cache(void *address, size_t size)
{
    __builtin___clear_cache((char *)address, (char *)address + size);
}

#define sys_icache_invalidate fixture_cache
#define fvm_port_code_supported fixture_code_supported
#define fvm_port_code_create fixture_code_create
#define fvm_port_code_release fixture_code_release
#include "../ports/mac/code.c"

#undef fvm_port_code_supported
#undef fvm_port_code_create
#undef fvm_port_code_release
#undef sys_icache_invalidate


static bool fixture_resolve(FVM_JitStep step, unsigned char *address, void *)
{
    std::memcpy(address, &step, sizeof(step));
    return true;
}


static FVM_Result fixture_compile(const void *actions, size_t size, FVM_JitCode **out)
{
    FVM_JitCode *code = new FVM_JitCode;
    code->source.resize(size);
    std::memcpy(code->source.begin(), actions, size);
    FVM_Result result = fvm_jit_decode(code->source.begin(), size, *code, 0, false);
    fvm_detail::vector<unsigned char> bytes;
    if (result == FVM_OK)
    {
        result = fvm_jit_encode(FVM_JIT_TARGET_MACOS_X64, *code, bytes, fixture_resolve, 0);
    }
    if (result == FVM_OK)
    {
        result = fixture_code_create(bytes.begin(), bytes.size(), &code->memory, &code->mapped);
    }
    if (result != FVM_OK)
    {
        delete code;
        return result;
    }
    code->bytes = bytes.size();
    *out = code;
    return FVM_OK;
}

/* Reuse actual callback/GC assertions with only compilation replaced by the Darwin pipeline. */
#define fvm_jit_compile fixture_compile
#define main darwin_callback_reference_main
#include "jit_callbacks.cpp"

#undef main
#undef fvm_jit_compile
#endif


int main()
{
#if defined(__linux__) && defined(__x86_64__) && !defined(__ILP32__)
    if (!(darwin_callback_reference_main() == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
