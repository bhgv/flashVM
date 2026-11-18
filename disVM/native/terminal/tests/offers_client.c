/* Copyright (c) 2026 bhgv
 * EN: Automatic typed offers select repeated/parallel original per-fid VFS fetches on one bidir connection.
 * RU: Автоматические typed offers выбирают повторные/параллельные per-fid VFS fetch одного bidir connection.
 */
#include "resource/announced.h"
#include "announcement.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_OFFER_CASE
#define STYX_OFFER_CASE 0 /* Repeated successful independent service opens. */
#endif


/* EN/RU: Fixture-local constructor failure gate, never a production control operation.
 * Локальный fixture gate отказа конструктора, не production control operation.
 */
extern void servergatearm(void);


/* EN/RU: Explicit fixture invariant, valid in all compiler profiles, borrowing why through return.
 * Явный fixture invariant во всех compiler profiles, why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "typed offer receiver: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Independent caller-owned staging/output per synchronous or parallel fetch, never shared cursors.
 * Независимые staging/output caller каждого fetch, без общих cursors.
 */
typedef struct Fetch
{
    StyxBidirMovieReceiver *receiver; /* Borrowed through fetch/join. */
    StyxBidirRoute *route; /* Borrowed through fetch/join. */
    pthread_t owner; /* Native callback owner selected at call entry. */
    int action; /* Owner-only fixture notice mutation or route stop on first data chunk. */
    int refuse; /* Owner-only injected sink refusal. */
    const char *path; /* Borrowed logical resource path. */
    unsigned calls; /* Owner-only staging calls. */
    size_t size; /* Exact staged bytes. */
    unsigned char bytes[16]; /* Independent bounded staging. */
    char resolved[64]; /* Caller-visible origin sentinel. */
    FVM_Result result; /* Read by main only after call/join. */
} Fetch;


/* EN/RU: Send one fixture-only S-record command, preserving ordinary production type/control schemas.
 * Передать один test-only S-command, не меняя production type/control schemas.
 */
static void command(StyxBidirRoute *route, const char *text)
{
    check(styxbidirroutesendwait(route, 102, text, strlen(text), 5000, NULL) &&
        styxbidirrouteflush(route, 5000), "fixture update uses one complete independent S-record");
}


/* EN/RU: Sink runs only on the caller owner; inject a generation/closure while actual file I/O is active.
 * Sink работает только на caller owner; изменить generation/closure во время настоящего file I/O.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    Fetch *fetch = state; /* Owner-local per-request collector. */
    check(pthread_equal(pthread_self(), fetch->owner), "native sink remains on original fetch owner");
    fetch->calls++;
    if (fetch->refuse)
    {
        return FVM_INVALID_ARGUMENT;
    }
    check(size <= sizeof(fetch->bytes) - fetch->size, "bounded staging");
    memcpy(fetch->bytes + fetch->size, bytes, size);
    fetch->size += size;
    int action = fetch->action; /* Consume mutation once, never repeat a source operation. */
    fetch->action = 0;
    if (action == 3 || action == 4)
    {
        command(fetch->route, action == 3 ? "newer" : "closed");
        check(styxbidirmoviereceiverwait(fetch->receiver, 2, 5000) == (action == 3 ? 1 : -1),
            "offer changes automatically during active resource acquisition");
    }
    if (action == 12)
    {
        styxbidirroutestop(fetch->route);
    }
    return FVM_OK;
}


/* EN/RU: Perform one managed callback on this native owner with its own output/staging, main joins if needed.
 * Выполнить один callback на данном native owner с отдельными output/staging, main при необходимости join.
 */
static void *fetching(void *state)
{
    Fetch *fetch = state; /* Host-owned through synchronous call or thread join. */
    fetch->owner = pthread_self();
    fetch->calls = 0;
    fetch->size = 0;
    strcpy(fetch->resolved, "unchanged");
    fetch->result = styxbidirannouncedfetch(FVM_DESKTOP_FETCH_CODE, fetch->path, "/base.swf", staging,
        fetch, fetch->resolved, sizeof(fetch->resolved), fetch->receiver);
    return NULL;
}


