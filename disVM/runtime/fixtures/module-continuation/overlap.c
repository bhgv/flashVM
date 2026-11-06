/* Private test gates make simultaneous admission independent of host timing. */
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include <pthread.h>

#define GUEST_COUNT 40 /* Independent guest threads reaching one copy each. */

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* Gate state. */
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER; /* Both test barriers. */
static unsigned arriving; /* Guests with published roots outside NativeWorld. */
static unsigned admitted; /* Guests that have acquired actual SHARED admission. */

int __real_dvm_native_owner_enter_shared(DVM_HeapOwner *owner);
int __real_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind);


/* Wait on one test-only counter. All guests own separate TLS contexts. */
static void meet(unsigned *count)
{
    pthread_mutex_lock(&mutex);
    ++*count;
    pthread_cond_broadcast(&changed);
    while (*count < GUEST_COUNT)
    {
        pthread_cond_wait(&changed, &mutex);
    }
    pthread_mutex_unlock(&mutex);
}


/* Delay each guest before and after the real admission. The copy still uses
 * the production executor, range guards and exclusive return path. */
int __wrap_dvm_native_owner_enter_shared(DVM_HeapOwner *owner)
{
    int result;
    meet(&arriving);
    result = __real_dvm_native_owner_enter_shared(owner);
    if (result)
    {
        meet(&admitted);
    }
    return result;
}


/* Delay collector requests until the artificial barriers release. The normal
 * collector can then run and drain ownership during the rest of the guest run. */
int __wrap_dvm_gc_workers_request(DVM_GCWorkers *workers, DVM_GCKind kind)
{
    int ready;
    pthread_mutex_lock(&mutex);
    ready = admitted >= GUEST_COUNT;
    pthread_mutex_unlock(&mutex);
    return !ready || __real_dvm_gc_workers_request(workers, kind);
}
