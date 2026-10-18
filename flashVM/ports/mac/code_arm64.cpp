/* FlashVM: one macOS ARM64 MAP_JIT region, reusable slots and allowlisted synchronous writes.
 * Metadata stays outside executable memory; the region intentionally survives until process exit.
 */
#include "code_arm64.h"
#include "code_platform.h"
#include "code_arm64_constants.h"
#include <cstdlib>
#include <cstring>
#include <cerrno>
#if FVM_MAC_CODE_ARM64
#include "code_arm64_sdk.hpp"
namespace
{
/* Out-of-band address-ordered slot; all mutation is protected by arena_mutex. */
struct Slot
{
    size_t offset; /* Byte offset from the sole MAP_JIT base. */
    size_t size;   /* Aligned extent, used as the private release cookie. */
    bool used;     /* An owned slot cannot be reused until its caller releases it. */
    Slot *next;   /* Next physically adjacent slot, or NULL at the arena end. */
};
/* Process-owned arena state is independent of VM/context lifetime. */
static pthread_mutex_t arena_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned char *arena_memory = 0; /* Sole mapping; never released/recreated per body. */
static Slot *arena_slots = 0; /* Metadata list spanning the whole mapped region. */
/* One trusted synchronous write request; only the registered callback consumes it. */
struct Write
{
    Slot *slot;        /* Reserved live destination slot in the metadata list. */
    const void *bytes; /* Compiler-owned immutable source, alive through the call. */
    size_t size;       /* Actual instruction byte count, not padded slot capacity. */
};
static Write *active_write = 0; /* Valid only while the arena lock and system callback are active. */

/* Validate the synchronous request before writing; never allocate, throw or call user code here. */
static int write_code(void *opaque)
{
    if (!opaque || opaque != active_write || !arena_memory)
        return FVM_INVALID_ARGUMENT;
    const Write *request = static_cast<const Write *>(opaque);
    const Slot *slot = arena_slots;
    while (slot && slot != request->slot)
        slot = slot->next;
    if (!slot || !slot->used || !request->bytes || !request->size ||
        request->size > slot->size || slot->size > FVM_MAC_JIT_ARENA_BYTES ||
        slot->offset > FVM_MAC_JIT_ARENA_BYTES - slot->size)
        return FVM_INVALID_ARGUMENT;
    std::memcpy(arena_memory + slot->offset, request->bytes, request->size);
    return FVM_OK;
}
/* Register exactly one callback for this static library's executable image. */
FVM_MAC_JIT_ALLOW_CALLBACK(write_code);

/* Merge adjacent free slots without touching any live instruction bytes. Lock required. */
static void coalesce(void)
{
    Slot *slot = arena_slots;
    while (slot && slot->next)
    {
        Slot *next = slot->next;
        if (!slot->used && !next->used)
        {
            slot->size += next->size;
            slot->next = next->next;
            FVM_MAC_JIT_FREE(next);
        }
        else
            slot = next;
    }
}

/* Lazily reserve the sole region; allocation failures leave no half-initialized arena. */
static FVM_Result initialize(void)
{
    if (arena_memory)
        return FVM_OK;
    Slot *first = static_cast<Slot *>(FVM_MAC_JIT_ALLOCATE(sizeof(Slot)));
    if (!first)
        return FVM_OUT_OF_MEMORY;
    void *memory = mmap(0, FVM_MAC_JIT_ARENA_BYTES, PROT_READ | PROT_WRITE | PROT_EXEC,
                        MAP_PRIVATE | MAP_ANON | MAP_JIT, -1, 0);
    if (memory == MAP_FAILED)
    {
        int error = errno;
        FVM_MAC_JIT_FREE(first);
        return error == ENOMEM ? FVM_OUT_OF_MEMORY : FVM_UNSUPPORTED;
    }
    first->offset = 0;
    first->size = FVM_MAC_JIT_ARENA_BYTES;
    first->used = false;
    first->next = 0;
    arena_slots = first;
    arena_memory = static_cast<unsigned char *>(memory);
    return FVM_OK;
}

/* Reserve, split and publish one body while holding the shared arena lock. */
static FVM_Result create_locked(const void *bytes, size_t size, void **out, size_t *cookie)
{
    size_t padding = (FVM_MAC_JIT_SLOT_ALIGNMENT - size % FVM_MAC_JIT_SLOT_ALIGNMENT) %
        FVM_MAC_JIT_SLOT_ALIGNMENT;
    if (size > FVM_MAC_JIT_ARENA_BYTES || padding > FVM_MAC_JIT_ARENA_BYTES - size)
        return FVM_UNSUPPORTED;
    size_t length = size + padding;
    FVM_Result status = initialize();
    if (status != FVM_OK)
        return status;
    Slot *slot = arena_slots;
    while (slot && (slot->used || slot->size < length))
        slot = slot->next;
    if (!slot)
        return FVM_OUT_OF_MEMORY;
    if (slot->size != length)
    {
        Slot *tail = static_cast<Slot *>(FVM_MAC_JIT_ALLOCATE(sizeof(Slot)));
        if (!tail)
            return FVM_OUT_OF_MEMORY;
        tail->offset = slot->offset + length;
        tail->size = slot->size - length;
        tail->used = false;
        tail->next = slot->next;
        slot->next = tail;
        slot->size = length;
    }
    slot->used = true;
    Write request;
    request.slot = slot;
    request.bytes = bytes;
    request.size = size;
    active_write = &request;
    int result = pthread_jit_write_with_callback_np(write_code, &request);
    active_write = 0;
    if (result != FVM_OK)
    {
        slot->used = false;
        coalesce();
        return FVM_UNSUPPORTED;
    }
    /* The system restored this thread's executable protection before cache publication. */
    sys_icache_invalidate(arena_memory + slot->offset, size);
    *out = arena_memory + slot->offset;
    *cookie = slot->size;
    return FVM_OK;
}
} // namespace
#endif

