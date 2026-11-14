/* Copyright (c) 2026 bhgv
 * EN: Minimal async-safe RX fault capture and same-thread scalar-leaf landing.
 * RU: Минимальный async-safe RX fault capture и same-thread scalar-leaf landing.
 */
#define _GNU_SOURCE
#include "internal.h"
#include "native_jump.h"
#include <errno.h>
#include <string.h>

static const int numbers[] = {SIGILL, SIGSEGV, SIGBUS, SIGFPE}; /* Synchronous CPU faults only. */
static struct sigaction previous[4];          /* Saved launch-time dispositions, restored after owners join. */
static unsigned int installed;                /* Launch/shutdown thread only. */
int dvm_jit_signals_enabled;                  /* Immutable while VM execution threads exist. */
__thread JitSignalState dvm_jit_signal_state; /* Static executable TLS, allocated with owner thread. */


/* EN: Capture only synchronous faults whose actual PC lies in the admitted RX
 * mapping. Handler uses lock-free scalar stores and POSIX siglongjmp only; no
 * managed inspection, cleanup, logging, allocation, mutex or quarantine update.
 * Other faults restore their original disposition and re-deliver the signal.
 * RU: Снимать только synchronous faults с actual PC внутри admitted RX mapping.
 * Только lock-free scalar stores и POSIX siglongjmp; без managed inspection,
 * cleanup, logging, allocation, mutex либо quarantine update. Остальные faults
 * возвращают original disposition и повторно доставляют сигнал. */
static void capture(int number, siginfo_t *information, void *context)
{
    JitSignalState *state = &dvm_jit_signal_state;
    uintptr_t pc = dvm_jit_signal_pc(context);
    uintptr_t base = __atomic_load_n(&state->base, __ATOMIC_RELAXED);
    size_t size = __atomic_load_n(&state->size, __ATOMIC_RELAXED);
    unsigned int index;
    int saved_errno = errno;
    if (__atomic_load_n(&state->armed, __ATOMIC_ACQUIRE) && information && information->si_code > 0 && pc >= base &&
        pc - base < size)
    {
        __atomic_store_n(&state->armed, 0, __ATOMIC_RELEASE);
        __atomic_store_n(&state->fault.signal_number, number, __ATOMIC_RELAXED);
        __atomic_store_n(&state->fault.signal_code, information->si_code, __ATOMIC_RELAXED);
        __atomic_store_n(&state->fault.address, (uintptr_t)information->si_addr, __ATOMIC_RELAXED);
        __atomic_store_n(&state->fault.rx_base, base, __ATOMIC_RELAXED);
        __atomic_store_n(&state->fault.rx_size, size, __ATOMIC_RELAXED);
        dvm_jit_signal_context(context, &state->fault);
        __atomic_store_n(&state->ready, 1, __ATOMIC_RELEASE);
        errno = saved_errno;
        dvm_native_error_jump(state->landing, 1);
    }
    for (index = 0; index < sizeof(numbers) / sizeof(numbers[0]); index++)
    {
        if (number == numbers[index])
        {
            struct sigaction action = previous[index];
            if (information && information->si_code > 0 && action.sa_handler == SIG_IGN)
            {
                action.sa_handler = SIG_DFL;
            }
            sigaction(number, &action, NULL);
            raise(number);
            errno = saved_errno;
            return;
        }
    }
}


/* EN/RU: Install process-wide handlers before workers; roll back partial failures.
 * Установить process-wide handlers до workers; откатить partial failure. */
int dvm_jit_signals_start(void)
{
    struct sigaction action;
    unsigned int index;
    if (dvm_jit_signals_enabled)
    {
        return 1;
    }
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = capture;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    for (index = 0; index < sizeof(numbers) / sizeof(numbers[0]); index++)
    {
        sigaddset(&action.sa_mask, numbers[index]);
    }
    for (installed = 0; installed < sizeof(numbers) / sizeof(numbers[0]); installed++)
    {
        if (sigaction(numbers[installed], &action, &previous[installed]))
        {
            dvm_jit_signals_stop();
            return 0;
        }
    }
    dvm_jit_signals_enabled = 1;
    return 1;
}


/* EN/RU: Restore original dispositions only after all source owners have stopped.
 * Вернуть original dispositions только после остановки всех source owners. */
void dvm_jit_signals_stop(void)
{
    while (installed)
    {
        installed--;
        sigaction(numbers[installed], &previous[installed], NULL);
    }
    dvm_jit_signals_enabled = 0;
}
