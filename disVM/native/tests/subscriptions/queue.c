/* Copyright (c) 2026 bhgv
 * EN: Independent subscription queues, quota, per-reader cache and shared policy lifetime.
 * RU: Независимые очереди подписок, quota, per-reader cache и lifetime общей policy.
 */
#include "subscriptions.h"
#include <poll.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: External fixture state stays alive through final shared-policy release.
 * External fixture state живёт до final shared-policy release. */
typedef struct State
{
    unsigned releases; /* Exactly-once final policy cleanup. */
    unsigned encodes; /* Independent cache formatter invocation count. */
    int allowed; /* Dynamic native delivery permission. */
} State;


/* EN/RU: Always-active native fixture check, with a concrete failure diagnostic.
 * Постоянная native fixture проверка с конкретной диагностикой failure. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "independent subscriptions: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Explicit native filter rechecked for each stream's cached head.
 * Явный native filter, перепроверяемый для cached head каждого stream. */
static int allow(void *opaque, const StyxEvent *event)
{
    (void)event;
    return ((State *)opaque)->allowed;
}


/* EN/RU: Native scalar copy is fixture-only; count each distinct formatting attempt.
 * Native scalar copy только fixture; считать отдельные formatting attempts. */
static int encode(void *opaque, const StyxEvent *event, void *bytes, size_t size, size_t *count)
{
    State *state = opaque;
    state->encodes++;
    if (size < sizeof(*event))
    {
        return 0;
    }
    memcpy(bytes, event, sizeof(*event));
    *count = sizeof(*event);
    return 1;
}


/* EN/RU: Count final shared context retirement without freeing external fixture storage.
 * Считать final shared context retirement без освобождения external fixture storage. */
static void release(void *opaque)
{
    ((State *)opaque)->releases++;
}


/* EN/RU: Read exactly one complete fixture record; failure remains visible in optimized builds.
 * Прочитать ровно одну полную fixture запись; failure виден и в optimized builds. */
static StyxEvent record(StyxEvents *events)
{
    StyxEvent event;
    size_t count = 0;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_RECORD && count == sizeof(event), "one record");
    return event;
}


/* EN/RU: Test-only readiness probe never drains the inbox's borrowed descriptor.
 * Test-only readiness probe не читает borrowed descriptor inbox. */
static int ready(StyxEvents *events)
{
    struct pollfd fd = {styxeventsfd(events), POLLIN, 0};
    check(poll(&fd, 1, 0) >= 0, "readiness poll");
    return (fd.revents & POLLIN) != 0;
}


/* EN/RU: Publish one scoped content change with an explicitly increasing source generation.
 * Опубликовать scoped content change с явно возрастающей source generation. */
static void publish(DVM_Changes *source, unsigned long long generation)
{
    DVM_ChangeEvent event = {1, generation, STYX_EVENT_CONTENT, STYX_EVENT_RESOURCE, 42};
    check(dvm_changes_notify(source, &event) == 1, "journal publication");
}


/* EN/RU: Check independent head/cache, permission recheck and readiness on two streams.
 * Проверить independent head/cache, permission recheck и readiness двух streams. */
static void cache(DVM_Changes *source, StyxEvents *a, StyxEvents *b, State *state)
{
    StyxEvent event;
    size_t count = 99;
    publish(source, 2);
    check(styxeventsread(a, &event, 1, &count) == STYX_EVENT_SMALL && !count, "short A read");
    check(styxeventsread(b, &event, 1, &count) == STYX_EVENT_SMALL && state->encodes == 4, "distinct formatting caches");
    check(record(a).generation == 2 && !ready(a) && ready(b), "A consumed B head/readiness");
    state->allowed = 0;
    check(styxeventsread(b, &event, sizeof(event), &count) == STYX_EVENT_EMPTY && !count && !ready(b), "B cached head bypassed revocation");
    state->allowed = 1;
    publish(source, 3);
    check(record(a).generation == 3 && record(b).generation == 3, "independent delivery after revocation");
}


/* EN/RU: A slow subscriber overflows only its own bounded tail; fast stream keeps exact scopes.
 * Медленный subscriber переполняет только свой bounded tail; fast stream сохраняет scope. */
static void overflow(DVM_Changes *source, StyxEvents *a, StyxEvents *b)
{
    for (unsigned long long generation = 2; generation <= 45; generation++)
    {
        publish(source, generation);
        StyxEvent event = record(a);
        check(event.generation == generation && event.kinds == STYX_EVENT_CONTENT &&
              event.flags == STYX_EVENT_RESOURCE && event.resource == 42, "fast subscriber lost precise records");
    }
    StyxEvent event = record(b);
    check(event.kinds == (STYX_EVENT_CONTENT|STYX_EVENT_RIGHTS) && !event.flags, "slow subscriber missing overflow refresh");
    unsigned long long last = event.generation;
    unsigned count = 1;
    while (ready(b))
    {
        event = record(b);
        check(event.generation > last && ++count <= 33, "slow queue is unbounded/unordered");
        last = event.generation;
    }
    check(last == 45 && count < 44, "slow queue failed to retain latest change");
}


