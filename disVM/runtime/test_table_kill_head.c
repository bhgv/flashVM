#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include <unistd.h>

static int table_victim_pid;
static int table_retry_ready;
static int table_kill_delivered;

int __real_dvm_bigtable_call_ready(void (*function)(void *), void *frame);
int __wrap_dvm_bigtable_call_ready(void (*function)(void *), void *frame)
{
    int ready = __real_dvm_bigtable_call_ready(function, frame);

    if (!ready)
    {
        int unset = 0;
        __atomic_compare_exchange_n(&table_victim_pid, &unset, currun()->pid,
            0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
    }
    return ready;
}

int __real_dvm_runtime_kill_child(Prog *parent, int pid);
int __wrap_dvm_runtime_kill_child(Prog *parent, int pid)
{
    int result;
    int gated = pid == __atomic_load_n(&table_victim_pid, __ATOMIC_ACQUIRE);

    if (gated)
    {
        while (!__atomic_load_n(&table_retry_ready, __ATOMIC_ACQUIRE))
            usleep(1000);
    }
    result = __real_dvm_runtime_kill_child(parent, pid);
    if (gated)
        __atomic_store_n(&table_kill_delivered, 1, __ATOMIC_RELEASE);
    return result;
}

int __real_dvm_runqueue_claim(Prog *process);

int __wrap_dvm_runqueue_claim(Prog *process)
{
    /* The first table waiter has been granted a turn. Hold its retry
     * until the parent has posted termination to that thread. */
    if (process->pid == __atomic_load_n(&table_victim_pid, __ATOMIC_ACQUIRE))
    {
        __atomic_store_n(&table_retry_ready, 1, __ATOMIC_RELEASE);
        while (!__atomic_load_n(&table_kill_delivered, __ATOMIC_ACQUIRE))
            usleep(1000);
    }
    return __real_dvm_runqueue_claim(process);
}
