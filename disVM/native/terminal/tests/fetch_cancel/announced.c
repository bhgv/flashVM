/* Copyright (c) 2026 bhgv
 * EN: Real deferred resource Tread cancellation and independent per-call scopes on one announced receiver.
 * RU: Отмена real deferred resource Tread и независимые per-call scopes одного announced receiver.
 */
#include "resource/announced_internal.h"
#include "checkpoint.h"
#include <stdio.h>
#ifndef STYX_FETCH_CANCEL_CASE
#define STYX_FETCH_CANCEL_CASE 0 /* Receiver stop while original resource Tread is pending. */
#endif


/* EN/RU: Arm one owner-local scope subscription refusal before native fetch setup.
 * Включить один owner-local отказ subscription до native fetch setup.
 */
extern void fetchfailarm(unsigned number);


/* EN/RU: Arm one actual native Twrite uncertain-prefix fault, preserving real 9P I/O.
 * Включить один actual native Twrite uncertain-prefix fault, сохраняя real 9P I/O.
 */
extern void servergatearm(void);


/* EN/RU: Explicit invariant in every build profile, why borrowed through return or exit.
 * Явный invariant во всех build profiles, why borrowed до return либо exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "announced fetch cancellation: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One caller owns sink/output and its parent source; only receiver/client are shared with other fetches.
 * Один caller владеет sink/output и parent source; с другими fetches общие только receiver/client.
 */
typedef struct Fetch
{
    StyxBidirMovieReceiver *receiver; /* Borrowed through call/join. */
    StyxMovieSession *raw;            /* Optional borrowed raw session for direct scoped RPC acceptance. */
    DVM_Cancel *outer;                /* Main pins independent caller source through join. */
    pthread_t owner;                  /* Original native sink owner. */
    unsigned block;                   /* Block this sink only, never another fetch. */
    unsigned fail;                    /* Selected owner-local subscription setup refusal. */
    unsigned calls;                   /* Native sink invocation count, owner only. */
    FVM_Result result;                /* Read by main after join. */
    char resolved[64];                /* Caller origin remains a sentinel on every failure. */
} Fetch;


/* EN/RU: Validate exact service bytes and native owner, optionally wait for this fetch's scope cancellation.
 * Проверить service bytes/native owner, optional ждать cancellation только scope данного fetch.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent VFS source oracle. */
    Fetch *fetch = state; /* Owned by synchronous caller. */
    check(pthread_equal(pthread_self(), fetch->owner) && dvm_cancel_current() &&
        size == sizeof(expected) && !memcmp(bytes, expected, size), "native sink owner/scope and exact source bytes");
    fetch->calls++;
    if (STYX_FETCH_CANCEL_CASE == 16 && fetch->raw)
    {
        unsigned char notice[MOVIE_OFFER_FRAME]; /* Existing raw-session generation update during staging. */
        size_t encoded; /* Exact complete notice extent. */
        check(movie_offer_record(23, MOVIE_OFFER_READY, 2, notice, sizeof(notice), &encoded) &&
            styxmoviecontrol(fetch->raw, notice, encoded), "raw-session generation changes before final origin publication");
    }
    if (fetch->block)
    {
        fetchsinkwait();
    }
    return FVM_OK;
}


/* EN/RU: Enter caller-owned scope and perform one ordinary/typed-announced fetch on this native owner.
 * Войти в caller-owned scope и выполнить один raw/typed-announced fetch на данном native owner.
 */
static void *loading(void *state)
{
    Fetch *fetch = state; /* Main owns through call/join. */
    DVM_CancelScope scope = {0}; /* Independent caller scope restored before return. */
    fetch->owner = pthread_self();
    strcpy(fetch->resolved, "unchanged");
    check(dvm_cancel_enter(&scope, fetch->outer), "caller scope entered");
    if (fetch->fail)
    {
        fetchfailarm(fetch->fail);
    }
    fetch->result = fetch->raw ? styxmovieannouncedfetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging,
        fetch, fetch->resolved, sizeof(fetch->resolved), fetch->raw) :
        styxbidirannouncedfetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging,
            fetch, fetch->resolved, sizeof(fetch->resolved), fetch->receiver);
    check(dvm_cancel_current() == fetch->outer && dvm_cancel_leave(&scope), "caller scope restored without parent mutation");
    return NULL;
}


