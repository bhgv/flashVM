/* Independent behavioral checks: success silence, precise origins, lifetime and paused-owner liveness. */
#include "broker.h"
#include "codec.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef DVM_BROKER_SANDBOX_LSAN
/* The restricted host has no /proc task inspection; address/undefined checks stay enabled. */
int __lsan_is_turned_off(void)
{
    return 1;
}
#endif


/* All fixture failures stay active in Release/NDEBUG and preserve diagnostics. */
static void require(int condition, const char *message)
{
    if (!condition)
    {
        fprintf(stderr, "FAIL broker: %s\n", message);
        exit(1);
    }
}


/* Lock only broker metadata; a separate paused application mutex is never taken here. */
static void enter(void *mutex)
{
    require(pthread_mutex_lock(mutex) == 0, "metadata lock");
}


/* Release the caller-owned native broker mutex. */
static void leave(void *mutex)
{
    require(pthread_mutex_unlock(mutex) == 0, "metadata unlock");
}


/* Register an exact occurrence of a repeated variable name in one source expression. */
static uint32_t site(DVM_Broker *broker, uint32_t column)
{
    DVM_BrokerSite value;
    uint32_t id = 0;
    memset(&value, 0, sizeof(value));
    strcpy(value.build, "build-a");
    strcpy(value.path, "source/пример.b");
    strcpy(value.module, "Test.value");
    value.line = 7;
    value.column = column;
    value.offset = 100 + column - 1;
    value.length = 1;
    value.binding = 9;
    require(dvm_broker_site(broker, &value, &id) == DVM_BROKER_OK, "register exact site");
    return id;
}


/* Open copied external metadata; reconnects must preserve this original source location. */
static uint32_t open_channel(DVM_Broker *broker, uint32_t origin, uint64_t heartbeat)
{
    DVM_BrokerChannel card;
    uint32_t id = 0;
    memset(&card, 0, sizeof(card));
    card.transport = DVM_BROKER_NETWORK;
    strcpy(card.logical, "ai.primary");
    strcpy(card.local, "tcp://127.0.0.1:9000");
    strcpy(card.peer, "tcp://127.0.0.1:9001");
    card.created_at = origin;
    card.connected_at = origin;
    card.heartbeat_ms = heartbeat;
    require(dvm_broker_open(broker, &card, 0, &id) == DVM_BROKER_OK, "open copied channel");
    return id;
}


/* Admit a request without an implicit successful diagnostic; generation is supplied explicitly. */
static DVM_BrokerKey
begin(DVM_Broker *broker, uint32_t channel, uint32_t generation, uint32_t origin, uint64_t now, uint64_t deadline)
{
    DVM_BrokerSnapshot value;
    DVM_BrokerKey key;
    memset(&value, 0, sizeof(value));
    value.key.channel = channel;
    value.key.generation = generation;
    value.site = origin;
    value.object = 41;
    value.warn_ms = 10;
    value.deadline_ms = deadline;
    strcpy(value.operation, "ReadAs(Number)");
    require(dvm_broker_begin(broker, &value, now, &key) == DVM_BROKER_OK, "admit request");
    return key;
}


/* Consume one expected deviation from a private listener queue and reject silent overflow. */
static DVM_BrokerEvent event(DVM_Broker *broker, uint32_t listener, DVM_BrokerEventKind kind)
{
    DVM_BrokerEvent value;
    uint64_t dropped = 999;
    require(dvm_broker_poll(broker, listener, &value, &dropped) == DVM_BROKER_OK && !dropped && value.kind == kind,
            "expected deviation class");
    return value;
}


/* Empty polling must preserve the event output and emit no successful operation traffic. */
static void silent(DVM_Broker *broker, uint32_t listener)
{
    DVM_BrokerEvent value;
    DVM_BrokerEvent sentinel;
    uint64_t dropped = 999;
    memset(&value, 0xa5, sizeof(value));
    sentinel = value;
    require(dvm_broker_poll(broker, listener, &value, &dropped) == DVM_BROKER_EMPTY && !dropped &&
                !memcmp(&value, &sentinel, sizeof(value)),
            "success silence and empty output preservation");
}


/* Reentrant diagnostics run on another native thread while the application owner is stopped. */
typedef struct
{
    DVM_Broker *broker; /* Borrowed broker; main joins before destruction. */
    DVM_BrokerKey key;  /* Paused application's request. */
    int observed;       /* Written by worker, read after pthread_join synchronization. */
} Probe;


/* Query copied state repeatedly without consulting the blocked application.
 * argument is a borrowed Probe; returns no owner or pointer result. */
