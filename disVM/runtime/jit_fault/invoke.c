/* Copyright (c) 2026 bhgv
 * EN: Audited RX scalar invocation and non-replaying post-signal error delivery.
 * RU: Проверенный RX scalar invocation и доставка post-signal error без replay.
 */
#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "isa.h"
#include "internal.h"
#include "fp_view.h"
#include "jit_quarantine.h"
#include "disvm/native_context.h"
#include "disvm/native_cleanup.h"
#include "shared_access.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <fenv.h>


/* EN/RU: Publish immutable RX extent/opcode while the rooted JIT is borrowed.
 * Публиковать immutable RX extent/opcode при заимствованном rooted JIT. */
void dvm_jit_signal_select(const void *base, size_t size, int opcode)
{
    __atomic_store_n(&dvm_jit_signal_state.base, (uintptr_t)base, __ATOMIC_RELAXED);
    __atomic_store_n(&dvm_jit_signal_state.size, size, __ATOMIC_RELAXED);
    dvm_jit_signal_state.opcode = opcode;
}


/* [GC:] EN: Admit integer and basic REAL scalar leaves only. No allocation, managed pointer/PC
 * publication, channel/module/stack transition or refcount operation is allowed.
 * C helpers outside RX remain outside this synchronous-signal landing contract.
 * RU: Допустить integer и basic REAL scalar leaves. Нет allocation, публикации managed
 * pointer/PC, channel/module/stack переходов либо refcount operations.
 * C helpers вне RX не входят в контракт synchronous-signal landing. */
static int scalar(int opcode)
{
    switch (opcode)
    {
    case IMOVF:
    case IADDF:
    case ISUBF:
    case IMULF:
    case IDIVF:
    case INEGF:
    case IMOVW:
    case IMOVB:
    case IMOVL:
    case IADDW:
    case ISUBW:
    case IMULW:
    case IDIVW:
    case IMODW:
    case IANDW:
    case IORW:
    case IXORW:
    case ISHLW:
    case ISHRW:
    case IADDL:
    case ISUBL:
    case IMULL:
    case IDIVL:
    case IMODL:
    case IANDL:
    case IORL:
    case IXORL:
    case ISHLL:
    case ISHRL:
        return 1;
    default:
        return 0;
    }
}


/* EN/RU: Print bounded integer CPU values outside handler, correlated by source PID.
 * Вывести bounded integer CPU values вне handler с PID исходного owner. */
static void report_registers(const DVM_JitFault *fault, int pid)
{
    char output[1280];
    size_t used =
        (size_t)snprintf(output, sizeof(output), "JITCPU pid=%d native_pc=%llx", pid, (unsigned long long)fault->pc);
    unsigned int index;
    for (index = 0; index < fault->count && index < 32 && used + 40 < sizeof(output); index++)
    {
        used += (size_t)snprintf(output + used,
                                 sizeof(output) - used,
                                 " %s=%llx",
                                 dvm_jit_fault_register_name(index),
                                 (unsigned long long)fault->registers[index]);
    }
    snprintf(output + used, sizeof(output) - used, "\n");
    fputs(output, stderr);
}


/* [GC:] EN: Guard a selected RX scalar callback only with no held object guards.
 * Synchronous signal PC must remain inside the checked mapping. Landing discards
 * only native leaf frames; it never restores VM REG or replays writes. Log and
 * quarantine occur outside the handler, then ordinary error/retained-debug flow.
 * RU: Защищать selected RX scalar callback только без object guards. Actual
 * signal PC должен остаться в mapping. Landing удаляет только native leaf frames;
 * не восстанавливает VM REG и не повторяет writes. Log/quarantine вне handler,
 * затем обычный error/retained-debug flow. */
int dvm_jit_signal_invoke(int (*fast)(REG *), REG *registers, int opcode)
{
    JitSignalState *state = &dvm_jit_signal_state;
    uintptr_t address = (uintptr_t)fast;
    uintptr_t base = __atomic_load_n(&state->base, __ATOMIC_RELAXED);
    size_t size = __atomic_load_n(&state->size, __ATOMIC_RELAXED);
    int result;
    DVM_NativeCleanupMark mark;
    fenv_t environment; /* Pre-invocation FP policy; restored only after admitted fault. */
    REG before;         /* Comparison only; never a register-restore or resume source. */
    if (!dvm_jit_signals_enabled || !scalar(opcode) || opcode != state->opcode || !base || size > UINTPTR_MAX - base ||
        address < base || address - base >= size || dvm_runtime_shared_mode() != 2 ||
        dvm_native_context_object_count(dvm_native_context_current()) || !dvm_native_cleanup_mark(&mark))
    {
        return fast(registers);
    }
    if (fegetenv(&environment))
    {
        return 0; /* No effects: preserve interpreter policy if FP state is unavailable. */
    }
    before = *registers;
    if (!dvm_jit_signal_thread_start())
    {
        return 0; /* No partial invocation; ordinary interpreter handles the instruction. */
    }
    if (sigsetjmp(state->landing, 1))
    {
        DVM_JitFault *fault = &state->fault;
        __atomic_store_n(&state->armed, 0, __ATOMIC_RELEASE);
        if (dvm_runtime_shared_mode() != 2 || dvm_native_context_object_count(dvm_native_context_current()) ||
            !dvm_native_cleanup_clean(&mark) || registers->M != before.M || registers->MP != before.MP ||
            registers->FP != before.FP || registers->SP != before.SP || registers->TS != before.TS ||
            registers->EX != before.EX || registers->PC != before.PC || registers->s != before.s ||
            registers->d != before.d || registers->m != before.m)
        {
            fputs("disvm: native RX fault changed admission/cleanup/control; unsafe recovery refused\n", stderr);
            abort();
        }
        if (fesetenv(&environment))
        {
            fputs("disvm: cannot restore pre-fault FP environment; unsafe recovery refused\n", stderr);
            abort();
        }
        fprintf(
            stderr,
            "JITSIGNAL signal=%d code=%d address=%llx native_pc=%llx native_sp=%llx rx_offset=%llu registers=%u replay=0\n",
            fault->signal_number,
            fault->signal_code,
            (unsigned long long)fault->address,
            (unsigned long long)fault->pc,
            (unsigned long long)fault->sp,
            (unsigned long long)(fault->pc - fault->rx_base),
            fault->count);
        report_registers(fault, currun()->pid);
        dvm_jit_fault_fp_report(fault, currun()->pid);
        dvm_jit_report_failure(opcode, "synchronous RX scalar leaf signal");
        error("sys: JIT native signal");
        return 0;
    }
    __atomic_store_n(&state->armed, 1, __ATOMIC_RELEASE);
    result = fast(registers);
    __atomic_store_n(&state->armed, 0, __ATOMIC_RELEASE);
    return result;
}
