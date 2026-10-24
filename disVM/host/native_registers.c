/* Extended up owns active REG, alongside the retained Proc error/FPU environment. */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "disvm/native_process.h"

/* Proc is the prefix consumed by retained host/kernel routines. Active registers
 * and native coordination are extensions, not incompatible edits to that prefix.
 * [GC:] Proc.iprog/prog/env and active registers belong to this native owner. */
typedef struct DVM_ExtendedUp {
    Proc Proc; /* Error frames, current Prog links, OS/FPU environment. */
    REG R;     /* The currently executing Dis register file. */
} DVM_ExtendedUp;

/* Create up and REG in one native allocation, served through one TLS owner. */
DVM_NativeContext *dvm_native_process_create(void *runtime)
{
    return dvm_native_context_create_embedded(sizeof(DVM_ExtendedUp),
        offsetof(DVM_ExtendedUp, R), sizeof(REG), offsetof(DVM_ExtendedUp, Proc.iprog), runtime);
}

/* The normal Inferno up macro calls this same native TLS owner, not another key. */
Proc *getup(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    return (Proc *)dvm_native_context_host_process(context);
}

/* Active R is a subobject of current up; saved Prog.R remains an independent snapshot. */
REG *dvm_native_registers(void)
{
    DVM_ExtendedUp *process = (DVM_ExtendedUp *)getup();
    DVM_NativeContext *context = dvm_native_context_current();
    if (!process || dvm_native_context_register_size(context) != sizeof(REG) ||
        dvm_native_context_registers(context) != &process->R)
        abort();
    return &process->R;
}
