/* Copyright (c) 2026 bhgv
 * EN: Real event-resource queue, filtering, encoded head, ordinary provider and readiness tests.
 * RU: Проверки event-resource queue, filtering, encoded head, обычного provider и readiness.
 */
#include "events_provider.h"
#include "file_provider.h"
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* EN/RU: External native fixture state survives all adopted resource references.
 * External native fixture state живёт дольше всех adopted resource references. */
typedef struct State
{
    unsigned long long allowed; /* Dynamically admitted private resource identity,0 denies all. */
    unsigned releases; /* Exactly-once policy cleanup. */
    unsigned encodes; /* Cached-head formatter invocation count. */
    int invalid; /* Invalid negative filter decision must fail closed. */
    int fail; /* Deliberate formatter refusal. */
    int badlength; /* Deliberate malformed formatter acknowledgement. */
} State;


/* EN/RU: Checks remain active in optimized builds.
 * Проверки остаются активными в optimized builds. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "event resource queue: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Admit only this prepared view's resource42 or provider-wide invalidations.
 * Допустить только resource42 выданного view либо provider-wide invalidations. */
static int allow(void *opaque, const StyxEvent *event)
{
    State *state = opaque;
    if (state->invalid)
    {
        return -1;
    }
    return state->allowed && (!(event->flags & STYX_EVENT_RESOURCE) || event->resource == state->allowed);
}


/* EN/RU: Native struct copy is fixture data only, not a public/wire event encoding.
 * Копия native struct — только fixture data, не public/wire формат событий. */
static int encode(void *opaque, const StyxEvent *event, void *bytes, size_t size, size_t *count)
{
    State *state = opaque;
    state->encodes++;
    if (state->fail == 1 || size < sizeof(*event))
    {
        return 0;
    }
    if (state->badlength)
    {
        *count = size+1;
        return 1;
    }
    memcpy(bytes, event, sizeof(*event));
    *count = sizeof(*event);
    return state->fail == 2 ? -1 : 1;
}


/* EN/RU: Count final policy cleanup without owning external fixture storage.
 * Считать final policy cleanup, не владея external fixture storage. */
static void release(void *opaque)
{
    ((State *)opaque)->releases++;
}


/* EN/RU: Observe fd readiness without draining/consuming the resource's notification pipe.
 * Проверить fd readiness без drain/consumption notification pipe ресурса. */
static int ready(StyxEvents *events)
{
    struct pollfd fd = {styxeventsfd(events), POLLIN, 0};
    check(poll(&fd, 1, 0) >= 0, "readiness poll");
    return (fd.revents & POLLIN) != 0;
}


/* EN/RU: Publish one accepted scalar source event, incrementing its generation exactly once.
 * Опубликовать один scalar source event с одним увеличением generation. */
static void publish(DVM_Changes *source, unsigned long long *generation, unsigned kinds, unsigned flags, unsigned long long id)
{
    DVM_ChangeEvent event = {1, ++*generation, kinds, flags, id};
    check(dvm_changes_notify(source, &event) == 1, "source publication");
}


/* EN/RU: Read one complete copied record through the scalar API.
 * Прочитать одну полную copied запись через scalar API. */
static StyxEvent record(StyxEvents *events)
{
    StyxEvent event;
    size_t count;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_RECORD && count == sizeof(event), "one full record");
    return event;
}


/* EN/RU: Cached records retain private scope for delivery-time access recheck before ID masking.
 * Cached records сохраняют private scope для delivery-time access recheck до ID masking. */
static void revocation(void)
{
    State state = {0};
    state.allowed = 42;
    DVM_Changes *source = dvm_changes_create();
    StyxEventPolicy policy = {DVM_CHANGE_CONTENT, 0, sizeof(StyxEvent), allow, encode, &state, release};
    StyxEvents *events = styxeventsource(source, &policy);
    check(events && record(events).generation == 1, "revocation source snapshot");
    unsigned long long generation = 1;
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42);
    size_t count;
    unsigned char small;
    check(styxeventsread(events, &small, 1, &count) == STYX_EVENT_SMALL, "revocation cached head");
    state.invalid = 1;
    StyxEvent event;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_EMPTY && !count, "negative policy decision admitted");
    state.invalid = 0;
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42);
    check(styxeventsread(events, &small, 1, &count) == STYX_EVENT_SMALL, "dynamic policy cached head");
    state.allowed = 7;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_EMPTY && !count && !ready(events), "revoked cached metadata leaked");
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 7);
    event = record(events);
    check(event.generation == 4 && !event.flags && !event.resource, "private resource identity exposed");
    styxeventsfree(events);
    check(state.releases == 1, "revocation policy cleanup");
    dvm_changes_destroy(source);
}


/* EN/RU: Test successful adoption, rejected input, filtering, recovery and shared provider heads.
 * Проверить adoption, invalid input, filtering, recovery и shared provider heads. */
