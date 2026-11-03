/* Extended up owns active REG, alongside the retained Proc error/FPU environment. */
#include "lib9.h"
#include "dat.h"
#include "isa.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "disvm/native_process.h"
#include "disvm/native_cleanup.h"


/* Proc is the prefix consumed by retained host/kernel routines. Active registers
 * and native coordination are extensions, not incompatible edits to that prefix.
 * [GC:] Proc.iprog/prog/env and active registers belong to this native owner. */
typedef struct DVM_ExtendedUp
{
    Proc Proc; /* Error frames, current Prog links, OS/FPU environment. */
    REG R;     /* The currently executing Dis register file. */
    DVM_NativeCleanupMark error_mark[NERR]; /* Native checkpoints, not REG/process roots or retained Proc fields. */
    DVM_NativeExecution execution; /* Native owner guards, excluded from whole REG copies. */
} DVM_ExtendedUp;


/* Create up and REG in one native allocation, served through one TLS owner. */
DVM_NativeContext *dvm_native_process_create(void *runtime)
{
    return dvm_native_context_create_embedded(sizeof(DVM_ExtendedUp),
                                              offsetof(DVM_ExtendedUp, R),
                                              sizeof(REG),
                                              offsetof(DVM_ExtendedUp, Proc.iprog),
                                              runtime);
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
    {
        abort();
    }
    return &process->R;
}


/* Borrow execution guards from exactly the same TLS allocation as up and R.
 * No caller-owned pointer is accepted. The REG adapter checks bound/typed
 * storage first; a saved/restored Prog.R cannot overwrite these native fields. */
DVM_NativeExecution *dvm_native_execution(void)
{
    DVM_ExtendedUp *process;
    (void)dvm_native_registers();
    process = (DVM_ExtendedUp *)getup();
    return &process->execution;
}


/* Capture cleanup generation BEFORE the caller executes setjmp. Existing Proc
 * nerr/estack remain unchanged in layout. Refuse overflow/guarded/unwinding roles
 * without pushing a partially initialized label; caller macro treats refusal
 * as a fatal host protocol error. No allocation or managed heap access. */
int dvm_native_error_push(void)
{
    DVM_ExtendedUp *process = (DVM_ExtendedUp *)getup();
    if (!process || process->Proc.nerr < 0 || process->Proc.nerr >= NERR ||
        !dvm_native_cleanup_mark(&process->error_mark[process->Proc.nerr]))
    {
        return 0;
    }
    process->Proc.nerr++;
    return 1;
}


/* Normal label retirement requires explicit close of scopes created inside it.
 * Check scalar generations without traversing possibly returned C frames; do not
 * attempt late cleanup after a callback returned. Refuse without popping on
 * pending scopes/invalid stack. Error marks are owner-local native metadata. */
int dvm_native_error_pop(void)
{
    DVM_ExtendedUp *process = (DVM_ExtendedUp *)getup();
    if (!process || process->Proc.nerr <= 0 || process->Proc.nerr > NERR ||
        !dvm_native_cleanup_clean(&process->error_mark[process->Proc.nerr - 1]))
    {
        return 0;
    }
    process->Proc.nerr--;
    process->error_mark[process->Proc.nerr].context = NULL;
    process->error_mark[process->Proc.nerr].serial = 0;
    return 1;
}


/* Close newer native scopes BEFORE the host performs longjmp, while callback
 * frames and their caller-owned entries are live. On protocol failure caller
 * must terminate, not jump through a partially cleaned native continuation.
 * Successful return consumes exactly one Proc error label. No managed cleanup. */
int dvm_native_error_unwind(void)
{
    DVM_ExtendedUp *process = (DVM_ExtendedUp *)getup();
    if (!process || process->Proc.nerr <= 0 || process->Proc.nerr > NERR ||
        !dvm_native_cleanup_unwind(&process->error_mark[process->Proc.nerr - 1]))
    {
        return 0;
    }
    process->Proc.nerr--;
    process->error_mark[process->Proc.nerr].context = NULL;
    process->error_mark[process->Proc.nerr].serial = 0;
    return 1;
}
