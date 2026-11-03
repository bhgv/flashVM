/* Reuse checked opcode ownership/marking from native tail entries. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "owner_fast.h"
#include "shared_bytes.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"

extern void self(void); /* Retained current-module ownership opcode. */
extern void headp(void); /* Retained pointer-list head ownership opcode. */
extern void tail(void); /* Retained list-tail ownership opcode. */


/* [GC:] registers borrows candidate native state. Return1 only for current
 * full-size process REG and ACTIVE nonpending role,0 before operand reads
 * otherwise. Metadata preflight acquires no source lifetime or access right. */
int dvm_runtime_owner_entry_ready(REG *registers)
{
    DVM_NativeContext *context = dvm_native_context_current();
    DVM_HeapOwnerStats state;
    DVM_HeapOwner *owner;
    if (!registers || !context ||
        dvm_native_context_register_size(context) != sizeof(REG) ||
        !dvm_native_context_host_process(context) || registers != dvm_native_registers())
    {
        return 0;
    }
    owner = dvm_native_owner_current();
    if (!dvm_native_owner_statistics(owner, &state) ||
        state.phase != DVM_HEAP_ACTIVE || state.pending_progress)
    {
        return 0;
    }
    return 1;
}


/* [GC:] Borrow protected decoded state; execute once after native preflight.
 * operation is process-live C code. Errors propagate, never decline afterward. */
static int invoke(REG *registers, void (*operation)(void))
{
    if (!dvm_runtime_owner_entry_ready(registers))
    {
        return 0;
    }
    operation();
    return 1;
}


/* [GC:] Copy/replace decoded pointer owner through actual checked C movp. */
int dvm_runtime_owner_movp_fast(REG *registers)
{
    return invoke(registers, movp);
}


/* [GC:] Retain current module owner through actual checked C self. */
int dvm_runtime_owner_self_fast(REG *registers)
{
    return invoke(registers, self);
}


/* [GC:] Resolve/retain decoded pointer-list head through actual checked C headp. */
int dvm_runtime_owner_headp_fast(REG *registers)
{
    if (dvm_runtime_guarded_copies && dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    return invoke(registers, headp);
}


/* [GC:] Resolve/retain decoded list tail through actual checked C tail. */
int dvm_runtime_owner_tail_fast(REG *registers)
{
    if (dvm_runtime_guarded_copies && dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    return invoke(registers, tail);
}
