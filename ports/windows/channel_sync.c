#include "../channel_sync.h"
#include <windows.h>
#include <stdlib.h>
struct FVM_PortSync { CRITICAL_SECTION mutex; CONDITION_VARIABLE condition; };
FVM_PortSync *fvm_port_sync_create(void) {
    FVM_PortSync *s=(FVM_PortSync *)malloc(sizeof(*s));
    if (!s) return 0;
    InitializeCriticalSection(&s->mutex);
    InitializeConditionVariable(&s->condition);
    return s;
}
void fvm_port_sync_destroy(FVM_PortSync *s) { DeleteCriticalSection(&s->mutex); free(s); }
void fvm_port_sync_lock(FVM_PortSync *s) { EnterCriticalSection(&s->mutex); }
void fvm_port_sync_unlock(FVM_PortSync *s) { LeaveCriticalSection(&s->mutex); }
void fvm_port_sync_wait(FVM_PortSync *s) { SleepConditionVariableCS(&s->condition,&s->mutex,INFINITE); }
void fvm_port_sync_signal_all(FVM_PortSync *s) { WakeAllConditionVariable(&s->condition); }
