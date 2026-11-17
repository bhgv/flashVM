/* Copyright (c) 2026 bhgv
 * EN: Sticky native cancellation with source-pinned taps and independent callback retirement.
 * RU: Sticky native cancellation с source-pinned taps и independent callback retirement.
 */
#include "cancel.h"
#include "changes.h"
#include "channel_sync.h"
#include <stdio.h>
#include <stdlib.h>


/* EN/RU: Native source lifetime is independent of any guest/provider lifetime.
 * Native source lifetime не зависит от guest/provider lifetime. */
struct DVM_Cancel
{
    FVM_PortSync *lock; /* Protects references and sticky request. */
    size_t references; /* Caller, scopes and attached tap handles. */
    int requested; /* Monotonic cancellation predicate. */
    DVM_NoticeHub *notices; /* Owned native callback dispatcher. */
};


/* EN/RU: Caller tap pins source; callback state has its own notice-flight lifetime.
 * Caller tap удерживает source; callback state живёт по notice-flight lifetime. */
struct DVM_CancelTap
{
    DVM_Cancel *source; /* Owned source pin. */
    DVM_NoticeTap *tap; /* Owned callback subscription. */
};


/* EN/RU: Retained solely by native dispatcher, with no pointer back to source/guest.
 * Удерживается только native dispatcher, без pointer назад к source/guest. */
typedef struct Receiver
{
    void (*receive)(void *); /* Idempotent native interrupt signal. */
    void (*release)(void *); /* Optional final consuming cleanup. */
    void *state; /* Owned native cancellation capability. */
} Receiver;
static _Thread_local DVM_CancelScope *current; /* Borrowed LIFO native stack, never VM state. */


/* EN/RU: Forward one native signal; event contents do not grant authority.
 * Передать native signal; event не даёт authority. */
static void receive(void *opaque, const DVM_ChangeEvent *event)
{
    Receiver *receiver = opaque;
    (void)event;
    receiver->receive(receiver->state);
}


/* EN/RU: Consume native receiver after unsubscribe and every admitted callback.
 * Поглотить native receiver после unsubscribe и всех admitted callbacks. */
static void release(void *opaque)
{
    Receiver *receiver = opaque;
    if (receiver->release)
    {
        receiver->release(receiver->state);
    }
    free(receiver);
}


/* EN/RU: See cancel.h; failure adopts no external state.
 * См. cancel.h; failure не принимает external state. */
DVM_Cancel *dvm_cancel_create(void)
{
    DVM_Cancel *cancel = calloc(1, sizeof(*cancel));
    if (!cancel)
    {
        return NULL;
    }
    cancel->lock = fvm_port_sync_create();
    cancel->notices = dvm_notice_create();
    if (!cancel->lock || !cancel->notices)
    {
        if (cancel->lock)
        {
            fvm_port_sync_destroy(cancel->lock);
        }
        dvm_notice_destroy(cancel->notices);
        free(cancel);
        return NULL;
    }
    cancel->references = 1;
    return cancel;
}


/* EN/RU: See cancel.h; a live caller pin excludes concurrent final release.
 * См. cancel.h; live caller pin исключает concurrent final release. */
int dvm_cancel_retain(DVM_Cancel *cancel)
{
    if (!cancel)
    {
        return 0;
    }
    fvm_port_sync_lock(cancel->lock);
    int valid = cancel->references != (size_t)-1;
    if (valid)
    {
        cancel->references++;
    }
    fvm_port_sync_unlock(cancel->lock);
    return valid;
}


/* EN/RU: See cancel.h; attached tap handles own pins, detached flights own their hub.
 * См. cancel.h; attached tap handles владеют pins, detached flights — hub. */
void dvm_cancel_release(DVM_Cancel *cancel)
{
    if (!cancel)
    {
        return;
    }
    fvm_port_sync_lock(cancel->lock);
    int last = --cancel->references == 0;
    fvm_port_sync_unlock(cancel->lock);
    if (last)
    {
        if (!dvm_notice_destroy(cancel->notices))
        {
            fprintf(stderr, "disvm: cancellation source retired with attached taps\n");
            abort();
        }
        fvm_port_sync_destroy(cancel->lock);
        free(cancel);
    }
}


