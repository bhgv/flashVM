/* Exercise production native FIFO with real OS contexts, including active controls. */
#include "file_gate.h"
#include "disvm/native_context.h"
#include "channel_sync.h"
#include "thread_port.h"
#include <stdio.h>
#include <stdlib.h>


/* Native fixture storage remains alive until every worker joins. */
typedef struct Worker
{
    DVM_FileGate *gate;      /* Borrowed gate through join. */
    FVM_PortSync *sync;      /* Protect fixture outputs independently of the gate. */
    size_t ticket;          /* Reserved by the controller before thread creation. */
    int foreign;            /* Attempt completion using another context's ticket. */
    int done;               /* Published after worker completion under sync. */
    int failed;             /* Semantic refusal/order error under sync. */
    int *order;             /* Shared output array protected by fixture sync. */
    int *count;             /* Shared output count protected by fixture sync. */
    int identity;           /* Expected FIFO ordinal, independent of start order. */
} Worker;


/* Bind a genuine native context and attempt the requested operation. argument
 * borrows Worker until join; no heap/managed object is involved. All output
 * stores use the independent fixture mutex even in deliberately weakened gates. */
static void execute(void *argument)
{
    Worker *worker = argument;
    DVM_NativeContext *context = dvm_native_context_create(8, NULL);
    int failed = 0;
    if (!context || !dvm_native_context_bind(context))
    {
        fprintf(stderr, "FAIL native fixture context\n");
        exit(2);
    }
    if (worker->foreign)
    {
        failed = dvm_file_gate_leave(worker->gate, worker->ticket);
    }
    else if (!dvm_file_gate_enter(worker->gate, worker->ticket))
    {
        failed = 1;
    }
    else
    {
        fvm_port_sync_lock(worker->sync);
        worker->order[(*worker->count)++] = worker->identity;
        fvm_port_sync_unlock(worker->sync);
        if (!dvm_file_gate_leave(worker->gate, worker->ticket))
        {
            failed = 1;
        }
    }
    if (!dvm_native_context_unbind(context) || !dvm_native_context_destroy(context))
    {
        fprintf(stderr, "FAIL native fixture retirement\n");
        exit(2);
    }
    fvm_port_sync_lock(worker->sync);
    worker->failed = failed;
    worker->done = 1;
    fvm_port_sync_unlock(worker->sync);
}


/* Native controller serializes reservations/lifetime as the runtime heap gate
 * does. Reverse-start all contenders; completed order must remain0..7. Return1
 * only for a semantic guard failure,2 for fixture setup,0 on all checks. */
int main(void)
{
    DVM_NativeContext *context = dvm_native_context_create(8, NULL);
    DVM_FileGate *gate = dvm_file_gate_create();
    FVM_PortSync *sync = fvm_port_sync_create();
    FVM_PortThread *threads[8];
    Worker workers[8] = {{0}};
    Worker foreign = {0};
    FVM_PortThread *challenger;
    size_t ticket;
    size_t waiting = 0;
    size_t pending;
    int order[8] = {0};
    int count = 0;
    int index;
    if (!context || !gate || !sync || !dvm_native_context_bind(context))
    {
        return 2;
    }
    ticket = dvm_file_gate_reserve(gate);
    if (!dvm_file_gate_enter(gate, ticket))
    {
        return 2;
    }
    foreign.gate = gate;
    foreign.sync = sync;
    foreign.ticket = ticket;
    foreign.foreign = 1;
    challenger = fvm_port_thread_start(execute, &foreign);
    if (!challenger)
    {
        return 2;
    }
    fvm_port_thread_join(challenger);
    if (foreign.failed)
    {
        fprintf(stderr, "FAIL foreign file completion accepted\n");
        return 1;
    }
    if (dvm_file_gate_destroy(gate))
    {
        fprintf(stderr, "FAIL busy file gate retired\n");
        return 1;
    }
    if (!dvm_file_gate_leave(gate, ticket) || dvm_file_gate_leave(gate, ticket))
    {
        fprintf(stderr, "FAIL duplicate file completion accepted\n");
        return 1;
    }
    for (index = 0; index < 8; index++)
    {
        workers[index].gate = gate;
        workers[index].sync = sync;
        workers[index].ticket = dvm_file_gate_reserve(gate);
        workers[index].identity = index;
        workers[index].order = order;
        workers[index].count = &count;
    }
    if (dvm_file_gate_destroy(gate))
    {
        fprintf(stderr, "FAIL reserved file gate retired\n");
        return 1;
    }
    for (index = 7; index >= 0; index--)
    {
        threads[index] = fvm_port_thread_start(execute, &workers[index]);
        if (!threads[index])
        {
            return 2;
        }
        if (index)
        {
            for (;;)
            {
                size_t current;
                int done;
                dvm_file_gate_snapshot(gate, NULL, &current);
                fvm_port_sync_lock(sync);
                done = workers[index].done;
                fvm_port_sync_unlock(sync);
                if (current > waiting || done)
                {
                    waiting = current;
                    break;
                }
                fvm_port_thread_sleep_us(50);
            }
        }
    }
    for (index = 0; index < 8; index++)
    {
        fvm_port_thread_join(threads[index]);
    }
    dvm_file_gate_snapshot(gate, &pending, &waiting);
    if (count != 8 || pending || waiting < 7)
    {
        fprintf(stderr, "FAIL FIFO file admission order violated\n");
        return 1;
    }
    for (index = 0; index < 8; index++)
    {
        if (workers[index].failed || order[index] != index)
        {
            fprintf(stderr, "FAIL FIFO file admission order violated\n");
            return 1;
        }
    }
    if (!dvm_file_gate_destroy(gate) || !dvm_native_context_unbind(context) ||
        !dvm_native_context_destroy(context))
    {
        return 2;
    }
    fvm_port_sync_destroy(sync);
    dvm_file_gate_statistics();
    puts("PASS native file FIFO: reverse-start8, foreign/duplicate finish, busy/reserved lifetime");
    return 0;
}
