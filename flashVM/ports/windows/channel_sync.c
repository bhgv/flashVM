/* Copyright (c) 2026 bhgv
 * EN: Platform mutex/condition ownership and bounded acquisition.
 * RU: Владение платформенными mutex/condition и ограниченный захват.
 */


/* FlashVM: Portable platform adapter interface or implementation: channel sync.
 * File: ports/windows/channel_sync.c.
 */

#include "../channel_sync.h"
#include <windows.h>
#include <stdlib.h>


/* Define FVM_PortSync struct and its representation. */
struct FVM_PortSync
{
    CRITICAL_SECTION mutex;
    CONDITION_VARIABLE condition;
};


/* Create port sync. */
FVM_PortSync *fvm_port_sync_create(void)
{
    FVM_PortSync *s = (FVM_PortSync *)malloc(sizeof(*s));
    if (!s)
    {
        return 0;
    }

    /* Computation / Вычисление и обновление состояния. */
    InitializeCriticalSection(&s->mutex);
    InitializeConditionVariable(&s->condition);
    return s;
}


/* Destroy port sync. */
void fvm_port_sync_destroy(FVM_PortSync *s)
{
    DeleteCriticalSection(&s->mutex);
    free(s);
}


/* Acquire port sync. */
void fvm_port_sync_lock(FVM_PortSync *s)
{
    EnterCriticalSection(&s->mutex);
}


/* EN: Try borrowed sync once. Return one owning its mutex, zero without
 * ownership on contention or port error; never wait or touch the condition.
 * RU: Однократно захватить sync. Один означает владение мьютексом, ноль —
 * конфликт либо ошибку порта; ожидания и обращения к condition нет. */
int fvm_port_sync_try_lock(FVM_PortSync *s)
{
    return TryEnterCriticalSection(&s->mutex) != 0;
}


/* Release port sync. */
void fvm_port_sync_unlock(FVM_PortSync *s)
{
    LeaveCriticalSection(&s->mutex);
}


/* Wait for port sync. */
void fvm_port_sync_wait(FVM_PortSync *s)
{
    SleepConditionVariableCS(&s->condition, &s->mutex, INFINITE);
}


/* Wait with the mutex held; the platform returns it on wake or timeout. */
int fvm_port_sync_wait_ms(FVM_PortSync *s, unsigned long milliseconds)
{
    DWORD timeout = milliseconds < INFINITE ? (DWORD)milliseconds : INFINITE - 1;

    return SleepConditionVariableCS(&s->condition, &s->mutex, timeout) ||
        GetLastError() == ERROR_TIMEOUT;
}


/* Notify port sync all. */
void fvm_port_sync_signal_all(FVM_PortSync *s)
{
    WakeAllConditionVariable(&s->condition);
}


/* SRW locks, like the existing condition variables, require Windows Vista+. */
static SRWLOCK registry_mutex = SRWLOCK_INIT;


/* Acquire port registry. */
void fvm_port_registry_lock(void)
{
    AcquireSRWLockExclusive(&registry_mutex);
}


/* Release port registry. */
void fvm_port_registry_unlock(void)
{
    ReleaseSRWLockExclusive(&registry_mutex);
}