/* EN/RU: Exercise public factory contracts against actual journal and event-source code.
 * Проверить public factory contracts на настоящем journal и event-source code. */
int main(int argc, char **argv)
{
    check(argc == 2, "scenario");
    State state = {0, 0, 1};
    DVM_Changes *source = dvm_changes_create();
    StyxEventPolicy policy = {STYX_EVENT_CONTENT|STYX_EVENT_RIGHTS, 1, sizeof(StyxEvent), allow, encode, &state, release};
    check(source && !styxsubscriptions(source, &policy, 0) && !styxsubscriptions(source, &policy, 1025) &&
          !styxsubscriptions(NULL, &policy, 2) && !state.releases, "invalid setup adopted context");
    StyxSubscriptions *factory = styxsubscriptions(source, &policy, 2);
    check(factory && styxsubscriptionsretain(factory), "factory ownership");
    styxsubscriptionsfree(factory);
    if (!strcmp(argv[1], "empty"))
    {
        StyxSubscriptionInfo empty;
        check(styxsubscriptionsinfo(factory, &empty) && !empty.active && !state.releases, "empty factory status");
        styxsubscriptionsclose(factory);
        styxsubscriptionsclose(factory);
        check(!styxsubscribe(factory), "empty closed factory admitted source");
        styxsubscriptionsfree(factory);
        check(state.releases == 1, "empty factory policy lifetime");
        dvm_changes_destroy(source);
        puts("PASS independent subscription queue: empty");
        return 0;
    }
    if (!strcmp(argv[1], "rollback"))
    {
        struct rlimit saved;
        check(!getrlimit(RLIMIT_NOFILE, &saved), "save fd quota");
        struct rlimit blocked = saved;
        blocked.rlim_cur = 0;
        check(!setrlimit(RLIMIT_NOFILE, &blocked), "block descriptor allocation");
        StyxEvents *failed = styxsubscribe(factory);
        check(!setrlimit(RLIMIT_NOFILE, &saved), "restore fd quota");
        StyxSubscriptionInfo rollback;
        check(!failed && styxsubscriptionsinfo(factory, &rollback) && !rollback.active &&
              !state.releases, "failed source allocation leaked slot/adopted policy");
    }
    StyxEvents *a = styxsubscribe(factory);
    StyxEvents *b = styxsubscribe(factory);
    StyxSubscriptionInfo info;
    check(a && b && styxeventsfd(a) != styxeventsfd(b) && !styxsubscribe(factory) &&
          styxsubscriptionsinfo(factory, &info) && info.active == 2 && info.limit == 2 && !info.closed, "quota/independent descriptors");
    check(record(a).generation == 1 && record(b).generation == 1, "independent initial snapshots");
    if (!strcmp(argv[1], "cache"))
    {
        cache(source, a, b, &state);
    }
    else if (!strcmp(argv[1], "overflow"))
    {
        overflow(source, a, b);
    }
    else if (!strcmp(argv[1], "quota") || !strcmp(argv[1], "rollback"))
    {
        styxeventsfinish(a);
        publish(source, 2);
        check(record(b).generation == 2, "finishing A stopped subscriber B");
        check(!styxsubscribe(factory), "finished but owned inbox released quota early");
        styxeventsfree(a);
        a = styxsubscribe(factory);
        check(a && record(a).generation == 2, "slot not reusable after source retirement");
    }
    else if (!strcmp(argv[1], "close"))
    {
        styxsubscriptionsclose(factory);
        check(!styxsubscribe(factory) && styxsubscriptionsinfo(factory, &info) && info.closed, "closed admission reopened");
        publish(source, 2);
        check(record(a).generation == 2 && record(b).generation == 2, "factory close stopped existing subscribers");
    }
    else
    {
        check(0, "unknown scenario");
    }
    styxeventsfinish(a);
    StyxEvent event;
    size_t count;
    check(styxeventsread(a, &event, sizeof(event), &count) == STYX_EVENT_EOF, "independent finish");
    styxeventsfree(a);
    styxsubscriptionsfree(factory);
    check(!state.releases, "factory owner loss freed live subscriber policy");
    dvm_changes_destroy(source);
    styxeventsfree(b);
    check(state.releases == 1, "final shared policy lifetime");
    printf("PASS independent subscription queue: %s\n", argv[1]);
    return 0;
}
