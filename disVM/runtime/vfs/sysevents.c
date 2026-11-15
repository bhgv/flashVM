/* Copyright (c) 2026 bhgv
 * EN: Native system inbox lifecycle and bounded queues using owner_messages.
 * RU: Native system inbox lifecycle и bounded queues через owner_messages.
 */
#include "sysevents_internal.h"
#include <stdlib.h>
#include <string.h>


/* EN/RU: See internal header; never consumes request/reply control mailboxes.
 * См. internal header; control mailboxes request/reply не затрагиваются. */
void dvm_sysevents_purge(DVM_SysEvents *events)
{
    DVM_OwnerMessage *message;
    while ((message = dvm_messages_take(events->endpoint, 0, 0)))
    {
        free(message);
    }
    free(events->head);
    events->head = NULL;
    events->pending = 0;
    events->resync = 0;
}


/* EN/RU: See internal header; overflow broadens detail, never silently signals EOF.
 * См. internal header; overflow расширяет detail, не сигнализирует EOF. */
int dvm_sysevents_post(DVM_SysEvents *events, unsigned kind, uint64_t source,
                       unsigned classes, uint64_t session, uint64_t generation, const char *on)
{
    DVM_SysMessage *message;
    if (events->pending >= DVM_SYSEVENT_LIMIT)
    {
        dvm_sysevents_purge(events);
        events->resync = 1;
    }
    message = calloc(1, sizeof(*message) + strlen(on));
    if (!message)
    {
        events->resync = 1;
        return 0;
    }
    message->message.kind = DVM_SYSEVENT_MESSAGE_BASE + kind;
    message->message.sender = (uintptr_t)source;
    message->message.request = generation;
    message->message.value = (intptr_t)classes;
    message->session = session;
    message->epoch = events->epoch;
    strcpy(message->on, on);
    if (!dvm_messages_send(events->router, 1, 0, &message->message))
    {
        free(message);
        events->resync = 1;
        return 0;
    }
    events->pending++;
    return 1;
}


/* EN/RU: See sysevents.h; partial construction consumes no caller group reference.
 * См. sysevents.h; partial construction не поглощает caller reference. */
DVM_SysEvents *dvm_sysevents_create(DVM_Namespace *group)
{
    DVM_SysEvents *events;
    if (!group)
    {
        return NULL;
    }
    events = calloc(1, sizeof(*events));
    if (!events)
    {
        return NULL;
    }
    events->lock = fvm_port_sync_create();
    events->router = events->lock ? dvm_messages_create() : NULL;
    events->endpoint = events->router ? dvm_messages_register(events->router, 1) : NULL;
    if (!events->endpoint || !dvm_namespace_retain(group))
    {
        if (events->endpoint)
        {
            dvm_messages_stop(events->endpoint);
            dvm_messages_unregister(events->router, events->endpoint);
        }
        if (events->router)
        {
            dvm_messages_destroy(events->router);
        }
        if (events->lock)
        {
            fvm_port_sync_destroy(events->lock);
        }
        free(events);
        return NULL;
    }
    events->group = group;
    events->references = 1;
    events->captured = 0;
    events->epoch = 1;
    events->namespace_tap = dvm_sysevents_subscribe_group(events, group);
    if (!events->namespace_tap)
    {
        dvm_sysevents_release(events);
        return NULL;
    }
    dvm_sysevents_namespace(events, NULL);
    return events;
}


/* EN/RU: See sysevents.h; caller holds a live owner/FD reference.
 * См. sysevents.h; caller держит live owner/FD reference. */
int dvm_sysevents_retain(DVM_SysEvents *events)
{
    int result = 0;
    if (!events)
    {
        return 0;
    }
    fvm_port_sync_lock(events->lock);
    if (events->references != (size_t)-1)
    {
        events->references++;
        result = 1;
    }
    fvm_port_sync_unlock(events->lock);
    return result;
}


/* EN/RU: See sysevents.h; final native cleanup runs after all owner/FD users.
 * См. sysevents.h; final cleanup после всех owner/FD users. */
void dvm_sysevents_release(DVM_SysEvents *events)
{
    int last;
    if (!events)
    {
        return;
    }
    fvm_port_sync_lock(events->lock);
    last = --events->references == 0;
    fvm_port_sync_unlock(events->lock);
    if (last)
    {
        dvm_sysevents_purge(events);
        dvm_messages_stop(events->endpoint);
        if (!dvm_messages_unregister(events->router, events->endpoint))
        {
            abort();
        }
        dvm_messages_destroy(events->router);
        dvm_sysevents_watches_free(events->watches);
        dvm_namespace_release(events->group);
        fvm_port_sync_destroy(events->lock);
        free(events);
    }
}


/* EN/RU: See sysevents.h; queued events remain owned by the retained inbox.
 * См. sysevents.h; queued events остаются у retained inbox. */
void dvm_sysevents_retire(DVM_SysEvents *events)
{
    DVM_NoticeTap *tap;
    DVM_SysWatch *watches;
    if (!events)
    {
        return;
    }
    fvm_port_sync_lock(events->lock);
    if (!events->retired)
    {
        events->exit_pending = 1;
        events->retired = 1;
    }
    tap = events->namespace_tap;
    events->namespace_tap = NULL;
    watches = events->watches;
    events->watches = NULL;
    fvm_port_sync_unlock(events->lock);
    dvm_notice_unsubscribe(tap);
    dvm_sysevents_watches_free(watches);
}


/* EN/RU: See sysevents.h; old scope cannot survive pctl in queued or staged records.
 * См. sysevents.h; old scope не переживает pctl в queue/staged records. */
int dvm_sysevents_group(DVM_SysEvents *events, DVM_Namespace *group)
{
    DVM_Namespace *old;
    DVM_NoticeTap *tap, *previous;
    DVM_SysWatch *watches;
    if (!events || !group || !dvm_namespace_retain(group))
    {
        return 0;
    }
    tap = dvm_sysevents_subscribe_group(events, group);
    if (!tap)
    {
        dvm_namespace_release(group);
        return 0;
    }
    fvm_port_sync_lock(events->lock);
    if (events->epoch == UINT64_MAX || events->retired)
    {
        fvm_port_sync_unlock(events->lock);
        dvm_notice_unsubscribe(tap);
        dvm_namespace_release(group);
        return 0;
    }
    old = events->group;
    previous = events->namespace_tap;
    watches = events->watches;
    events->namespace_tap = tap;
    events->watches = NULL;
    events->group = group;
    events->epoch++;
    events->captured = 0;
    dvm_sysevents_purge(events);
    fvm_port_sync_unlock(events->lock);
    dvm_notice_unsubscribe(previous);
    dvm_sysevents_watches_free(watches);
    dvm_namespace_release(old);
    dvm_sysevents_namespace(events, NULL);
    return 1;
}