static void *diagnostic(void *argument)
{
    Probe *probe = argument;
    DVM_BrokerSnapshot snapshot;
    int index;
    for (index = 0; index < 1000; index++)
    {
        require(dvm_broker_query(probe->broker, probe->key, &snapshot) == DVM_BROKER_OK &&
                    snapshot.phase == DVM_BROKER_PAUSED,
                "independent diagnostic worker");
    }
    probe->observed = 1;
    return NULL;
}


/* Test deviation-only fanout, repeated-name source spans, session generations and deadline ownership. */
static void behavior(void)
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_t stopped_application = PTHREAD_MUTEX_INITIALIZER;
    DVM_BrokerSync sync = {enter, leave, &mutex};
    DVM_Broker *broker = NULL;
    uint32_t first;
    uint32_t second;
    uint32_t channel;
    uint32_t global;
    uint32_t filtered;
    DVM_BrokerKey fast;
    DVM_BrokerKey slow;
    DVM_BrokerKey resumed;
    DVM_BrokerSnapshot snapshot;
    DVM_BrokerChannel card;
    DVM_BrokerEvent message;
    pthread_t thread;
    Probe probe;
    require(dvm_broker_create(&sync, &broker) == DVM_BROKER_OK, "create broker");
    first = site(broker, 1);
    second = site(broker, 17);
    require(first != second, "same binding has different occurrence sites");
    require(dvm_broker_watch(broker, 0, 0, &global) == DVM_BROKER_OK, "global listener without IDs");
    channel = open_channel(broker, first, 100);
    fast = begin(broker, channel, 1, first, 0, 0);
    require(dvm_broker_heartbeat(broker, channel, 1, 1) == DVM_BROKER_OK, "heartbeat accepted quietly");
    require(dvm_broker_finish(broker, fast, 0, "", 2) == DVM_BROKER_OK, "ordinary success");
    require(dvm_broker_forget(broker, fast) == DVM_BROKER_OK, "retire completed record");
    silent(broker, global);
    slow = begin(broker, channel, 1, second, 3, 30);
    require(dvm_broker_watch(broker, channel, slow.request, &filtered) == DVM_BROKER_OK, "request listener");
    require(dvm_broker_progress(broker, slow, DVM_BROKER_NETWORK_IO, 1, "waiting for upstream", 4) == DVM_BROKER_OK,
            "owner publishes reason before blocking");
    require(dvm_broker_tick(broker, 12) == DVM_BROKER_OK, "before warning threshold");
    silent(broker, global);
    require(dvm_broker_tick(broker, 13) == DVM_BROKER_OK, "at warning threshold");
    message = event(broker, global, DVM_BROKER_SLOW);
    require(message.origin.column == 17 && message.origin.binding == 9 && message.creation.column == 1 &&
                message.request.key.request == slow.request && message.request.updated_ms == 4 &&
                message.request.phase == DVM_BROKER_NETWORK_IO,
            "exact occurrence, creation and reason freshness");
    message = event(broker, filtered, DVM_BROKER_SLOW);
    require(message.origin.offset == 116 && !strcmp(message.request.channel.peer, "tcp://127.0.0.1:9001"),
            "independent listener has copied endpoint/source");
    require(dvm_broker_tick(broker, 14) == DVM_BROKER_OK, "repeated ordinary tick");
    silent(broker, global);
    require(dvm_broker_progress(broker, slow, DVM_BROKER_PAUSED, 1, "breakpoint", 15) == DVM_BROKER_OK,
            "debug pause is distinct from dead broker");
    event(broker, global, DVM_BROKER_DEBUG_PAUSED);
    event(broker, filtered, DVM_BROKER_DEBUG_PAUSED);
    probe.broker = broker;
    probe.key = slow;
    probe.observed = 0;
    require(pthread_mutex_lock(&stopped_application) == 0, "simulate stopped application");
    require(pthread_create(&thread, NULL, diagnostic, &probe) == 0, "spawn independent diagnostic task");
    require(pthread_join(thread, NULL) == 0 && probe.observed,
            "diagnostics complete while application remains stopped");
    require(pthread_mutex_unlock(&stopped_application) == 0, "resume fixture application");
    require(dvm_broker_tick(broker, 30) == DVM_BROKER_OK, "caller deadline independent of owner pause");
    event(broker, global, DVM_BROKER_TIMED_OUT);
    event(broker, filtered, DVM_BROKER_TIMED_OUT);
    require(dvm_broker_forget(broker, slow) == DVM_BROKER_BUSY, "timeout is not execution cancellation/root release");
    require(dvm_broker_finish(broker, slow, 0, "late completion", 31) == DVM_BROKER_STALE,
            "late result records completion but cannot satisfy an expired caller");
    require(dvm_broker_query(broker, slow, &snapshot) == DVM_BROKER_OK && snapshot.finished && snapshot.timed_out,
            "late completion remains explicitly queryable");
    silent(broker, global);
    require(dvm_broker_forget(broker, slow) == DVM_BROKER_OK, "retire after actual completion");
    resumed = begin(broker, channel, 1, second, 32, 0);
    require(dvm_broker_close(broker, channel, 1, 33) == DVM_BROKER_OK, "revoke old session");
    event(broker, global, DVM_BROKER_DISCONNECTED);
    message = event(broker, global, DVM_BROKER_DISCONNECTED);
    require(message.request.key.request == resumed.request, "session loss diagnoses active source occurrence");
    require(dvm_broker_reconnect(broker, channel, "ipc://local", "tcp://127.0.0.1:9002", second, 34) == DVM_BROKER_OK,
            "new endpoint/generation");
    require(dvm_broker_heartbeat(broker, channel, 1, 35) == DVM_BROKER_STALE,
            "old heartbeat cannot revive old session");
    require(dvm_broker_query(broker, resumed, &snapshot) == DVM_BROKER_OK && snapshot.disconnected &&
                snapshot.channel.generation == 1 && !strcmp(snapshot.channel.peer, "tcp://127.0.0.1:9001"),
            "old request preserves original endpoint");
    require(dvm_broker_channel(broker, channel, &card) == DVM_BROKER_OK && card.generation == 2 &&
                card.created_at == first && card.connected_at == second && !strcmp(card.peer, "tcp://127.0.0.1:9002"),
            "current connection and immutable creation sites");
    require(dvm_broker_finish(broker, resumed, 0, "", 35) == DVM_BROKER_STALE,
            "old session cannot receive a late success");
    require(dvm_broker_forget(broker, resumed) == DVM_BROKER_OK, "retire acknowledged revoked work");
    require(dvm_broker_tick(broker, 134) == DVM_BROKER_OK, "heartbeat expiry");
    event(broker, global, DVM_BROKER_DISCONNECTED);
    silent(broker, global);
    require(dvm_broker_unwatch(broker, filtered) == DVM_BROKER_OK &&
                dvm_broker_unwatch(broker, filtered) == DVM_BROKER_STALE,
            "listener generation not resurrected");
    dvm_broker_destroy(broker);
    require(pthread_mutex_destroy(&mutex) == 0 && pthread_mutex_destroy(&stopped_application) == 0, "quiesced cleanup");
}


