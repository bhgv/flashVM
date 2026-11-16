/* Copyright (c) 2026 bhgv
 * EN: Actual SDK event journal/provider with explicit per-view filtering and fixture-only codec.
 * RU: Настоящий SDK event journal/provider с explicit view filtering и fixture-only codec.
 */
#include "fixture.h"
#include "events_provider.h"
#include "file_provider.h"
#include "subscriptions.h"
#include <stdio.h>
#include <stdlib.h>

static DVM_Changes *journal; /* Owned independently from resource and producer. */
static StyxEvents *events; /* Fixture owner reference through producer join. */
static StyxSubscriptions *subscriptions; /* Optional bounded factory for per-open independent inboxes. */
static unsigned long long generation = 1; /* Single producer's source generation. */
static unsigned released; /* Exactly-once owned policy cleanup. */


/* EN/RU: Fixture check active in every build configuration.
 * Fixture check активна во всех build configurations. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "event resource backend: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Example policy exposes resource42 changes and provider-wide invalidations only.
 * Example policy выдаёт только resource42 changes и provider-wide invalidations. */
static int allow(void *state, const StyxEvent *event)
{
    (void)state;
    return !(event->flags & STYX_EVENT_RESOURCE) || event->resource == 42;
}


/* EN/RU: This text format is fixture data, not a default library or public wire schema.
 * Этот text format — fixture data, не default library/public wire schema. */
static int encode(void *state, const StyxEvent *event, void *bytes, size_t size, size_t *count)
{
    (void)state;
    int n = snprintf(bytes, size, "event %llu %u %u %llu\n", event->generation, event->kinds, event->flags, event->resource);
    if (n <= 0 || (size_t)n >= size)
    {
        return 0;
    }
    *count = n;
    return 1;
}


/* EN/RU: Adopted policy owns only a native marker, never provider/guest state.
 * Adopted policy владеет только native marker, не provider/guest state. */
static void release(void *state)
{
    free(state);
    released++;
}


/* EN/RU: See fixture.h; explicit policy selects shared source or independent per-open queues.
 * См. fixture.h; explicit policy выбирает shared source либо independent per-open queues. */
void eventfixturecreate(int independent)
{
    void *state = malloc(1);
    StyxEventPolicy policy = {DVM_CHANGE_RIGHTS|DVM_CHANGE_CONTENT, 0, 128, allow, encode, state, release};
    journal = dvm_changes_create();
    check(state && journal, "fixture allocation");
    if (independent)
    {
        subscriptions = styxsubscriptions(journal, &policy, 2);
        check(subscriptions != NULL, "independent event factory");
    }
    else
    {
        events = styxeventsource(journal, &policy);
        check(events != NULL, "shared event source");
    }
}


/* EN/RU: See fixture.h; only producer owns generation, all callbacks remain native.
 * См. fixture.h; generation принадлежит producer, callbacks только native. */
void eventfixturepublish(char command)
{
    if (command == 'f')
    {
        styxeventsfinish(events);
        styxsubscriptionsclose(subscriptions);
        return;
    }
    DVM_ChangeEvent event = {1, ++generation, command == 'l' ? DVM_CHANGE_LOAD : DVM_CHANGE_CONTENT,
                             command == 'l' ? 0 : DVM_CHANGE_RESOURCE, command == 'h' ? 7 : 42};
    check(dvm_changes_notify(journal, &event) == 1, "journal publication");
}


/* EN/RU: See fixture.h; actual producer/client lifetime fences precede cleanup.
 * См. fixture.h; producer/client lifetime fences предшествуют cleanup. */
void eventfixturefree(void)
{
    styxeventsfree(events);
    styxsubscriptionsfree(subscriptions);
    dvm_changes_destroy(journal);
    check(released == 1, "owned policy lifetime");
}


/* EN/RU: One native fid owns its ordinary open and independent readiness/source pin.
 * Один native fid владеет ordinary open и readiness/source pin. */
typedef struct Open
{
    StyxOpen *file; /* Owned ordinary provider-backed open. */
    StyxEvents *events; /* Owned inbox pin for borrowed readiness fd. */
} Open;


/* EN/RU: See fixture.h; independent factory allocates a queue for each successful readonly open.
 * См. fixture.h; independent factory создаёт очередь для каждого successful readonly open. */
void *eventfixtureopen(int mode)
{
    if (mode != 0)
    {
        return NULL;
    }
    Open *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return NULL;
    }
    opened->events = subscriptions ? styxsubscribe(subscriptions) :
        styxeventsretain(events) ? events : NULL;
    if (!opened->events || !styxeventsretain(opened->events))
    {
        eventfixtureclose(opened);
        return NULL;
    }
    DVM_Provider *provider = styxeventprovider(opened->events);
    StyxBacking *backing = styxbacking(provider);
    opened->file = styxfileopen(backing, mode);
    styxbackingfree(backing);
    dvm_provider_release(provider);
    if (!opened->file)
    {
        eventfixtureclose(opened);
        return NULL;
    }
    return opened;
}


/* EN/RU: See fixture.h; descriptor belongs to retained source, not the socket owner.
 * См. fixture.h; descriptor принадлежит удержанному source, не socket owner. */
int eventfixtureopenfd(void *context)
{
    Open *opened = context;
    return opened ? styxeventsfd(opened->events) : -1;
}


/* EN/RU: See fixture.h; actual SDK file result retains record/AGAIN/EOF distinctions.
 * См. fixture.h; SDK file result сохраняет различие record/AGAIN/EOF. */
void eventfixtureread(void *context, unsigned long long offset, void *bytes, size_t size, StyxTransfer *result)
{
    Open *opened = context;
    styxfileread(opened ? opened->file : NULL, offset, bytes, size, result);
}


/* EN/RU: See fixture.h; callback retirement precedes source fd/context release.
 * См. fixture.h; callback retirement предшествует освобождению source fd/context. */
void eventfixtureclose(void *context)
{
    Open *opened = context;
    if (opened)
    {
        styxfileclose(opened->file);
        styxeventsfree(opened->events);
        free(opened);
    }
}
