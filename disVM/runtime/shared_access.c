/* Shared preflight for hosted pointer and byte copy primitives. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "shared_access.h"
#include "disvm/native_owner.h"
#include "disvm/native_cleanup.h"


/* [GC:] Preflight exact native process/role BEFORE touching cells. Return one
 * for serial inline, two for serialized worker world, zero for refusal. Cleanup
 * state and held guards cannot be used as payload/cell permission. */
int dvm_runtime_shared_mode(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DVM_NativeCleanupMark mark;
    if (!context || dvm_native_context_register_size(context) != sizeof(REG) ||
        !dvm_native_context_host_process(context) ||
        dvm_native_context_object_count(context) || !dvm_native_cleanup_mark(&mark) ||
        !dvm_native_owner_statistics(owner, &state) || state.pending_progress)
    {
        return 0;
    }
    if (dvm_native_owner_world(owner))
    {
        return state.phase == DVM_HEAP_ACTIVE ? 2 : 0;
    }
    return state.phase == DVM_HEAP_DETACHED && !dvm_native_context_has_poll(context) ? 1 : 0;
}