/* EN/RU: See cancel.h; publish once outside the source mutex.
 * См. cancel.h; публикация один раз вне source mutex. */
void dvm_cancel_request(DVM_Cancel *cancel)
{
    if (!cancel)
    {
        return;
    }
    fvm_port_sync_lock(cancel->lock);
    int first = !cancel->requested;
    cancel->requested = 1;
    fvm_port_sync_unlock(cancel->lock);
    if (first)
    {
        DVM_ChangeEvent event = {0};
        dvm_notice_publish(cancel->notices, &event);
    }
}


/* EN/RU: See cancel.h; no callback or I/O.
 * См. cancel.h; без callback или I/O. */
int dvm_cancel_requested(DVM_Cancel *cancel)
{
    if (!cancel)
    {
        return 0;
    }
    fvm_port_sync_lock(cancel->lock);
    int requested = cancel->requested;
    fvm_port_sync_unlock(cancel->lock);
    return requested;
}


/* EN/RU: See cancel.h; subscribe before replay so cancellation cannot be lost.
 * См. cancel.h; subscribe до replay, чтобы cancellation не терялась. */
DVM_CancelTap *dvm_cancel_subscribe(DVM_Cancel *cancel, void (*callback)(void *), void *state, void (*cleanup)(void *))
{
    if (!callback || !dvm_cancel_retain(cancel))
    {
        return NULL;
    }
    DVM_CancelTap *tap = calloc(1, sizeof(*tap));
    Receiver *receiver = calloc(1, sizeof(*receiver));
    if (!tap || !receiver)
    {
        free(tap);
        free(receiver);
        dvm_cancel_release(cancel);
        return NULL;
    }
    receiver->receive = callback;
    receiver->release = cleanup;
    receiver->state = state;
    tap->source = cancel;
    tap->tap = dvm_notice_subscribe(cancel->notices, receive, receiver, release);
    if (!tap->tap)
    {
        free(receiver);
        free(tap);
        dvm_cancel_release(cancel);
        return NULL;
    }
    if (dvm_cancel_requested(cancel))
    {
        callback(state);
    }
    return tap;
}


/* EN/RU: See cancel.h; callback storage can outlive this caller handle.
 * См. cancel.h; callback storage может пережить caller handle. */
void dvm_cancel_unsubscribe(DVM_CancelTap *tap)
{
    if (tap)
    {
        dvm_notice_unsubscribe(tap->tap);
        dvm_cancel_release(tap->source);
        free(tap);
    }
}


/* EN/RU: See cancel.h; retain before publishing current-thread scope.
 * См. cancel.h; retain до публикации current-thread scope. */
int dvm_cancel_enter(DVM_CancelScope *scope, DVM_Cancel *cancel)
{
    if (!scope || scope->cancel || !dvm_cancel_retain(cancel))
    {
        return 0;
    }
    scope->previous = current;
    scope->cancel = cancel;
    current = scope;
    return 1;
}


/* EN/RU: See cancel.h; refuse foreign/out-of-order scope without changing its owner.
 * См. cancel.h; foreign/out-of-order scope отклонить без изменения owner. */
int dvm_cancel_leave(DVM_CancelScope *scope)
{
    if (!scope || current != scope || !scope->cancel)
    {
        return 0;
    }
    DVM_Cancel *cancel = scope->cancel;
    current = scope->previous;
    scope->cancel = NULL;
    scope->previous = NULL;
    dvm_cancel_release(cancel);
    return 1;
}


/* EN/RU: See cancel.h; lifetime belongs to current native stack scope.
 * См. cancel.h; lifetime принадлежит current native stack scope. */
DVM_Cancel *dvm_cancel_current(void)
{
    return current ? current->cancel : NULL;
}