/* Exercise queue pressure, coalescing and request retention limits without fabricated success events. */
static void bounds(void)
{
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    DVM_BrokerSync sync = {enter, leave, &mutex};
    DVM_Broker *broker = NULL;
    uint32_t origin;
    uint32_t channel;
    uint32_t listener;
    DVM_BrokerKey keys[DVM_BROKER_REQUESTS];
    DVM_BrokerSnapshot input;
    DVM_BrokerKey unchanged = {999, 999, 999};
    DVM_BrokerKey rejected = unchanged;
    DVM_BrokerEvent message;
    uint64_t dropped;
    size_t index;
    require(dvm_broker_create(&sync, &broker) == DVM_BROKER_OK, "create bounded broker");
    origin = site(broker, 1);
    channel = open_channel(broker, origin, 0);
    require(dvm_broker_watch(broker, 0, 0, &listener) == DVM_BROKER_OK, "bounded global listener");
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        keys[index] = begin(broker, channel, 1, origin, 0, 0);
    }
    memset(&input, 0, sizeof(input));
    input.key.channel = channel;
    input.key.generation = 1;
    input.site = origin;
    strcpy(input.operation, "Get");
    require(dvm_broker_begin(broker, &input, 0, &rejected) == DVM_BROKER_LIMIT &&
                !memcmp(&rejected, &unchanged, sizeof(rejected)),
            "full registry preserves caller output");
    require(dvm_broker_tick(broker, 10) == DVM_BROKER_OK, "overflow deviations, never block");
    require(dvm_broker_poll(broker, listener, &message, &dropped) == DVM_BROKER_OK &&
                dropped == DVM_BROKER_REQUESTS - DVM_BROKER_EVENTS,
            "observable bounded overflow count");
    require(dvm_broker_progress(broker, keys[1], DVM_BROKER_DISK, 1, "disk", 11) == DVM_BROKER_OK,
            "coalesce already queued diagnostic");
    for (index = 0; index < DVM_BROKER_EVENTS - 1; index++)
    {
        require(dvm_broker_poll(broker, listener, &message, &dropped) == DVM_BROKER_OK && !dropped,
                "coalescing adds no duplicate or extra overflow");
        if (!index)
        {
            require(message.request.phase == DVM_BROKER_DISK, "coalesced event has latest reason");
        }
    }
    silent(broker, listener);
    require(dvm_broker_tick(broker, 9) == DVM_BROKER_INVALID, "reject backward time");
    for (index = 0; index < DVM_BROKER_REQUESTS; index++)
    {
        require(dvm_broker_finish(broker, keys[index], 0, "", 12) == DVM_BROKER_OK, "complete pressure fixture");
        require(dvm_broker_forget(broker, keys[index]) == DVM_BROKER_OK, "retire pressure fixture");
    }
    silent(broker, listener);
    input.key.generation = 2;
    require(dvm_broker_begin(broker, &input, 12, &rejected) == DVM_BROKER_STALE,
            "old/imported generation cannot silently bind a different session");
    dvm_broker_destroy(broker);
    require(pthread_mutex_destroy(&mutex) == 0, "release bounded fixture lock");
}


