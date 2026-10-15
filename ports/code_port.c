/* FlashVM: Linux W^X executable mappings with instruction-cache synchronization. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* Request MAP_ANONYMOUS from the platform headers. */
#endif
#include "code_port.h"
#if defined(__linux__)
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#endif
/* No backend is inferred from pointer width alone. */
int fvm_port_code_supported(void)
{
#if defined(__linux__)
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__) || defined(__arm__) ||          \
                          defined(__aarch64__) || defined(__riscv))
    return 1;
#else
    return 0;
#endif
#else
    return 0;
#endif
}
/* Never map RWX or retain writable aliases after sealing the instruction stream. */
FVM_Result fvm_port_code_create(const void *bytes, size_t size, void **out, size_t *mapped)
{
#if defined(__linux__)
    long page;
    size_t length, padding;
    void *memory;
    if (!bytes || !size || !out || !mapped)
        return FVM_INVALID_ARGUMENT;
    page = sysconf(_SC_PAGESIZE);
    if (page <= 0)
        return FVM_UNSUPPORTED;
    padding = ((size_t)page - size % (size_t)page) % (size_t)page;
    if (size > (size_t)-1 - padding)
        return FVM_INVALID_ARGUMENT;
    length = size + padding;
    memory = mmap(0, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED)
        return errno == ENOMEM ? FVM_OUT_OF_MEMORY : FVM_UNSUPPORTED;
    memcpy(memory, bytes, size);
    if (mprotect(memory, length, PROT_READ | PROT_EXEC))
    {
        munmap(memory, length);
        return FVM_UNSUPPORTED;
    }
#if defined(__GNUC__)
    /* Publish only after the architecture/compiler cache synchronization has completed. */
    __builtin___clear_cache((char *)memory, (char *)memory + size);
#else
    munmap(memory, length);
    return FVM_UNSUPPORTED;
#endif
    *out = memory;
    *mapped = length;
    return FVM_OK;
#else
    (void)bytes;
    (void)size;
    (void)out;
    (void)mapped;
    return FVM_UNSUPPORTED;
#endif
}
/* The caller guarantees no active native frame can return into this mapping. */
void fvm_port_code_release(void *memory, size_t mapped)
{
#if defined(__linux__)
    if (memory && mapped)
        munmap(memory, mapped);
#else
    (void)memory;
    (void)mapped;
#endif
}
