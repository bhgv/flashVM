/* Run the exact Windows code adapter against a deterministic SDK boundary; not an OS run. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"
#include <cstdlib>
#include <cstring>

#define FVM_WINDOWS_CODE_SDK_H
#define MEM_COMMIT 0x1000U
#define MEM_RESERVE 0x2000U
#define MEM_RELEASE 0x8000U
#define PAGE_READWRITE 4U
#define PAGE_EXECUTE_READ 0x20U
#define ERROR_NOT_ENOUGH_MEMORY 8U
#define ERROR_OUTOFMEMORY 14U
typedef unsigned int DWORD;
typedef size_t DWORD64;
typedef size_t ULONG_PTR;


typedef struct
{
    DWORD BeginAddress, EndAddress, UnwindData;
} RUNTIME_FUNCTION;

typedef RUNTIME_FUNCTION *PRUNTIME_FUNCTION;
static unsigned failure = 0, last_error = 8;
static bool registered = false, executable = false;
static void *allocated = 0;
static size_t allocation_size = 0;
static char events[32];
static size_t event_count = 0;


static void event(char c)
{
    if (!(event_count + 1 < sizeof(events)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events[event_count++] = c;
    events[event_count] = 0;
}


static void *VirtualAlloc(void *base, size_t size, DWORD flags, DWORD protect)
{
    event('V');
    if (!(!base && flags == (MEM_COMMIT | MEM_RESERVE) && protect == PAGE_READWRITE && !allocated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 1)
    {
        return 0;
    }
    allocated = std::calloc(size, 1);
    if (!(allocated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allocation_size = size;
    executable = false;
    return allocated;
}


static DWORD GetLastError()
{
    return last_error;
}


static int VirtualProtect(void *base, size_t size, DWORD protect, DWORD *old)
{
    event('P');
    if (!(base == allocated && size == allocation_size && protect == PAGE_EXECUTE_READ && !executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 2)
    {
        return 0;
    }
    *old = PAGE_READWRITE;
    executable = true;
    return 1;
}


static void *GetCurrentProcess()
{
    return (void *)1;
}


static int FlushInstructionCache(void *process, const void *base, size_t size)
{
    event('C');
    if (!(process == (void *)1 && base == allocated && size < allocation_size && executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return failure != 3;
}


static int RtlAddFunctionTable(PRUNTIME_FUNCTION table, DWORD count, DWORD64 base)
{
    event('R');
    if (!(count == 1 && base == (ULONG_PTR)allocated && !registered && executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(sizeof(RUNTIME_FUNCTION) == 12 && table->BeginAddress == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!((unsigned char *)table == (unsigned char *)allocated + table->UnwindData + 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(table->UnwindData % 4 == 0 && table->UnwindData >= table->EndAddress &&
          table->UnwindData - table->EndAddress < 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char expected[] = {1, 13, 2, 0, 10, 0x32, 6, 0xc0};
    if (!(!std::memcmp((unsigned char *)allocated + table->UnwindData, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 4)
    {
        return 0;
    }
    registered = true;
    return 1;
}


static int RtlDeleteFunctionTable(PRUNTIME_FUNCTION table)
{
    event('D');
    if (!(registered && table && executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 5)
    {
        return 0;
    }
    registered = false;
    return 1;
}


static int VirtualFree(void *base, size_t size, DWORD flags)
{
    event('F');
    if (!(base == allocated && !registered && !size && flags == MEM_RELEASE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::free(allocated);
    allocated = 0;
    return 1;
}

#define fvm_port_code_supported fixture_code_supported
#define fvm_port_code_create fixture_code_create
#define fvm_port_code_release fixture_code_release
#include "../ports/windows/code.c"

#undef fvm_port_code_supported
#undef fvm_port_code_create
#undef fvm_port_code_release


static bool fixture_resolve(FVM_JitStep, unsigned char *address, void *)
{
    std::memset(address, 0, 8);
    address[4] = 1;
    return true;
}


int main()
{
#if defined(__x86_64__) || defined(_M_X64)
    const unsigned char code[] = {0xf3, 0x0f, 0x1e, 0xfa, 0x41, 0x54, 0x48, 0x83, 0xec, 32,
                                  0x49, 0x89, 0xcc, 0x48, 0x83, 0xc4, 32,   0x41, 0x5c, 0xc3};
    if (!(fixture_code_supported()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const char *expected[] = {"", "V", "VPF", "VPCF", "VPCRF"};
    for (failure = 1; failure <= 4; ++failure)
    {
        void *out = (void *)1;
        size_t mapped = 77;
        event_count = 0;
        events[0] = 0;
        if (!(fixture_code_create(code, sizeof(code), &out, &mapped) ==
              (failure == 1 ? FVM_OUT_OF_MEMORY : FVM_UNSUPPORTED)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(out == (void *)1 && mapped == 77 && !registered && !allocated && !std::strcmp(events, expected[failure])))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    failure = 1;
    last_error = 14;
    void *out = (void *)1;
    size_t mapped = 77;
    event_count = 0;
    if (!(fixture_code_create(code, sizeof(code), &out, &mapped) == FVM_OUT_OF_MEMORY && !allocated))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failure = 1;
    last_error = 5;
    event_count = 0;
    if (!(fixture_code_create(code, sizeof(code), &out, &mapped) == FVM_UNSUPPORTED && out == (void *)1 &&
          mapped == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failure = 0;
    event_count = 0;
    events[0] = 0;
    if (!(fixture_code_create(code, sizeof(code), &out, &mapped) == FVM_OK && out == allocated && registered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!std::memcmp(out, code, sizeof(code)) && !std::strcmp(events, "VPCR")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failure = 5;
    fixture_code_release(out, mapped);
    if (!(allocated && registered))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    failure = 0;
    fixture_code_release(out, mapped);
    if (!(!allocated && !registered && !std::strcmp(events, "VPCRDDF")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (size_t count = 1; count <= 4; ++count)
    {
        FVM_JitCode plan;
        plan.actions.resize(count);
        for (size_t i = 0; i < count; ++i)
        {
            plan.actions[i].branch = i + 1 == count ? FVM_JIT_EXIT : FVM_JIT_JUMP;
            plan.actions[i].target = count - 1;
        }
        fvm_detail::vector<unsigned char> generated;
        if (!(fvm_jit_encode(FVM_JIT_TARGET_WINDOWS_X64, plan, generated, fixture_resolve, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        event_count = 0;
        events[0] = 0;
        if (!(fixture_code_create(generated.begin(), generated.size(), &out, &mapped) == FVM_OK && registered))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(!std::memcmp(out, generated.begin(), generated.size())))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fixture_code_release(out, mapped);
        if (!(!allocated && !registered && !std::strcmp(events, "VPCRDF")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    event_count = 0;
    events[0] = 0;
    fixture_code_release(0, 0);
    if (!(!event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    out = (void *)1;
    mapped = 77;
    if (!(fixture_code_create(0, sizeof(code), &out, &mapped) == FVM_INVALID_ARGUMENT && out == (void *)1 &&
          mapped == 77))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned char bad[sizeof(code)];
    std::memcpy(bad, code, sizeof(code));
    bad[12] ^= 1;
    if (!(fixture_code_create(bad, sizeof(bad), &out, &mapped) == FVM_INVALID_ARGUMENT && !event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, (size_t)-1, &out, &mapped) == FVM_UNSUPPORTED && !event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, sizeof(code), 0, &mapped) == FVM_INVALID_ARGUMENT && !event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, sizeof(code), &out, 0) == FVM_INVALID_ARGUMENT && !event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
