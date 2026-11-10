#define _GNU_SOURCE
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int table_victim_pid;
static int table_retry_ready;
static int table_control_delivered;

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

static int gated_control(int (*request)(Prog *, int), Prog *parent, int pid)
{
    int result;
    int gated = pid == __atomic_load_n(&table_victim_pid, __ATOMIC_ACQUIRE);

    if (gated)
    {
        while (!__atomic_load_n(&table_retry_ready, __ATOMIC_ACQUIRE))
            usleep(1000);
    }
    result = request(parent, pid);
    if (gated)
        __atomic_store_n(&table_control_delivered, 1, __ATOMIC_RELEASE);
    return result;
}

int __real_dvm_runtime_kill_child(Prog *parent, int pid);
int __wrap_dvm_runtime_kill_child(Prog *parent, int pid)
{
    return gated_control(__real_dvm_runtime_kill_child, parent, pid);
}

int __real_dvm_runtime_prepare_cancelwait(Prog *parent, int pid);
int __wrap_dvm_runtime_prepare_cancelwait(Prog *parent, int pid)
{
    return gated_control(__real_dvm_runtime_prepare_cancelwait, parent, pid);
}

int __real_dvm_runtime_table_retry_begin(Prog *process);
int __wrap_dvm_runtime_table_retry_begin(Prog *process)
{
    const char *mode = getenv("DVM_TEST_TABLE_RETRY_ADMISSION");
    if (mode && process->pid == __atomic_load_n(&table_victim_pid, __ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&table_retry_ready, __ATOMIC_ACQUIRE))
    {
        int late = strcmp(mode, "late") == 0;
        int ready = late ? __real_dvm_runtime_table_retry_begin(process) : 0;
        __atomic_store_n(&table_retry_ready, 1, __ATOMIC_RELEASE);
        while (!__atomic_load_n(&table_control_delivered, __ATOMIC_ACQUIRE))
            usleep(1000);
        puts(late ? "PASS cancellation after retry admission" : "PASS cancellation before retry admission");
        return late ? ready : __real_dvm_runtime_table_retry_begin(process);
    }
    return __real_dvm_runtime_table_retry_begin(process);
}

int __real_dvm_runqueue_finish(Prog *process);

int __wrap_dvm_runqueue_finish(Prog *process)
{
    /* Hold the first waiter at the end of its parked turn
     * until the parent has posted termination or cancellation. */
    if (!getenv("DVM_TEST_TABLE_RETRY_ADMISSION") &&
        process->pid == __atomic_load_n(&table_victim_pid, __ATOMIC_ACQUIRE) &&
        !__atomic_load_n(&table_retry_ready, __ATOMIC_ACQUIRE))
    {
        __atomic_store_n(&table_retry_ready, 1, __ATOMIC_RELEASE);
        while (!__atomic_load_n(&table_control_delivered, __ATOMIC_ACQUIRE))
            usleep(1000);
    }
    return __real_dvm_runqueue_finish(process);
}
