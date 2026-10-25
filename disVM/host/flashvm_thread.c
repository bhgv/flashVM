/* Native execution uses FlashVM's selected Linux/Windows/macOS/Android port. */
#include "disvm/flashvm_host.h"


/* Start an owning native host thread; its callback can drive Dis's scheduler. */
FVM_PortThread *dvm_host_thread_start(FVM_PortThreadFn callback, void *user)
{
    return fvm_port_thread_start(callback, user);
}


/* Join and release the handle after the scheduler/worker has stopped. */
void dvm_host_thread_join(FVM_PortThread *thread)
{
    fvm_port_thread_join(thread);
}


/* Allocate the same mutex/condition pair used by FlashVM channels/workers. */
FVM_PortSync *dvm_host_sync_create(void)
{
    return fvm_port_sync_create();
}


/* Caller guarantees no waiter still borrows the synchronization object. */
void dvm_host_sync_destroy(FVM_PortSync *sync)
{
    fvm_port_sync_destroy(sync);
}


/* Serialize a host-side queue; this is not a Dis heap or interpreter lock. */
void dvm_host_sync_lock(FVM_PortSync *sync)
{
    fvm_port_sync_lock(sync);
}


/* Release the host-side queue lock after publishing predicate changes. */
void dvm_host_sync_unlock(FVM_PortSync *sync)
{
    fvm_port_sync_unlock(sync);
}


/* Wait in a predicate loop while holding the matching host-side lock. */
void dvm_host_sync_wait(FVM_PortSync *sync)
{
    fvm_port_sync_wait(sync);
}


/* Wake all waiters after publishing a changed predicate under the same lock. */
void dvm_host_sync_wake_all(FVM_PortSync *sync)
{
    fvm_port_sync_signal_all(sync);
}