int main(void)
{
    State state = {0};
    state.allowed = 42;
    DVM_Changes *source = dvm_changes_create();
    StyxEventPolicy policy = {DVM_CHANGE_RIGHTS|DVM_CHANGE_CONTENT, 1, sizeof(StyxEvent), allow, encode, &state, release};
    check(source != NULL, "journal");
    StyxEventPolicy invalid = policy;
    invalid.encode = NULL;
    check(!styxeventsource(source, &invalid) && !state.releases, "invalid policy adopted");
    check(!styxeventsource(NULL, &policy) && !styxeventsretain(NULL) && styxeventsfd(NULL) == -1, "invalid inputs");
    StyxEvents *events = styxeventsource(source, &policy);
    check(events && ready(events), "initial race-closing snapshot/readiness");
    check((fcntl(styxeventsfd(events), F_GETFD) & FD_CLOEXEC) &&
          (fcntl(styxeventsfd(events), F_GETFL) & O_NONBLOCK), "readiness flags");
    size_t count = 99;
    unsigned char small;
    check(styxeventsread(events, &small, 1, &count) == STYX_EVENT_SMALL && !count && state.encodes == 1 && ready(events), "short buffer consumed snapshot");
    unsigned long long generation = 1;
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42);
    check(record(events).generation == 1 && state.encodes == 1, "encoded head changed/reformatted");
    StyxEvent event = record(events);
    check(event.generation == 2 && event.resource == 42 && event.flags == STYX_EVENT_RESOURCE, "projected resource scope");
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_EMPTY && !count && !ready(events), "empty live source became EOF/ready");
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 7);
    publish(source, &generation, DVM_CHANGE_LOAD, 0, 0);
    check(!ready(events), "hidden class/resource published");
    publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42);
    event = record(events);
    check(event.generation == 5 && event.kinds == policy.kinds && !event.flags && !event.resource, "gap did not broaden authorized view");
    publish(source, &generation, DVM_CHANGE_CONTENT, 0, 0);
    state.fail = 1;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_ERROR && !count && ready(events), "formatter refusal lost head");
    state.fail = 0;
    state.badlength = 1;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_ERROR && !count, "bad formatter length accepted");
    state.badlength = 0;
    state.fail = 2;
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_ERROR && !count, "negative formatter result admitted");
    state.fail = 0;
    check(record(events).generation == 6, "formatter recovery lost head");
    check(styxeventsretain(events), "factory ref");
    DVM_Provider *provider = styxeventprovider(events);
    DVM_VfsHandle opened = {0};
    check(provider && dvm_provider_bindable(provider), "stable provider");
    check(dvm_provider_open(provider, ".", "rw", &opened) != FVM_OK && !opened.ops, "write authority widened");
    check(dvm_provider_open(provider, "hidden", "r", &opened) != FVM_OK, "unexpected path");
    check(dvm_provider_open(provider, ".", "r", &opened) == FVM_OK, "ordinary event open");
    DVM_IoResult result = dvm_vfs_read_chunk(&opened, &event, sizeof(event));
    check(result.status != FVM_OK && result.stop == DVM_IO_AGAIN && !result.count && !(result.flags & DVM_IO_UNCERTAIN), "provider AGAIN semantics");
    publish(source, &generation, DVM_CHANGE_RIGHTS, 0, 0);
    result = dvm_vfs_read_chunk(&opened, &small, 1);
    check(result.status != FVM_OK && result.stop != DVM_IO_EOF, "provider short buffer became EOF");
    dvm_vfs_close(&opened);
    check(dvm_provider_open(provider, ".", "r", &opened) == FVM_OK, "reopen");
    result = dvm_vfs_read_chunk(&opened, &event, sizeof(event));
    check(result.status == FVM_OK && (result.flags & DVM_IO_RECORD) && event.generation == 7, "close/reopen lost shared head");
    dvm_vfs_close(&opened);
    StyxBacking *backing = styxbacking(provider);
    StyxOpen *file = styxfileopen(backing, 0);
    StyxTransfer transfer;
    styxfileread(file, 0, &event, sizeof(event), &transfer);
    check(file && transfer.again && !transfer.success && !transfer.count, "scalar bridge lost AGAIN");
    styxfileclose(file);
    styxbackingfree(backing);
    dvm_provider_release(provider);
    publish(source, &generation, DVM_CHANGE_CONTENT, 0, 0);
    check(styxeventsread(events, &small, 1, &count) == STYX_EVENT_SMALL, "pinned overflow head");
    unsigned long long pinned = generation;
    for (unsigned i = 0; i < 40; i++)
    {
        publish(source, &generation, DVM_CHANGE_CONTENT, DVM_CHANGE_RESOURCE, 42);
    }
    check(record(events).generation == pinned, "overflow replaced pinned head");
    unsigned records = 0, broad = 0;
    while (styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_RECORD)
    {
        records++;
        broad += event.kinds == policy.kinds && !event.flags;
    }
    check(records <= 32 && broad && !ready(events), "overflow lost conservative resync/bound");
    publish(source, &generation, DVM_CHANGE_RIGHTS, 0, 0);
    styxeventsfinish(events);
    check(record(events).generation == generation, "finish discarded queued record");
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_EOF && !count && ready(events), "drained finish lacks EOF readiness");
    publish(source, &generation, DVM_CHANGE_RIGHTS, 0, 0);
    check(styxeventsread(events, &event, sizeof(event), &count) == STYX_EVENT_EOF, "finished subscription revived");
    int descriptor = styxeventsfd(events);
    styxeventsfree(events);
    check(fcntl(descriptor, F_GETFD) < 0 && errno == EBADF, "final readiness descriptor leak");
    check(state.releases == 1, "policy release count");
    dvm_changes_destroy(source);
    revocation();
    puts("PASS native event resource queue/provider/readiness");
    return 0;
}
