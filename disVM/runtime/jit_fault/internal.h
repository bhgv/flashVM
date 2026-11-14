/* Copyright (c) 2026 bhgv
 * EN: Per-owner preallocated signal landing storage; no foreign managed ownership.
 * RU: Заранее выделенное signal landing storage своего owner; без чужого managed ownership.
 */
#ifndef DVM_JIT_FAULT_INTERNAL_H
#define DVM_JIT_FAULT_INTERNAL_H
#include "signals.h"
#include <signal.h>
#include <setjmp.h>

#if __GCC_ATOMIC_POINTER_LOCK_FREE != 2 || __GCC_ATOMIC_LLONG_LOCK_FREE != 2 || __GCC_ATOMIC_INT_LOCK_FREE != 2
#error "JIT signal capture requires lock-free pointer and 64-bit atomics"
#endif


/* EN/RU: State belongs permanently to its native OS thread; handler never allocates.
 * State навсегда принадлежит своему OS thread; handler не выделяет память. */
typedef struct JitSignalState
{
    sigjmp_buf landing; /* Same-thread live invocation continuation only. */
    int armed;          /* Lock-free atomic predicate, set only around audited RX invocation. */
    int ready;          /* Lock-free atomic publication after complete CPU snapshot. */
    uintptr_t base;     /* Immutable admitted mapping lower bound. */
    size_t size;        /* Mapping extent, checked before admission. */
    int opcode;         /* Original selected scalar opcode. */
    void *stack;        /* Owned alternate stack, NULL when caller supplies one. */
    stack_t previous;   /* Previous disabled/foreign stack configuration. */
    DVM_JitFault fault; /* Handler writes lock-free scalars; owner reads after landing. */
} JitSignalState;

extern __thread JitSignalState dvm_jit_signal_state; /* Static executable TLS, no dynamic lookup. */
extern int dvm_jit_signals_enabled;                  /* Immutable across VM owner lifetimes. */


/* EN/RU: Decode only kernel-supplied context into lock-free scalar snapshot fields.
 * Декодировать только kernel context в lock-free scalar поля snapshot. */
void dvm_jit_signal_context(void *context, DVM_JitFault *output);


/* EN/RU: Read actual kernel PC without allocation, locks or managed access.
 * Читать actual kernel PC без allocation, locks либо managed access. */
uintptr_t dvm_jit_signal_pc(void *context);


/* EN/RU: Prepare alternate stack outside signal handler; return0 on native failure.
 * Подготовить alternate stack вне signal handler; вернуть0 при native failure. */
int dvm_jit_signal_thread_start(void);
#endif
