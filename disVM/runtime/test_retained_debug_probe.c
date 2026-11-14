/* Copyright (c) 2026 bhgv
 * EN: Safe semantic omissions affect only fault-stop request or copied opcode.
 * RU: Safe semantic omissions меняют только fault-stop request либо copied opcode.
 */
#include "lib9.h"
#include "isa.h"
#include "dat.h"
#include "fns.h"
#include "gc/collector_passes.h"
#include <string.h>
#include "interp.h"
#include "debugger/debugger.h"
#include "diagnostics/location.h"
#include <stdio.h>

extern void __real_dvm_debug_fault(const char *);
extern void __real_dvm_debug_instruction(const REG *, const Inst *, const DVM_DiagnosticLocation *);
extern unsigned int __real_dvm_gc_fast_shared(DVM_NativeContext *, size_t, void *);
extern void __real_dvm_debug_native_gate(Prog *);
static int fault_pid;   /* Exact source PID under test, atomic. */
static int fault_held;  /* Actual source fault window, atomic. */
static int gc_observed; /* Atomic once-only successful Fast work evidence. */
static int observed;    /* Atomic once-only independent ISA evidence. */


/* EN/RU: CONTROL=1 omits the stop before handler; guest exception semantics remain normal.
 * CONTROL=1 убирает stop до handler; guest exception semantics остаются обычными. */
void __wrap_dvm_debug_fault(const char *message)
{
    if (!__atomic_exchange_n(&observed, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVEDEBUGSOURCE divide_opcode=%u omitted=%d\n", IDIVW, CONTROL == 1);
    }
    if (CONTROL != 1)
    {
        if (!strcmp(message, "zero divide"))
        {
            __atomic_store_n(&fault_pid, currun()->pid, __ATOMIC_RELAXED);
            __atomic_store_n(&fault_held, 1, __ATOMIC_RELEASE);
        }
        __real_dvm_debug_fault(message);
    }
}


/* EN/RU: CONTROL=2 corrupts copied divide opcode only; never source bytecode/REG.
 * CONTROL=2 искажает только copied divide opcode; не source bytecode/REG. */
void __wrap_dvm_debug_instruction(const REG *registers, const Inst *instruction, const DVM_DiagnosticLocation *location)
{
    Inst copy = *instruction;
    if (CONTROL == 2 && copy.op == IDIVW)
    {
        copy.op = INOP;
    }
    __real_dvm_debug_instruction(registers, &copy, location);
}


/* [GC:] EN/RU: Observe actual completed Fast work while original fault roots survive.
 * Наблюдать actual completed Fast work при сохранённых original fault roots. */
unsigned int __wrap_dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user)
{
    unsigned int result = __real_dvm_gc_fast_shared(context, budget, user);
    if (result && __atomic_load_n(&fault_held, __ATOMIC_ACQUIRE) &&
        !__atomic_exchange_n(&gc_observed, 1, __ATOMIC_RELAXED))
    {
        fprintf(stderr, "NATIVEDEBUGGC completed_fast_work_while_fault_held=1\n");
    }
    return result;
}


/* EN/RU: End probe fault window only when its actual source crosses the reentry gate.
 * Завершить probe fault window только при actual source reentry через gate. */
void __wrap_dvm_debug_native_gate(Prog *process)
{
    __real_dvm_debug_native_gate(process);
    if (process && process->pid == __atomic_load_n(&fault_pid, __ATOMIC_RELAXED))
    {
        __atomic_store_n(&fault_held, 0, __ATOMIC_RELEASE);
    }
}
