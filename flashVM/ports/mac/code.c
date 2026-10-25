/* FlashVM: classic macOS Intel RW-to-RX mappings, with explicit denial/fallback under OS policy. */
#include "../code_port.h"
#include "code_platform.h"
#include "code_sdk.h"
#include "code_arm64.h"
#include <errno.h>
#include <string.h>


/* Report the implemented ABI, not an entitlement or a guarantee of mapping permission. */
int fvm_port_code_supported(void)
{
#if FVM_MAC_CODE_ARM64
    return fvm_mac_arm64_code_supported();
#else
    return FVM_MAC_CODE_SUPPORTED;
#endif
}


/* Publish independent immutable code pages only after protection and instruction-cache synchronization. */
FVM_Result fvm_port_code_create(const void *bytes, size_t size, void **out, size_t *mapped)
{
#if FVM_MAC_CODE_ARM64
    return fvm_mac_arm64_code_create(bytes, size, out, mapped);
#elif FVM_MAC_CODE_SUPPORTED
    long page;
    size_t padding, length;
    void *memory;
    if (!bytes || !size || !out || !mapped)
    {
        return FVM_INVALID_ARGUMENT;
    }
    page = sysconf(_SC_PAGESIZE);
    if (page <= 0)
    {
        return FVM_UNSUPPORTED;
    }
    padding = ((size_t)page - size % (size_t)page) % (size_t)page;
    if (size > (size_t)-1 - padding)
    {
        return FVM_INVALID_ARGUMENT;
    }
    length = size + padding;
    memory = mmap(0, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (memory == MAP_FAILED)
    {
        return errno == ENOMEM ? FVM_OUT_OF_MEMORY : FVM_UNSUPPORTED;
    }
    memcpy(memory, bytes, size);
    if (mprotect(memory, length, PROT_READ | PROT_EXEC))
    {
        munmap(memory, length);
        return FVM_UNSUPPORTED;
    }
    sys_icache_invalidate(memory, size);
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


/* Caller ownership excludes active native frames or concurrent use during this unmapping. */
void fvm_port_code_release(void *memory, size_t mapped)
{
#if FVM_MAC_CODE_ARM64
    fvm_mac_arm64_code_release(memory, mapped);
#elif FVM_MAC_CODE_SUPPORTED
    if (memory && mapped)
    {
        munmap(memory, mapped);
    }
#else
    (void)memory;
    (void)mapped;
#endif
}