/* EN/RU: Verify independent binary oracle, not fixture SWF execution or a mirrored encoder round-trip.
 * Проверить независимый binary oracle, не исполнение fixture SWF и не encoder round-trip.
 */
static void success(const Fetch *fetch)
{
    static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Actual prepared VFS resource bytes. */
    check(fetch->result == FVM_OK && fetch->calls == 1 && fetch->size == sizeof(expected) &&
        !memcmp(fetch->bytes, expected, sizeof(expected)) && !strcmp(fetch->resolved, "/ok.swf"),
        "independent per-fid resource acquisition and final origin publication");
}


/* EN/RU: Main alone consumes one general item; native receiver owns only the registered offer queue.
 * Main потребляет одну general item, native receiver владеет только своей offer queue.
 */
static void general(StyxBidirRoute *route, unsigned long expected_tag, const char *expected)
{
    unsigned char bytes[512]; /* One copied general payload. */
    unsigned long tag = 0; /* Actual consumed numeric tag. */
    size_t size = 0; /* Exact body extent. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected_tag &&
        (expected ? size == strlen(expected) && !memcmp(bytes, expected, size) : size > 0),
        "UI/general traffic is not consumed by the offer worker");
}


/* EN/RU: Exercise exact-client generation offers and independent ordinary resource fids on real 9P.
 * Проверить exact-client generation offers и отдельные обычные resource fids на настоящем 9P.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Original exact-client bidir grant. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType types[] = {{MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}, {"test.unused", 1}}; /* Exclusive native subscriptions. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 512, 2, "test.ide.fvd1", 1, types, 2);
    check(route && styxbidirrouteclient(route) == client &&
        !styxbidirmoviereceiver(route, 1, 100) && !styxbidirmoviereceiver(route, 0, 0),
        "exact client association and offer-schema preflight");
    if (STYX_OFFER_CASE == 10)
    {
        servergatearm();
        check(!styxbidirmoviereceiver(route, 0, 100), "native worker construction failure leaves route/grant owned");
    }
    StyxBidirMovieReceiver *receiver = styxbidirmoviereceiver(route, 0, STYX_OFFER_CASE == 6 ? 100 : 5000); /* Owned automatic offer consumer. */
    check(receiver != NULL, "owned receiver after optional constructor refusal");
    general(route, 0, NULL);
    general(route, 5, "other");
    DVM_UIMessage ui; /* Main-owned UI value, separate from offers and resource fids. */
    check(styxbidirroutewait(route, 1, 5000) &&
        styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42 && ui.size == 4,
        "actual UI queue stays independent");
    Fetch fetch = {0}; /* One native owner per invocation, preserved across sequential reuse. */
    fetch.receiver = receiver;
    fetch.route = route;
    fetch.path = "/ok.swf";
    if (STYX_OFFER_CASE == 6)
    {
        general(route, 23, NULL);
        check(styxbidirmoviereceiverwait(receiver, 1, 20) == 0 && styxbidirmovieavailable(receiver, NULL) == 0,
            "unsupported offer version grants no resource readiness");
        fetching(&fetch);
        check(fetch.result != FVM_OK && !fetch.calls, "pending timeout opens no resource");
    }
    else if (STYX_OFFER_CASE == 7)
    {
        check(styxbidirmoviereceiverwait(receiver, 1, 5000) == -1 &&
            styxbidirmoviereceiverstate(receiver) == STYX_MOVIE_RECEIVER_FAILED, "malformed offer fails closed locally");
    }
    else if (STYX_OFFER_CASE == 11)
    {
        check(styxbidirmovieavailable(receiver, NULL) == 0, "receiver pending before native stop");
        styxbidirmoviereceiverstop(receiver);
        fetching(&fetch);
        check(fetch.result != FVM_OK && !fetch.calls && styxbidirmoviereceiverwait(receiver, 1, 0) == -1,
            "native stop wakes readiness and prevents future resource walks");
    }
    else if (STYX_OFFER_CASE == 2)
    {
        pthread_t thread; /* Independent native fetch waiting for initial readiness. */
        check(styxbidirmovieavailable(receiver, NULL) == 0 && !pthread_create(&thread, NULL, fetching, &fetch),
            "native callback starts before delayed announcement");
        command(route, "ready");
        check(!pthread_join(thread, NULL), "native readiness fetch retired");
        success(&fetch);
    }
    else
    {
        check(styxbidirmoviereceiverwait(receiver, 1, 5000) == 1, "autonomous initial offer without manual feed");
        if (STYX_OFFER_CASE == 1)
        {
            Fetch parallel[3] = {fetch, fetch, fetch}; /* Three independent output/sink/fid owners. */
            pthread_t workers[3]; /* Joined before receiver retirement. */
            for (unsigned i = 0; i < 3; i++)
            {
                check(!pthread_create(&workers[i], NULL, fetching, &parallel[i]), "parallel native resource owner");
            }
            for (unsigned i = 0; i < 3; i++)
            {
                check(!pthread_join(workers[i], NULL), "parallel resource owner joined");
                success(&parallel[i]);
            }
        }
        else if (STYX_OFFER_CASE == 8)
        {
            command(route, "conflict");
            check(styxbidirmoviereceiverwait(receiver, 2, 5000) == -1 &&
                styxbidirmoviereceiverstate(receiver) == STYX_MOVIE_RECEIVER_FAILED, "same-generation contradiction refuses");
        }
        else if (STYX_OFFER_CASE == 9)
        {
            command(route, "newer");
            check(styxbidirmoviereceiverwait(receiver, 2, 5000) == 1, "new generation received automatically");
            command(route, "old");
            check(styxbidirmoviereceiverwait(receiver, 3, 5000) == 1, "old CLOSED cannot revoke newer generation");
            fetching(&fetch);
            success(&fetch);
        }
        else
        {
            fetch.action = STYX_OFFER_CASE == 3 || STYX_OFFER_CASE == 4 || STYX_OFFER_CASE == 12 ? STYX_OFFER_CASE : 0;
            fetch.refuse = STYX_OFFER_CASE == 5;
            fetch.path = STYX_OFFER_CASE == 13 ? "/denied.swf" : "/ok.swf";
            fetching(&fetch);
            if (STYX_OFFER_CASE == 3 || STYX_OFFER_CASE == 4 || STYX_OFFER_CASE == 5 ||
                STYX_OFFER_CASE == 12 || STYX_OFFER_CASE == 13)
            {
                check(fetch.result != FVM_OK && !strcmp(fetch.resolved, "unchanged") &&
                    (STYX_OFFER_CASE == 13 ? !fetch.calls : fetch.calls == 1), "failure/change does not publish staged origin");
                if (STYX_OFFER_CASE == 5)
                {
                    check(fetch.result == FVM_INVALID_ARGUMENT, "sink refusal preserved without retry");
                }
                if (STYX_OFFER_CASE == 4 || STYX_OFFER_CASE == 12)
                {
                    fetching(&fetch);
                    check(fetch.result != FVM_OK && !fetch.calls, "retired offer never opens a fresh resource fid");
                }
            }
            else
            {
                success(&fetch);
            }
            if (STYX_OFFER_CASE != 4 && STYX_OFFER_CASE != 12)
            {
                fetch.refuse = 0;
                fetch.path = "/ok.swf";
                fetching(&fetch);
                success(&fetch);
            }
        }
    }
    check(styxbidirmoviereceiverfree(receiver), "join only native offer worker and release private session");
    if (STYX_OFFER_CASE != 12)
    {
        command(route, "alive");
        general(route, 9, "alive");
    }
    styxbidirroutefree(route);
    check(styxclientalive(client), "ordinary original 9P session survives receiver/route retirement");
}
