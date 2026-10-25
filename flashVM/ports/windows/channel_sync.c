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
