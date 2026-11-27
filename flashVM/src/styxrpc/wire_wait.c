/* Copyright (c) 2026 bhgv
 * EN: Drive only native reverse routes while the movie waits for an exact Dis call.
 * RU: Обслуживать только нативные встречные маршруты, пока фильм ждёт вызов Dis.
 */
#include "wire_wait.h"
#include "styxrpc/graph_client.h"
#include "styxrpc/managed_client.h"
#include "../../ports/channel_sync.h"
#include "../../ports/thread_port.h"

#define REVERSE_WAIT_SLICE_MS 5 /* Resume the owner promptly after the wire thread completes. */

typedef struct WireWait
{
    FVM_StyxWireCall *call; /* Borrowed native inputs and result storage. */
    FVM_PortSync *lock;     /* Guards done until the join. */
    StyxRpcOutcome result; /* Written by the wire thread, read after join. */
    int done;              /* Publication of the completed wire result. */
} WireWait;


/* EN/RU: See wire_wait.h. / См. wire_wait.h. */
int fvm_styxrpc_worker_stopped(void *worker)
{
    int requested = 0;
    return worker && fvm_worker_stop_requested(worker, &requested) == FVM_OK && requested;
}


/* EN/RU: See wire_wait.h. / См. wire_wait.h. */
const char *fvm_styxrpc_call_error(StyxRpcOutcome outcome)
{
    switch (outcome)
    {
    case STYXRPC_CALL_OK:
        return NULL;
    case STYXRPC_CALL_NOT_SENT:
        return "Styx RPC call was not sent";
    case STYXRPC_CALL_REMOTE_ERROR:
        return "Styx RPC remote function failed";
    case STYXRPC_CALL_CANCELLED:
        return "Styx RPC call cancelled before submission";
    case STYXRPC_CALL_CANCELLED_UNCERTAIN:
        return "Styx RPC call cancelled; remote outcome is uncertain";
    default:
        return "Styx RPC outcome is uncertain";
    }
}


/* EN: Dispatch exact native codecs without accessing the VM or invoking script.
 * RU: Вызвать точные нативные кодеки без доступа к VM и запусков скрипта. */
static StyxRpcOutcome exchange(FVM_StyxWireCall *call, FVM_Worker *worker)
{
    StyxRpcArgumentScope *scope = call->scope;
    StyxRpcCallStop stop = worker ? fvm_styxrpc_worker_stopped : NULL;
    call->scope = NULL;
    switch (call->kind)
    {
    case FVM_STYX_WIRE_MANAGED:
        return styxrpc_managed_invoke_at_until(call->directory, call->target, call->arguments, call->size,
            call->reply, call->capacity, call->result, stop, worker);
    case FVM_STYX_WIRE_GRAPH:
        return styxrpc_graph_invoke_at_until(call->directory, call->target, call->arguments, call->size,
            call->reply, call->capacity, call->result, stop, worker);
    case FVM_STYX_WIRE_CHANNEL:
        return styxrpc_argument_scope_invoke_at_until(call->directory, scope, call->target, call->arguments,
            call->size, call->reply, call->capacity, call->result, stop, worker);
    }
    styxrpc_argument_scope_close(scope);
    return STYXRPC_CALL_NOT_SENT;
}


/* EN: Complete the native job and signal owner-visible completion.
 * RU: Завершить нативное задание и опубликовать завершение для владельца. */
static void run_wire(void *state)
{
    WireWait *wait = state;
    wait->result = exchange(wait->call, NULL);
    fvm_port_sync_lock(wait->lock);
    wait->done = 1;
    fvm_port_sync_unlock(wait->lock);
}


/* EN: Sample the completion flag without keeping the lock across transport actions.
 * RU: Прочитать флаг завершения, не удерживая мьютекс во время работы транспорта. */
static int completed(WireWait *wait)
{
    int done;
    fvm_port_sync_lock(wait->lock);
    done = wait->done;
    fvm_port_sync_unlock(wait->lock);
    return done;
}


/* EN/RU: See wire_wait.h. / См. wire_wait.h. */
StyxRpcOutcome fvm_styxrpc_wire_call(FVM_Context *context, StyxRpcServer *reverse, FVM_StyxWireCall *call)
{
    WireWait wait = {0};
    FVM_PortThread *thread = NULL;
    if (!reverse)
    {
        FVM_Worker *worker = NULL;
        fvm_worker_current(context, &worker);
        return exchange(call, worker);
    }
    wait.call = call;
    if (styxrpc_server_program_step_for(reverse, 0))
    {
        wait.lock = fvm_port_sync_create();
        if (wait.lock)
        {
            thread = fvm_port_thread_start(run_wire, &wait);
        }
    }
    if (thread)
    {
        while (!completed(&wait) && styxrpc_server_program_step_for(reverse, REVERSE_WAIT_SLICE_MS))
        {
        }
        fvm_port_thread_join(thread);
    }
    else
    {
        styxrpc_argument_scope_close(call->scope);
        call->scope = NULL;
        wait.result = STYXRPC_CALL_NOT_SENT;
    }
    if (wait.lock)
    {
        fvm_port_sync_destroy(wait.lock);
    }
    return wait.result;
}
