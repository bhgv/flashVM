/* Exact Darwin adapter against controlled syscalls: no claim of actual macOS SDK/runtime execution. */
#include <stdio.h>
#include <stdlib.h>
#include "flashvm/flashvm.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>

#define FVM_MAC_CODE_PLATFORM_H
#define FVM_MAC_CODE_SUPPORTED 1
#define FVM_MAC_CODE_SDK_H
#define _SC_PAGESIZE 1
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANON 4096
#define MAP_FAILED ((void *)-1)
static unsigned failure;
static long page = 4096;
static void *allocation;
static size_t length;
static bool executable;
static char events[32];
static size_t count;


static void event(char c)
{
    if (!(count + 1 < sizeof(events)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events[count++] = c;
    events[count] = 0;
}


static long fixture_sysconf(int key)
{
    event('S');
    if (!(key == _SC_PAGESIZE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return page;
}


static void *fixture_mmap(void *base, size_t size, int prot, int flags, int fd, long offset)
{
    event('M');
    if (!(!allocation && !base && prot == (PROT_READ | PROT_WRITE) && flags == (MAP_PRIVATE | MAP_ANON) && fd == -1 &&
          !offset && size % (size_t)page == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 1)
    {
        errno = ENOMEM;
        return MAP_FAILED;
    }
    if (failure == 2)
    {
        errno = EACCES;
        return MAP_FAILED;
    }
    length = size;
    allocation = std::calloc(size, 1);
    if (!(allocation))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    executable = false;
    return allocation;
}


static int fixture_mprotect(void *base, size_t size, int prot)
{
    event('P');
    if (!(base == allocation && size == length && prot == (PROT_READ | PROT_EXEC) && !executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (failure == 3)
    {
        return -1;
    }
    executable = true;
    return 0;
}


static void fixture_cache(void *base, size_t size)
{
    event('C');
    if (!(base == allocation && size <= length && executable))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static int fixture_munmap(void *base, size_t size)
{
    event('U');
    if (!(base == allocation && size == length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    std::free(allocation);
    allocation = 0;
    return 0;
}

#define sysconf fixture_sysconf
#define mmap fixture_mmap
#define mprotect fixture_mprotect
#define munmap fixture_munmap
#define sys_icache_invalidate fixture_cache
#define fvm_port_code_supported fixture_code_supported
#define fvm_port_code_create fixture_code_create
#define fvm_port_code_release fixture_code_release
#include "../ports/mac/code.c"

#undef sysconf
#undef mmap
#undef mprotect
#undef munmap
#undef sys_icache_invalidate
#undef fvm_port_code_supported
#undef fvm_port_code_create
#undef fvm_port_code_release


int main()
{
    unsigned char code[4097];
    std::memset(code, 0xa5, sizeof(code));
    if (!(fixture_code_supported()))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (failure = 0; failure < 4; ++failure)
    {
        void *out = (void *)1;
        size_t mapped = 77;
        count = 0;
        events[0] = 0;
        FVM_Result result = fixture_code_create(code, sizeof(code), &out, &mapped);
        if (!failure)
        {
            if (!(result == FVM_OK && out == allocation && mapped == 8192 && !std::memcmp(out, code, sizeof(code)) &&
                  !std::strcmp(events, "SMPC")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fixture_code_release(out, mapped);
            if (!(!allocation && !std::strcmp(events, "SMPCU")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == (failure == 1 ? FVM_OUT_OF_MEMORY : FVM_UNSUPPORTED) && out == (void *)1 && mapped == 77 &&
                  !allocation))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(!std::strcmp(events, failure == 3 ? "SMPU" : "SM")))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    failure = 0;
    void *out = (void *)1;
    size_t mapped = 77;
    for (unsigned bad = 0; bad < 2; ++bad)
    {
        page = bad ? -1 : 0;
        count = 0;
        if (!(fixture_code_create(code, 1, &out, &mapped) == FVM_UNSUPPORTED && out == (void *)1 && mapped == 77 &&
              !std::strcmp(events, "S")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    page = 4096;
    count = 0;
    if (!(fixture_code_create(code, (size_t)-1, &out, &mapped) == FVM_INVALID_ARGUMENT && out == (void *)1 &&
          mapped == 77 && !std::strcmp(events, "S")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(fixture_code_create(0, 1, &out, &mapped) == FVM_INVALID_ARGUMENT && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, 0, &out, &mapped) == FVM_INVALID_ARGUMENT && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, 1, 0, &mapped) == FVM_INVALID_ARGUMENT && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fixture_code_create(code, 1, &out, 0) == FVM_INVALID_ARGUMENT && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fixture_code_release(0, 0);
    if (!(!count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    count = 0;
    if (!(fixture_code_create(code, 4096, &out, &mapped) == FVM_OK && mapped == 4096))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fixture_code_release(out, mapped);
    if (!(!allocation))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    page = 16384;
    count = 0;
    if (!(fixture_code_create(code, 1, &out, &mapped) == FVM_OK && mapped == 16384))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fixture_code_release(out, mapped);
    if (!(!allocation))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
