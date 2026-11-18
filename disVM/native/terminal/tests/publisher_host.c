/* Copyright (c) 2026 bhgv
 * EN: Native source changes cross real fragmented Styx and return through the automatic offer receiver.
 * RU: Native source changes проходят real fragmented Styx и автоматический offer receiver.
 */
#include "resource/outgoing.h"
#include "resource/announced.h"
#include "announcement.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_PUBLISHER_CASE
#define STYX_PUBLISHER_CASE 0 /* Ready, replacement, ordinary per-fid fetch and closed. */
#endif


/* EN/RU: Owned real prepared VFS service/NULL; no VM entry or network construction.
 * Свой real prepared VFS service/NULL, без VM entry либо создания сети.
 */
extern DVM_Provider *exchangeprovider(void);


/* EN/RU: Consume the route after all its borrowers retire; verify sticky subscription lifetime.
 * Освободить route после borrowers, проверить lifetime sticky subscriptions.
 */
extern void publisherwatchfree(StyxBidirRoute *route);


/* EN/RU: Arm one fixture-only native TX/constructor fault before publisher creation.
 * Включить один test-only native TX/constructor fault до создания publisher.
 */
extern void servergatearm(void);


/* EN/RU: Wait for the native test TX checkpoint; bounded externally by the fixture process timeout.
 * Ждать native test TX checkpoint; снаружи ограничено timeout процесса fixture.
 */
extern void servergatewait(void);


/* EN/RU: Release the gated native TX after joining only the independent producer when required.
 * Освободить native TX gate, при необходимости после join только независимого producer.
 */
extern void servergaterelease(void);


/* EN/RU: Fail explicitly in every build profile; why borrowed until return/exit.
 * Явный отказ во всех build profiles, why заимствован до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "offer publisher: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Consume exactly one general record, preserving typed/UI ownership; expected NULL checks extent only.
 * Потребить одну general record, не трогая typed/UI; expected NULL проверяет только extent.
 */
static void general(StyxBidirRoute *route, unsigned long expected_tag, const char *expected)
{
    unsigned char bytes[512]; /* Caller-owned one-record copy. */
    unsigned long tag = 99; /* Actual tag on successful receive. */
    size_t size = 0; /* Exact payload extent. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected_tag && (expected ? size == strlen(expected) && !memcmp(bytes, expected, size) : size > 0),
        "one complete independent general record");
}


/* EN/RU: Check actual VFS service bytes on synchronous caller, counting independent sink invocations.
 * Проверить bytes настоящего VFS service на synchronous caller, count принадлежит caller.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent binary oracle. */
    unsigned *calls = state; /* Main-owned synchronous sink count. */
    check(size == sizeof(expected) && !memcmp(bytes, expected, size), "actual per-fid binary resource");
    (*calls)++;
    return FVM_OK;
}


/* EN/RU: Acquire a fresh ordinary movie/generation fid after automatic readiness, no manual offer feed.
 * Новый обычный movie/generation fid после автоматической readiness, без manual offer feed.
 */
static void fetch(StyxBidirMovieReceiver *receiver, unsigned long long generation)
{
    unsigned calls = 0; /* Independent output for this invocation. */
    char resolved[64] = "unchanged"; /* Published only after successful final generation fence. */
    check(styxbidirmoviereceiverwait(receiver, generation, 5000) == 1 &&
        styxbidirannouncedfetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging, &calls,
            resolved, sizeof(resolved), receiver) == FVM_OK && calls == 1 && !strcmp(resolved, "/ok.swf"),
        "source notification reaches native receiver and ordinary resource fetch");
}


/* EN/RU: Observe local queue admission plus real peer declaration; these have different guarantees.
 * Наблюдать local queue admission и настоящую peer declaration, гарантии различаются.
 */
static void queued(StyxBidirRoute *route, StyxBidirMoviePublisher *publisher, unsigned long long generation)
{
    check(styxbidirmoviepublisherwait(publisher, generation, 5000) == 1, "whole pair admitted to TX");
    general(route, 0, NULL);
}


/* EN/RU: Check shared source remains independently available after local subscriber cancellation.
 * Проверить независимую доступность shared source после локальной отмены подписчика.
 */
static void live(DVM_MovieOffer *source, unsigned long long generation)
{
    DVM_MovieOfferInfo info; /* Coherent caller-owned source snapshot. */
    check(dvm_movie_offer_info(source, &info) && info.generation == generation && !info.closed &&
        info.ready == (generation != 0), "local producer lifetime never withdraws shared source");
}