/* Deployment/SDK gating precedes this facility query; entitlement denial is reported at mapping. */
extern "C" int fvm_mac_arm64_code_supported(void)
{
#if FVM_MAC_CODE_ARM64
    return pthread_jit_write_protect_supported_np() != 0;
#else
    return 0;
#endif
}

/* Keep caller outputs unchanged on every failure, and never fall back to unprotected RWX memory. */
extern "C" FVM_Result fvm_mac_arm64_code_create(const void *bytes, size_t size,
                                               void **out, size_t *cookie)
{
#if FVM_MAC_CODE_ARM64
    if (!bytes || !size || !out || !cookie)
        return FVM_INVALID_ARGUMENT;
    if (!fvm_mac_arm64_code_supported())
        return FVM_UNSUPPORTED;
    if (pthread_mutex_lock(&arena_mutex))
        return FVM_ERROR;
    FVM_Result status = create_locked(bytes, size, out, cookie);
    pthread_mutex_unlock(&arena_mutex);
    return status;
#else
    (void)bytes;
    (void)size;
    (void)out;
    (void)cookie;
    return FVM_UNSUPPORTED;
#endif
}

/* Caller guarantees the slot is inactive; a wrong pointer/cookie pair does not release live code. */
extern "C" void fvm_mac_arm64_code_release(void *memory, size_t cookie)
{
#if FVM_MAC_CODE_ARM64
    if (!memory || !cookie || pthread_mutex_lock(&arena_mutex))
        return;
    Slot *slot = arena_slots;
    while (slot && (!slot->used || arena_memory + slot->offset != memory || slot->size != cookie))
        slot = slot->next;
    if (slot)
    {
        slot->used = false;
        coalesce();
    }
    pthread_mutex_unlock(&arena_mutex);
#else
    (void)memory;
    (void)cookie;
#endif
}
