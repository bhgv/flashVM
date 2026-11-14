/* Copyright (c) 2026 bhgv
 * EN: Alternate signal stack lifetime and copied native fault publication per owner.
 * RU: Lifetime alternate signal stack и copied native fault publication своего owner.
 */
#define _GNU_SOURCE
#include "internal.h"
#include <stdlib.h>
#include <string.h>


/* EN: Prepare this native owner's stack outside handlers; reuse an existing
 * enabled stack without ownership. Return0 on failure, declining RX admission.
 * RU: Подготовить stack своего native owner вне handlers; enabled existing stack
 * заимствовать без владения. Вернуть0 при отказе, запрещая RX admission. */
int dvm_jit_signal_thread_start(void)
{
    JitSignalState *state = &dvm_jit_signal_state;
    stack_t next;
    size_t size = 65536;
    if (state->stack)
    {
        return 1;
    }
    if (sigaltstack(NULL, &state->previous))
    {
        return 0;
    }
    if (!(state->previous.ss_flags & SS_DISABLE))
    {
        return state->previous.ss_size >= (size_t)MINSIGSTKSZ;
    }
    if (size < (size_t)SIGSTKSZ)
    {
        size = (size_t)SIGSTKSZ;
    }
    state->stack = malloc(size);
    if (!state->stack)
    {
        return 0;
    }
    memset(&next, 0, sizeof(next));
    next.ss_sp = state->stack;
    next.ss_size = size;
    if (sigaltstack(&next, NULL))
    {
        free(state->stack);
        state->stack = NULL;
        return 0;
    }
    return 1;
}


/* EN: Release only this owner's own alternate stack outside invocation. If another
 * party changed its configuration, do not free potentially active signal storage.
 * RU: Освободить только свой alternate stack вне invocation. При изменении
 * configuration другой стороной не освобождать потенциально active signal storage. */
void dvm_jit_signal_thread_stop(void)
{
    JitSignalState *state = &dvm_jit_signal_state;
    stack_t current;
    if (!state->stack || __atomic_load_n(&state->armed, __ATOMIC_ACQUIRE))
    {
        return;
    }
    if (!sigaltstack(NULL, &current) && current.ss_sp == state->stack && !sigaltstack(&state->previous, NULL))
    {
        free(state->stack);
        state->stack = NULL;
    }
}


/* EN/RU: Copy this owner's completed CPU snapshot after signal landing; no managed reads.
 * Копировать completed CPU snapshot своего owner после landing; без managed reads. */
int dvm_jit_fault_copy(DVM_JitFault *output)
{
    if (!output || !__atomic_load_n(&dvm_jit_signal_state.ready, __ATOMIC_ACQUIRE))
    {
        return 0;
    }
    *output = dvm_jit_signal_state.fault;
    return 1;
}


/* EN/RU: Next admitted instruction invalidates previous fault diagnostics, not VM state.
 * Следующая admitted instruction снимает прежнюю диагностику, не VM state. */
void dvm_jit_fault_clear(void)
{
    __atomic_store_n(&dvm_jit_signal_state.ready, 0, __ATOMIC_RELEASE);
}


/* EN/RU: VM error transfer invalidates the invocation's C landing frame.
 * VM error transfer делает C landing frame invocation недействительным. */
void dvm_jit_signal_disarm(void)
{
    __atomic_store_n(&dvm_jit_signal_state.armed, 0, __ATOMIC_RELEASE);
}
