/* Copyright (c) 2026 bhgv
 * EN: Native captured-root capabilities with source-pinned invalidation forwarding.
 * RU: Native captured-root capabilities с source-pinned forwarding invalidations.
 */
#include "provider_walk.h"
#include "cancel.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Bridge retains only a journal, never a provider or guest pointer in callbacks.
 * Bridge удерживает только journal, без provider/guest pointers в callbacks. */
typedef struct CaptureNotice
{
    DVM_Changes *target; /* Owned independent output journal pin through callback retirement. */
} CaptureNotice;


/* EN/RU: Rebase all source classes into the captured journal; IDs are unrelated, so clear scope.
 * Перенумеровать source classes в captured journal; resource IDs не связаны, scope очищается. */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    CaptureNotice *notice = state;
    changespost(notice->target, event->kinds, 0, 0);
}


/* EN/RU: Consume independent journal/context only after all admitted callbacks return.
 * Поглотить journal/context только после завершения admitted callbacks. */
static void released(void *state)
{
    CaptureNotice *notice = state;
    dvm_changes_destroy(notice->target);
    free(notice);
}


/* EN/RU: See provider.h; this callback is immutable after factory creation.
 * См. provider.h; callback immutable после factory creation. */
int dvm_provider_capturable(DVM_Provider *provider)
{
    return provider && provider->capture != NULL;
}


/* EN/RU: See provider.h; anchored is published before captured provider escapes to its caller.
 * См. provider.h; anchored публикуется до передачи captured provider caller. */
int dvm_provider_bindable_at(DVM_Provider *provider, const char *relative)
{
    return provider && relative && (dvm_provider_bindable(provider) ||
        (provider->anchored && !strcmp(relative, ".")));
}


/* EN/RU: See provider_internal.h; callbacks retain journals independently after unsubscribe.
 * См. provider_internal.h; callbacks удерживают journals после unsubscribe независимо. */
void dvm_provider_capture_detach(DVM_Provider *provider)
{
    dvm_notice_unsubscribe(provider->origin_tap);
    provider->origin_tap = NULL;
}


/* EN/RU: See provider.h; no source callback executes under either factory lifetime mutex.
 * См. provider.h; source callback не выполняется под lifetime mutex factory. */
DVM_Provider *dvm_provider_capture(DVM_Provider *provider, const char *relative)
{
    if (!provider || !relative || dvm_cancel_requested(dvm_cancel_current()))
    {
        return NULL;
    }
    if (!strcmp(relative, ".") && dvm_provider_bindable_at(provider, relative))
    {
        return dvm_provider_retain(provider) ? provider : NULL;
    }
    if (!provider->capture || (dvm_cancel_current() && !(provider->native_flags & DVM_PROVIDER_CANCEL_CAPTURE)))
    {
        return NULL;
    }
    WalkScope scope = {0};
    if (dvm_provider_walk_enter(provider, relative, WALK_CAPTURE, "", &scope) != FVM_OK)
    {
        return NULL;
    }
    DVM_Provider *captured = provider->capture(provider->state, relative);
    dvm_provider_walk_leave(&scope);
    if (!captured)
    {
        return NULL;
    }
    fvm_port_sync_lock(captured->lock);
    int fresh = captured != provider && captured->references == 1 && !captured->origin && !captured->origin_tap;
    fvm_port_sync_unlock(captured->lock);
    if (!fresh || (dvm_cancel_current() && !(captured->native_flags & DVM_PROVIDER_CANCEL_PATH)))
    {
        dvm_provider_release(captured);
        return NULL;
    }
    CaptureNotice *notice = calloc(1, sizeof(*notice));
    if (!notice || !changesretain(captured->changes))
    {
        free(notice);
        dvm_provider_release(captured);
        return NULL;
    }
    notice->target = captured->changes;
    if (!dvm_provider_retain(provider))
    {
        released(notice);
        dvm_provider_release(captured);
        return NULL;
    }
    captured->origin = provider;
    captured->origin_tap = dvm_notice_subscribe(dvm_changes_notices(provider->changes), changed, notice, released);
    if (!captured->origin_tap)
    {
        released(notice);
        dvm_provider_release(captured);
        return NULL;
    }
    captured->anchored = 1;
    return captured;
}
