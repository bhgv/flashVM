/* Copyright (c) 2026 bhgv
 * EN: Keep script conversion on its owner while a native Styx exchange runs.
 * RU: Сохранять преобразование скрипта у владельца во время нативного обмена Styx.
 */
#ifndef FVM_STYXRPC_WIRE_WAIT_H
#define FVM_STYXRPC_WIRE_WAIT_H
#include "flashvm/worker.h"
#include "styxrpc/channel_arguments.h"
#include "styxrpc/serve.h"

typedef enum FVM_StyxWireKind
{
    FVM_STYX_WIRE_MANAGED,  /* Simple managed values and reference updates. */
    FVM_STYX_WIRE_GRAPH,    /* Graph values with positional updates. */
    FVM_STYX_WIRE_CHANNEL   /* Graph values with live channel grants. */
} FVM_StyxWireKind;

typedef struct FVM_StyxWireCall
{
    FVM_StyxWireKind kind;             /* Selects the existing wire codec. */
    struct StyxFid *directory;        /* Borrowed exact module or NULL for path resolution. */
    const StyxRpcManagedTarget *target; /* Borrowed immutable method contract. */
    const void *arguments;            /* Borrowed encoded arguments through joined return. */
    size_t size;                      /* Exact encoded argument extent. */
    void *reply;                      /* Owner-allocated reply storage. */
    size_t capacity;                  /* Reply capacity in bytes. */
    void *result;                     /* Native result matching kind. */
    StyxRpcArgumentScope *scope;       /* Optional channel scope consumed by the channel codec. */
} FVM_StyxWireCall;


/* EN: Run a native exchange once. With reverse, keep only native program requests
 * moving on the caller thread and join the wire worker before returning. Consumes
 * call->scope on every outcome. Borrow all native buffers through return.
 * Poll the stop flag of context's detached task, if present.
 * RU: Однократно выполнить нативный обмен. При reverse обслуживать только запросы
 * нативного дерева программ у вызывающего. Дождаться потока обмена. Всегда
 * поглотить call->scope, нативные буферы заимствовать до возврата.
 * Проверять флаг остановки отделённой задачи, если context ей принадлежит. */
StyxRpcOutcome fvm_styxrpc_wire_call(FVM_Context *context, StyxRpcServer *reverse, FVM_StyxWireCall *call);


/* EN: Borrow an optional worker and report its stop flag to the SDK.
 * RU: Заимствовать необязательную задачу и сообщить SDK её флаг остановки. */
int fvm_styxrpc_worker_stopped(void *worker);


/* EN: Return a static exception message for the wire outcome, NULL on success.
 * RU: Вернуть статическое сообщение об исходе вызова либо NULL при успехе. */
const char *fvm_styxrpc_call_error(StyxRpcOutcome outcome);

#endif
