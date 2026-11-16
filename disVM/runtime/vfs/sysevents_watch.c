/* Copyright (c) 2026 bhgv
 * EN: Source-driven push subscriptions; no scheduler/owner/reader polling.
 * RU: Source-driven push subscriptions; без scheduler/owner/reader polling.
 */
#include "internal.h"
#include "sysevents_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: Final in-flight tap release frees context after retaining recipient/source.
 * Final tap release освобождает context после in-flight callbacks. */
static void watch_destroy(void *state)
{
    DVM_SysWatch *watch = state;
    DVM_SysEvents *events = watch->events;
    dvm_provider_release(watch->provider);
    free(watch);
    dvm_sysevents_release(events);
}


/* EN/RU: See internal header; caller holds no inbox/namespace/source lock.
 * См. internal header; caller не удерживает inbox/namespace/source lock. */
void dvm_sysevents_watches_free(DVM_SysWatch *watch)
{
    while (watch)
    {
        DVM_SysWatch *next = watch->next;
        if (watch->tap)
        {
            dvm_notice_unsubscribe(watch->tap);
        }
        else
        {
            watch_destroy(watch);
        }
        watch = next;
    }
}


/* EN/RU: Sender directly queues one recipient notification without executing or
 * pausing its guest. Epoch fence rejects detached namespace scopes.
 * Sender непосредственно ставит notification без исполнения/паузы guest target. */
static void changed(void *state, const DVM_ChangeEvent *event)
{
    DVM_SysWatch *watch = state;
    DVM_SysEvents *events = watch->events;
    fvm_port_sync_lock(events->lock);
    if (!events->retired && !events->scope_closed && events->epoch == watch->epoch &&
        (event->session > watch->last_session || (event->session == watch->last_session &&
         event->generation > watch->last_generation)))
    {
        unsigned kinds = event->kinds;
        if (event->session != watch->last_session || event->generation - watch->last_generation > 1)
        {
            kinds = DVM_CHANGE_ALL;
        }
        dvm_sysevents_post(events, DVM_SYSEVENT_VFS, watch->identity, kinds,
                          event->session, event->generation, watch->on);
        watch->last_session = event->session;
        watch->last_generation = event->generation;
    }
    fvm_port_sync_unlock(events->lock);
}


/* EN/RU: Detach an exact unchanged binding from the old owned list, under inbox lock.
 * Group-local identities are compared only within the same retained namespace.
 * Под inbox lock извлечь неизменившуюся binding из old list; IDs сравниваются
 * только внутри того же namespace. Return owned watch or NULL, no allocation. */
static DVM_SysWatch *reuse(DVM_SysWatch **old, const DVM_NamespaceBinding *binding)
{
    DVM_SysWatch **link;
    for (link = old; *link; link = &(*link)->next)
    {
        DVM_SysWatch *watch = *link;
        if (watch->tap && watch->identity == binding->identity && watch->provider == binding->provider &&
            !strcmp(watch->on, binding->on))
        {
            *link = watch->next;
            watch->next = NULL;
            return watch;
        }
    }
    return NULL;
}


/* EN/RU: Allocate one new binding subscription under inbox/group locks. No source I/O.
 * Failed partially owned contexts go to discard for cleanup after unlocking.
 * Создать новую subscription; partial contexts передать discard для cleanup вне
 * locks. Return owned watch/NULL; retain provider and inbox only on live context. */
static DVM_SysWatch *create(DVM_SysEvents *events, const DVM_NamespaceBinding *binding,
                             DVM_SysWatch **discard)
{
    DVM_SysWatch *watch;
    DVM_Changes *changes;
    DVM_ChangeBatch initial;
    DVM_ChangeCursor empty = {0};
    if (events->references == (size_t)-1)
    {
        return NULL;
    }
    watch = calloc(1, sizeof(*watch));
    if (!watch || !dvm_provider_retain(binding->provider))
    {
        free(watch);
        return NULL;
    }
    watch->provider = binding->provider;
    watch->events = events;
    events->references++;
    watch->identity = binding->identity;
    watch->epoch = events->epoch;
    strcpy(watch->on, binding->on);
    changes = dvm_provider_changes(watch->provider);
    watch->tap = dvm_notice_subscribe(dvm_changes_notices(changes), changed, watch, watch_destroy);
    if (!watch->tap)
    {
        watch->next = *discard;
        *discard = watch;
        return NULL;
    }
    if (dvm_changes_pending(changes, &empty, DVM_CHANGE_ALL, &initial) == 1)
    {
        watch->last_session = initial.event.session;
        watch->last_generation = initial.event.generation;
        dvm_sysevents_post(events, DVM_SYSEVENT_VFS, watch->identity, DVM_CHANGE_ALL,
            initial.event.session, initial.event.generation, watch->on);
    }
    return watch;
}


