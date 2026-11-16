/* Copyright (c) 2026 bhgv
 * EN: Actual SDK event journal/provider with explicit per-view filtering and fixture-only codec.
 * RU: Настоящий SDK event journal/provider с explicit view filtering и fixture-only codec.
 */
#include "fixture.h"
#include "events_provider.h"
#include "file_provider.h"
#include <stdio.h>
#include <stdlib.h>

static DVM_Changes *journal; /* Owned independently from resource and producer. */
static StyxEvents *events; /* Fixture owner reference through producer join. */
static DVM_Provider *provider; /* Ordinary readonly factory owns another inbox reference. */
static StyxBacking *backing; /* Owned prepared file capability through native client teardown. */
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


/* EN/RU: See fixture.h; explicit policy is selected before publishing the ordinary backing.
 * См. fixture.h; explicit policy выбирается до публикации ordinary backing. */
StyxBacking *eventfixturecreate(void)
{
    void *state = malloc(1);
    StyxEventPolicy policy = {DVM_CHANGE_RIGHTS|DVM_CHANGE_CONTENT, 0, 128, allow, encode, state, release};
    journal = dvm_changes_create();
    check(state && journal, "fixture allocation");
    events = styxeventsource(journal, &policy);
    check(events && styxeventsretain(events), "event source");
    provider = styxeventprovider(events);
    backing = styxbacking(provider);
    check(backing != NULL, "ordinary event backing");
    return backing;
}


/* EN/RU: See fixture.h; caller holds the fixture's owner reference.
 * См. fixture.h; caller удерживает fixture owner reference. */
int eventfixturefd(void)
{
    return styxeventsfd(events);
}


/* EN/RU: See fixture.h; only producer owns generation, all callbacks remain native.
 * См. fixture.h; generation принадлежит producer, callbacks только native. */
void eventfixturepublish(char command)
{
    if (command == 'f')
    {
        styxeventsfinish(events);
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
    styxbackingfree(backing);
    dvm_provider_release(provider);
    styxeventsfree(events);
    dvm_changes_destroy(journal);
    check(released == 1, "owned policy lifetime");
}