/* Check canonical wire round trips, all truncation points and write-free malformed rejection. */
static void codec(void)
{
    DVM_BrokerEvent original;
    DVM_BrokerEvent decoded;
    DVM_BrokerEvent sentinel;
    char frame[DVM_BROKER_FRAME_MAX];
    char second[DVM_BROKER_FRAME_MAX];
    char unchanged[DVM_BROKER_FRAME_MAX];
    size_t size = 0;
    size_t copied = 0;
    size_t index;
    memset(&original, 0, sizeof(original));
    original.kind = DVM_BROKER_SLOW;
    original.at_ms = UINT64_MAX;
    original.request.key.channel = UINT32_MAX;
    original.request.key.generation = UINT32_MAX;
    original.request.key.request = UINT64_MAX;
    original.request.object = UINT64_MAX - 1;
    original.request.channel.transport = DVM_BROKER_NETWORK;
    strcpy(original.request.channel.logical, "ai.primary");
    strcpy(original.request.channel.peer, "tcp://host.example:9001");
    strcpy(original.request.operation, "ReadAs(Number)");
    strcpy(original.origin.build, "build");
    strcpy(original.origin.path, "source/Я😀.as");
    original.origin.line = 7;
    original.origin.column = 17;
    original.origin.offset = UINT64_MAX - 1;
    original.origin.length = 1;
    original.creation = original.origin;
    original.connection = original.origin;
    require(dvm_broker_encode(&original, frame, sizeof(frame), &size) == DVM_BROKER_OK,
            "encode diagnostic with exact uint64 IDs and Unicode");
    require(dvm_broker_decode(frame, size, &decoded) == DVM_BROKER_OK && decoded.request.key.request == UINT64_MAX &&
                decoded.request.object == UINT64_MAX - 1 && !strcmp(decoded.origin.path, original.origin.path),
            "portable decoded fields");
    require(dvm_broker_encode(&decoded, second, sizeof(second), &copied) == DVM_BROKER_OK && copied == size &&
                !memcmp(second, frame, size),
            "canonical round trip without native padding");
    memset(&sentinel, 0xa5, sizeof(sentinel));
    for (index = 0; index < size; index++)
    {
        decoded = sentinel;
        require(dvm_broker_decode(frame, index, &decoded) == DVM_BROKER_INVALID &&
                    !memcmp(&decoded, &sentinel, sizeof(decoded)),
                "every truncated frame preserves output");
    }
    frame[size] = '0';
    require(dvm_broker_decode(frame, size + 1, &decoded) == DVM_BROKER_INVALID, "reject trailing wire bytes");
    memset(second, 0x5a, sizeof(second));
    memcpy(unchanged, second, sizeof(second));
    copied = 123;
    require(dvm_broker_encode(&original, second, size - 1, &copied) == DVM_BROKER_INVALID && copied == 123 &&
                !memcmp(second, unchanged, sizeof(second)),
            "short encode is write-free");
    original.origin.path[0] = (char)0xff;
    require(dvm_broker_encode(&original, second, sizeof(second), &copied) == DVM_BROKER_INVALID &&
                !memcmp(second, unchanged, sizeof(second)),
            "reject malformed UTF-8 before publication");
}


/* Run behavior and capacity/concurrency invariants; no external VM or network is mocked as implemented. */
int main(void)
{
    behavior();
    bounds();
    codec();
    puts(
        "PASS broker: deviations only, global/filter listeners, exact sites, paused-owner queries, deadlines, reconnect and bounds");
    return 0;
}
