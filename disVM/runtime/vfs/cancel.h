/* Copyright (c) 2026 bhgv
 * EN: Native operation cancellation and thread-local synchronous provider scopes.
 * RU: Native operation cancellation и thread-local scopes синхронных provider calls.
 */
#ifndef DVM_VFS_CANCEL_H
#define DVM_VFS_CANCEL_H
typedef struct DVM_Cancel DVM_Cancel; /* Owned native sticky cancellation source. */
typedef struct DVM_CancelTap DVM_CancelTap; /* Owned source-pinning native subscription. */


/* EN/RU: Scope is caller-owned zeroed storage on one native thread until leave.
 * Scope — zeroed caller storage одного native thread до leave; no VM pointers. */
typedef struct DVM_CancelScope
{
    DVM_Cancel *cancel; /* Owned pin while entered. */
    struct DVM_CancelScope *previous; /* Borrowed enclosing native stack scope. */
} DVM_CancelScope;


/* EN/RU: Create owned native source/NULL on allocation/lock failure; no callbacks.
 * Создать свой native source/NULL при allocation/lock failure; без callbacks. */
DVM_Cancel *dvm_cancel_create(void);


/* EN/RU: Retain borrowed live source1/0 on NULL/overflow; no callbacks or I/O.
 * Удержать borrowed live source1/0 при NULL/overflow; без callbacks/I/O. */
int dvm_cancel_retain(DVM_Cancel *cancel);


/* EN/RU: Consume owner ref; NULL harmless, final release never waits for callbacks.
 * Поглотить owner ref; NULL допустим, final release не ожидает callbacks. */
void dvm_cancel_release(DVM_Cancel *cancel);


/* EN/RU: Sticky nonwaiting request; no guest/transport I/O. Query borrowed source1/0.
 * Callbacks must be idempotent, native nonwaiting and must not enter the host wait
 * ledger, VM or provider I/O. Запрос sticky без ожидания; query1/0; callbacks не
 * входят в host wait ledger, VM или provider I/O и могут вызываться повторно. */
void dvm_cancel_request(DVM_Cancel *cancel);


/* EN/RU: Query borrowed source1/0; NULL gives0, no callback or authority grant.
 * Query borrowed source1/0; NULL даёт0, без callback или authority grant. */
int dvm_cancel_requested(DVM_Cancel *cancel);


/* EN/RU: Own state only on success; replay sticky request, possibly racing delivery.
 * Returned tap owns a source pin; unsubscribe consumes it without callback wait.
 * release(state) runs after every admitted callback; NULL failure leaves state owned
 * by caller. No provider/Prog/guest pointers in callback state.
 * State принят только при успехе; sticky replay может дублировать delivery;
 * unsubscribe без ожидания, release после callbacks; NULL оставляет state caller. */
DVM_CancelTap *dvm_cancel_subscribe(DVM_Cancel *cancel, void (*receive)(void *), void *state, void (*release)(void *));


/* EN/RU: Consume tap/source pin; NULL harmless, admitted callbacks own state until release.
 * Поглотить tap/source pin; NULL допустим, callbacks владеют state до release. */
void dvm_cancel_unsubscribe(DVM_CancelTap *tap);


/* EN/RU: Enter/leave synchronous native scope1/0; leave must match thread and LIFO.
 * Current is borrowed under scope pin. Scopes do not propagate to new native threads.
 * Войти/выйти из synchronous scope1/0; тот же thread/LIFO; current borrowed;
 * новые native threads не наследуют scope. No allocation or guest access. */
int dvm_cancel_enter(DVM_CancelScope *scope, DVM_Cancel *cancel);


/* EN/RU: Consume entered scope pin1/0; reject foreign/LIFO mismatch without mutation.
 * Поглотить entered scope pin1/0; foreign/LIFO mismatch отклонить без mutation. */
int dvm_cancel_leave(DVM_CancelScope *scope);


/* EN/RU: Borrow current synchronous native source/NULL; scope pins it, no VM access.
 * Заимствовать current synchronous native source/NULL; scope удерживает, без VM. */
DVM_Cancel *dvm_cancel_current(void);
#endif