/* EN/RU: Perform another complete independent per-fid fetch on the shared receiver; no old source is reused.
 * Другой complete per-fid fetch общего receiver, без повторного использования старого source.
 */
static void successful(StyxBidirMovieReceiver *receiver)
{
    Fetch fetch = {0}; /* Separate output, sink and caller source. */
    fetch.receiver = receiver;
    fetch.outer = dvm_cancel_create();
    check(fetch.outer != NULL, "independent caller source");
    loading(&fetch);
    check(fetch.result == FVM_OK && fetch.calls == 1 && !strcmp(fetch.resolved, "/ok.swf") &&
        !dvm_cancel_requested(fetch.outer), "independent fetch succeeds while another caller is cancelled");
    dvm_cancel_release(fetch.outer);
}


/* EN/RU: Read one general item without consuming offers; caller owns all outputs until return.
 * Прочитать одну general item без потребления offers; outputs принадлежат caller до return.
 */
static void general(StyxBidirRoute *route, unsigned long expected, const char *payload)
{
    unsigned char bytes[512]; /* One copied record. */
    size_t size; /* Exact body extent. */
    unsigned long tag; /* Original numeric tag. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected &&
        (!payload || (size == strlen(payload) && !memcmp(bytes, payload, size))), "independent general traffic");
}


/* EN/RU: One fixture-only command stays fully S-framed, separate from production offer/resource schemas.
 * Одна fixture-only command остаётся полностью S-framed, отдельно от production schemas.
 */
static void command(StyxBidirRoute *route, const char *text)
{
    check(styxbidirroutesendwait(route, 102, text, strlen(text), 5000, NULL) &&
        styxbidirrouteflush(route, 5000), "complete independent fixture command");
}


/* EN/RU: Exercise RPC Tflush, native sink wake, caller isolation and constructor/setup rollback on real 9P.
 * Проверить RPC Tflush, native sink wake, caller isolation и constructor/setup rollback на real 9P.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    unsigned scenario = STYX_FETCH_CANCEL_CASE; /* Compile-time deterministic trigger. */
    DVM_TerminalChannels channels = *descriptor; /* Existing exact trusted bidir grant. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType type = {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}; /* Receiver alone consumes offers. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, MOVIE_FRAME + 32, 2, "test.ide.fvd1", 1, &type, 1);
    check(route != NULL, "owned exact-client route");
    if (scenario == 12)
    {
        check(!styxbidirmoviereceiver(route, 0, 5000) && styxclientalive(client), "constructor subscription failure rolls back privately");
    }
    StyxBidirMovieReceiver *receiver = styxbidirmoviereceiver(route, 0, 5000); /* Shared native receiver, separate from each fetch. */
    check(receiver != NULL, "automatic receiver after optional rollback");
    general(route, 0, NULL);
    general(route, 5, "other");
    DVM_UIMessage ui; /* Independent UI consumer. */
    check(styxbidirroutewait(route, 1, 5000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42,
        "UI remains independent");
    if (scenario == 12)
    {
        successful(receiver);
    }
    else
    {
        Fetch fetch = {0}; /* One externally pinned asynchronous caller. */
        fetch.receiver = receiver;
        fetch.outer = dvm_cancel_create();
        fetch.block = scenario >= 6 && scenario <= 10;
        fetch.fail = scenario >= 13 && scenario <= 15 ? scenario - 12 : 0;
        check(fetch.outer != NULL, "owned caller source");
        if (scenario == 5 || scenario == 16)
        {
            fetch.raw = styxmoviesession(client);
            unsigned char bytes[MOVIE_OFFER_FRAME * 2]; /* Explicit test-only trusted raw session grant. */
            size_t first; /* Declaration size. */
            size_t second; /* Offer size. */
            check(fetch.raw && movie_offer_binding(23, bytes, MOVIE_OFFER_FRAME, &first) &&
                movie_offer_record(23, MOVIE_OFFER_READY, 1, bytes + first, sizeof(bytes) - first, &second) &&
                styxmoviecontrol(fetch.raw, bytes, first + second), "raw session uses same scoped original resource fids");
        }
        if (scenario == 11)
        {
            fetcharmwait(&receiver->changed);
        }
        pthread_t thread; /* Joined before caller source, receiver or route disposal. */
        check(!pthread_create(&thread, NULL, loading, &fetch), "native fetch owner");
        if (scenario < 6)
        {
            general(route, 9, "held");
        }
        else if (scenario <= 10)
        {
            fetchwait(FETCH_SINK);
        }
        else if (scenario == 11)
        {
            fetchwait(FETCH_READY_WAIT);
        }
        if (scenario == 2 || scenario == 8)
        {
            successful(receiver);
        }
        struct timespec started; /* Exclude setup/parallel success; measure only cancellation completion. */
        struct timespec ended; /* Native owner join time. */
        check(!clock_gettime(CLOCK_MONOTONIC, &started), "cancellation start clock");
        if (scenario == 0 || scenario == 6)
        {
            styxbidirmoviereceiverstop(receiver);
        }
        else if (scenario == 1 || scenario == 7)
        {
            styxbidirroutestop(route);
        }
        else if (scenario == 3 || scenario == 9)
        {
            command(route, "closed");
            check(styxbidirmoviereceiverwait(receiver, 2, 1000) == -1, "CLOSED automatically retires active fetches");
        }
        else if (scenario == 4 || scenario == 10)
        {
            servergatearm();
            int admitted = styxbidirroutesendwait(route, 102, "tail", 4, 5000, NULL); /* Failure can precede tail admission. */
            check((admitted || styxbidirroutestate(route)) && !styxbidirrouteflush(route, 5000) &&
                styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED, "real uncertain TX retires active caller I/O");
        }
        else if (scenario < 13)
        {
            dvm_cancel_request(fetch.outer);
        }
        check(!pthread_join(thread, NULL) && !clock_gettime(CLOCK_MONOTONIC, &ended), "native fetch retired");
        long elapsed = (ended.tv_sec - started.tv_sec) * 1000L + (ended.tv_nsec - started.tv_nsec) / 1000000L;
        check(elapsed < 1500, "active fetch cancellation wakes before RPC/readiness timeout");
        check(fetch.result != FVM_OK && !strcmp(fetch.resolved, "unchanged") &&
            fetch.calls == (fetch.block || scenario == 16 ? 1U : 0U) &&
            dvm_cancel_requested(fetch.outer) == (scenario == 2 || scenario == 5 || scenario == 8 || scenario == 11),
            "cancelled/refused fetch preserves parent source, origin and one-call sink semantics");
        if (scenario < 6)
        {
            fetchwait(FETCH_FLUSH);
        }
        if (scenario == 11)
        {
            fetcharmwait(NULL);
            command(route, "ready");
        }
        if (scenario == 2 || scenario == 5 || scenario == 8 || scenario == 11 || scenario >= 13)
        {
            dvm_cancel_request(fetch.outer);
            check(styxbidirmoviereceiverwait(receiver, 1, 1000) == 1, "caller cancellation never flows into shared receiver");
            successful(receiver);
        }
        styxmoviesessionfree(fetch.raw);
        dvm_cancel_release(fetch.outer);
    }
    check(styxbidirmoviereceiverfree(receiver), "receiver worker joined after every external fetch retired");
    styxbidirroutefree(route);
    StyxFid *identity = styxclientwalk(client, "identity"); /* Independent ordinary RPC after all cancellation/cleanup. */
    check(identity && styxclientopen(identity, 0), "ordinary shared client remains usable");
    char byte; /* One immutable peer identity byte. */
    StyxTransfer result; /* Original 9P outcome. */
    styxclientread(identity, 0, &byte, 1, &result);
    check(result.success && result.count == 1, "ordinary shared client after resource Tflush");
    styxclientclunk(identity);
}
