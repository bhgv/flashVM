/* Copyright (c) 2026 bhgv
 * EN: One-shot typed resource sink observes route/local/caller cancellation on its native owner.
 * RU: Sink одноразового typed resource учитывает route/local/caller cancellation на native owner.
 */
#include "resource/client.h"
#include "protocol.h"
#include "cancel.h"
#include "checkpoint.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_TYPED_FETCH_CASE
#define STYX_TYPED_FETCH_CASE 0 /* Route stop while native staging I/O is blocked. */
#endif


/* EN/RU: Arm one real native uncertain Twrite fault after resource data reaches its sink.
 * Включить один real native uncertain Twrite fault после поступления resource data в sink.
 */
extern void servergatearm(void);


/* EN/RU: Explicit always-enabled fixture check, why borrowed through return/exit.
 * Явная fixture check во всех профилях, why borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "typed fetch cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Caller owns native output/sink/source through thread join, client/route borrowed through free.
 * Caller владеет native output/sink/source до join, client/route borrowed до free.
 */
typedef struct Fetch
{
    StyxBidirMovieClient *client; /* Main-owned one-shot binding. */
    DVM_Cancel *outer;            /* Caller source, never requested by route/local cancellation. */
    pthread_t owner;              /* Native staging owner selected at entry. */
    FVM_Result result;            /* Main reads after thread join. */
    unsigned calls;               /* Exact sink count, owner only. */
    char resolved[64];            /* Failure sentinel, never partial origin. */
} Fetch;


/* EN/RU: Check real service bytes on original owner, then wait for this scope's native cancellation.
 * Проверить real service bytes на original owner и ждать native cancellation данного scope.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent binary service oracle. */
    Fetch *fetch = state; /* Synchronous owner-local state. */
    check(pthread_equal(pthread_self(), fetch->owner) && size == sizeof(expected) &&
        !memcmp(bytes, expected, size), "one native sink invocation with actual source bytes");
    fetch->calls++;
    fetchsinkwait();
    return FVM_OK;
}


/* EN/RU: Enter outer scope and perform one fetch; no parent cancellation or replay on return.
 * Войти во внешний scope и выполнить один fetch; без отмены parent либо replay при возврате.
 */
static void *loading(void *state)
{
    Fetch *fetch = state; /* Main pins until join. */
    DVM_CancelScope scope = {0}; /* Native caller's original scope. */
    fetch->owner = pthread_self();
    strcpy(fetch->resolved, "unchanged");
    check(dvm_cancel_enter(&scope, fetch->outer), "caller scope entered");
    fetch->result = styxbidirmoviefetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging, fetch,
        fetch->resolved, sizeof(fetch->resolved), fetch->client);
    check(dvm_cancel_current() == fetch->outer && dvm_cancel_leave(&scope), "caller scope restored");
    return NULL;
}


/* EN/RU: Read one general record, preserving the typed queue exclusively for its fetch caller.
 * Прочитать одну general record, оставив typed queue только fetch caller.
 */
static void general(StyxBidirRoute *route, unsigned long expected, const char *payload)
{
    unsigned char bytes[512]; /* One caller-owned body copy. */
    unsigned long tag; /* Exact copied numeric tag. */
    size_t size; /* Exact copied extent. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected &&
        (!payload || (size == strlen(payload) && !memcmp(bytes, payload, size))), "general route remains independent");
}


/* EN/RU: Cancel a native active sink without closing unrelated session or silently reusing its one-shot channel.
 * Отменить native active sink без закрытия общего session либо скрытого reuse one-shot channel.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Same trusted bidir grant as ordinary RPCs. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType type = {MOVIE_TYPE, MOVIE_VERSION}; /* One exclusive native resource RX queue. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, MOVIE_FRAME + 32, 2, "test.ide.fvd1", 1, &type, 1);
    check(route != NULL, "owned native route");
    general(route, 0, NULL);
    general(route, 5, "other");
    DVM_UIMessage ui; /* Separate optional UI consumer. */
    check(styxbidirroutewait(route, 1, 5000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD,
        "separate UI message");
    if (STYX_TYPED_FETCH_CASE == 4)
    {
        check(!styxbidirmovieclient(route, 0, 101, 5000) && !styxbidirroutestate(route),
            "constructor subscription refusal preserves shared route");
    }
    Fetch fetch = {0}; /* Externally pinned one-shot caller. */
    fetch.client = styxbidirmovieclient(route, 0, 101, 5000);
    fetch.outer = dvm_cancel_create();
    pthread_t thread; /* Own caller thread through join. */
    check(fetch.client && fetch.outer && !pthread_create(&thread, NULL, loading, &fetch), "owned client and native caller");
    general(route, 9, "request");
    fetchwait(FETCH_SINK);
    if (STYX_TYPED_FETCH_CASE == 0)
    {
        styxbidirroutestop(route);
    }
    else if (STYX_TYPED_FETCH_CASE == 1)
    {
        servergatearm();
        int admitted = styxbidirroutesendwait(route, 102, "tail", 4, 5000, NULL); /* Unknown effects remain nonreplayed. */
        check((admitted || styxbidirroutestate(route)) && !styxbidirrouteflush(route, 5000) &&
            styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED, "uncertain TX wakes only this sink scope");
    }
    else if (STYX_TYPED_FETCH_CASE == 2 || STYX_TYPED_FETCH_CASE == 4)
    {
        styxbidirmovieclientstop(fetch.client);
    }
    else
    {
        dvm_cancel_request(fetch.outer);
    }
    check(!pthread_join(thread, NULL) && fetch.result != FVM_OK && fetch.calls == 1 &&
        !strcmp(fetch.resolved, "unchanged") &&
        dvm_cancel_requested(fetch.outer) == (STYX_TYPED_FETCH_CASE == 3),
        "sink cancellation preserves origin, parent source and one-call semantics");
    styxbidirmovieclientfree(fetch.client);
    dvm_cancel_release(fetch.outer);
    if (STYX_TYPED_FETCH_CASE >= 2)
    {
        check(!styxbidirroutestate(route) && styxbidirroutesendwait(route, 102, "alive", 5, 5000, NULL),
            "local/caller stop leaves shared TX usable");
        general(route, 9, "alive");
    }
    styxbidirroutefree(route);
    check(styxclientalive(client), "shared original client survives native sink cancellation");
}