/* EN/RU: Run one real bidir fixture scenario; retire all borrowers before freeing shared route/client.
 * Один real bidir fixture scenario, завершить borrowers до освобождения shared route/client.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    const int scenario = STYX_PUBLISHER_CASE; /* Compile-time deterministic fault selection. */
    DVM_TerminalChannels channels = *descriptor; /* Exact granted client association. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType type = {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}; /* Exclusive incoming offer consumer. */
    int gated = scenario == 4 || scenario == 7 || scenario == 9; /* Native TX deliberately held. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 512, gated ? 1 : 2,
        scenario == 10 ? NULL : "test.ide.fvd1", scenario == 10 ? 0 : 1, &type, 1);
    check(route != NULL, "owned exact-client shared route");
    StyxBidirMovieReceiver *receiver = styxbidirmoviereceiver(route, 0, 5000); /* Independent native RX owner. */
    check(receiver != NULL, "automatic offer receiver owns only typed RX queue");
    general(route, 0, NULL);
    general(route, 5, "other");
    if (scenario == 10)
    {
        general(route, 17, NULL);
    }
    else
    {
        DVM_UIMessage ui; /* Optional UI consumer independent of resource workers. */
        check(styxbidirroutewait(route, 1, 5000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD &&
            ui.id == 42 && ui.size == 4, "UI remains available");
    }
    DVM_Provider *root = exchangeprovider(); /* Caller pin separate from source subscription. */
    DVM_MovieOffer *source = dvm_movie_offer_create(); /* Shared source with independently owned subscribers. */
    check(root && source && !styxbidirmoviepublisher(route, source, 0, 5000) &&
        !styxbidirmoviepublisher(route, source, 23, 0), "constructor rejects invalid TX tag/budget");
    int empty = scenario == 1 || scenario == 5; /* Exercise idle native outbox before first source root. */
    check(empty || dvm_movie_offer_bind(source, 0, root) == 1, "initial source ownership");
    if (scenario == 2)
    {
        check(dvm_movie_offer_close(source, 1), "source already retired before subscription");
    }
    if (gated || scenario == 6 || scenario == 8)
    {
        servergatearm();
    }
    StyxBidirMoviePublisher *publisher = styxbidirmoviepublisher(route, source, 23, scenario == 7 ? 100 : 5000);
    if (scenario == 8)
    {
        check(!publisher, "worker creation failure preserves caller ownership");
        live(source, 1);
        publisher = styxbidirmoviepublisher(route, source, 23, 5000);
    }
    check(publisher != NULL, "native autonomous publisher created");
    if (gated)
    {
        servergatewait();
        if (scenario == 4)
        {
            check(dvm_movie_offer_bind(source, 1, root) == 2 && dvm_movie_offer_bind(source, 2, root) == 3 &&
                dvm_movie_offer_close(source, 3), "source changes while immutable first pair is pending");
            servergaterelease();
            queued(route, publisher, 1);
            queued(route, publisher, 4);
            check(styxbidirmoviereceiverwait(receiver, 4, 5000) == -1 &&
                styxbidirmoviepublisherstate(publisher, NULL) == STYX_MOVIE_PUBLISHER_DONE,
                "latest CLOSED coalesces intermediate updates without changing pending pair");
        }
        else
        {
            if (scenario == 9)
            {
                styxbidirmoviepublisherstop(publisher);
            }
            unsigned long long generation = 99; /* No whole pair accepted while declaration is gated. */
            check(styxbidirmoviepublisherwait(publisher, 0, 5000) == 1 &&
                styxbidirmoviepublisherstate(publisher, &generation) ==
                    (scenario == 9 ? STYX_MOVIE_PUBLISHER_STOPPED : STYX_MOVIE_PUBLISHER_FAILED) && !generation &&
                styxbidirmoviepublisherfree(publisher), "stop/timeout joins publisher without waiting on shared TX");
            publisher = NULL;
            live(source, 1);
            servergaterelease();
            check(styxbidirrouteflush(route, 5000), "accepted declaration remains route-owned after producer free");
            general(route, 0, NULL);
        }
    }
    else if (scenario == 6)
    {
        check(styxbidirmoviepublisherwait(publisher, 0, 5000) == 1 &&
            styxbidirmoviepublisherstate(publisher, NULL) == STYX_MOVIE_PUBLISHER_FAILED &&
            styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED, "uncertain fragment retires route without replay");
    }
    else if (scenario == 5)
    {
        check(styxbidirmoviepublisherwait(publisher, 1, 20) == 0, "empty source waits natively");
        styxbidirmoviepublisherstop(publisher);
        check(styxbidirmoviepublisherwait(publisher, 1, 0) == -1 &&
            styxbidirmoviepublisherstate(publisher, NULL) == STYX_MOVIE_PUBLISHER_STOPPED, "idle stop wakes native FD wait");
        live(source, 0);
    }
    else if (scenario == 2)
    {
        queued(route, publisher, 2);
        check(styxbidirmoviereceiverwait(receiver, 2, 5000) == -1 &&
            styxbidirmoviepublisherstate(publisher, NULL) == STYX_MOVIE_PUBLISHER_DONE, "initial CLOSED, no fabricated READY");
    }
    else
    {
        if (empty)
        {
            check(styxbidirmoviepublisherwait(publisher, 1, 20) == 0 &&
                dvm_movie_offer_bind(source, 0, root) == 1, "late publication wakes native outbox");
        }
        queued(route, publisher, 1);
        fetch(receiver, 1);
        if (scenario == 3)
        {
            StyxBidirMoviePublisher *second = styxbidirmoviepublisher(route, source, 24, 5000); /* Independent subscriber. */
            check(second != NULL, "second publisher shares source, has its own reserved TX tag");
            queued(route, second, 1);
            check(styxbidirmoviepublisherfree(publisher), "first subscriber retired independently");
            publisher = second;
            live(source, 1);
        }
        check(dvm_movie_offer_bind(source, 1, root) == 2, "replace source live");
        queued(route, publisher, 2);
        fetch(receiver, 2);
        check(dvm_movie_offer_close(source, 2), "orderly source withdrawal");
        queued(route, publisher, 3);
        check(styxbidirmoviereceiverwait(receiver, 3, 5000) == -1 &&
            styxbidirmoviepublisherstate(publisher, NULL) == STYX_MOVIE_PUBLISHER_DONE, "real peer received CLOSED generation");
    }
    check(styxbidirmoviepublisherfree(publisher) && styxbidirmoviereceiverfree(receiver), "only own native workers joined");
    dvm_movie_offer_release(source);
    dvm_provider_release(root);
    if (scenario != 6)
    {
        check(!styxbidirroutestate(route) && styxbidirroutesendwait(route, 102, "alive", 5, 5000, NULL),
            "shared TX remains usable after local publisher retirement");
        general(route, 9, "alive");
    }
    publisherwatchfree(route);
    check(styxclientalive(client), "ordinary original 9P session remains alive");
}