/* EN/RU: Native mutation reconciles exact bindings, keeping unchanged taps and
 * generation watermarks without allocation or source queries. Missing new bindings
 * mark resync; failure never drops other valid subscriptions. No target execution.
 * Native mutation сохраняет unchanged taps/watermarks; failure новой subscription
 * даёт resync, не отключая остальные. Removed taps очищаются вне locks. */
void dvm_sysevents_namespace(void *state, const DVM_ChangeEvent *event)
{
    DVM_SysEvents *events = state;
    DVM_SysWatch *head = NULL, **tail = &head, *old;
    DVM_NamespaceBinding *binding;
    DVM_NamespaceBinding cwd = {0}; /* Temporary native description; no ownership escapes. */
    uint64_t version;
    int complete = 1;
    (void)event;
    fvm_port_sync_lock(events->lock);
    if (events->retired)
    {
        fvm_port_sync_unlock(events->lock);
        return;
    }
    fvm_port_sync_lock(events->group->lock);
    version = events->group->version;
    if (events->captured && version == events->version)
    {
        fvm_port_sync_unlock(events->group->lock);
        fvm_port_sync_unlock(events->lock);
        return;
    }
    dvm_sysevents_purge(events);
    old = events->watches;
    if (events->epoch == UINT64_MAX)
    {
        events->scope_closed = 1;
        events->watches = NULL;
        events->captured = 0;
        events->resync = 1;
        fvm_port_sync_unlock(events->group->lock);
        fvm_port_sync_unlock(events->lock);
        dvm_sysevents_watches_free(old);
        return;
    }
    events->epoch++;
    dvm_sysevents_post(events, DVM_SYSEVENT_NAMESPACE, 0, DVM_CHANGE_ALL, 0, version, "/");
    cwd.provider = events->group->cwd_provider;
    cwd.identity = events->group->cwd_identity;
    strcpy(cwd.on, events->group->cwd);
    cwd.next = events->group->bindings;
    for (binding = cwd.provider ? &cwd : cwd.next; binding; binding = binding->next)
    {
        DVM_SysWatch *watch;
        if (!binding->provider || dvm_sysevents_is_device(binding->provider))
        {
            continue;
        }
        watch = reuse(&old, binding);
        if (!watch)
        {
            watch = create(events, binding, &old);
        }
        if (!watch)
        {
            complete = 0;
            continue;
        }
        watch->epoch = events->epoch;
        *tail = watch;
        tail = &watch->next;
    }
    fvm_port_sync_unlock(events->group->lock);
    events->watches = head;
    events->captured = complete;
    events->version = version;
    if (!complete)
    {
        events->resync = 1;
    }
    fvm_port_sync_unlock(events->lock);
    dvm_sysevents_watches_free(old);
}


/* EN/RU: Consume the namespace tap's native recipient reference.
 * Поглотить native recipient reference namespace tap. */
static void recipient_release(void *state)
{
    dvm_sysevents_release(state);
}


/* EN/RU: See internal header; callback registration retains recipient before admission.
 * См. internal header; удержать recipient до callback admission. */
DVM_NoticeTap *dvm_sysevents_subscribe_group(DVM_SysEvents *events, DVM_Namespace *group)
{
    DVM_NoticeTap *tap;
    if (!dvm_sysevents_retain(events))
    {
        return NULL;
    }
    tap = dvm_notice_subscribe(group->notices, dvm_sysevents_namespace, events, recipient_release);
    if (!tap)
    {
        dvm_sysevents_release(events);
    }
    return tap;
}
