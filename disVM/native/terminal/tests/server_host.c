/* Copyright (c) 2026 bhgv
 * EN: Actual autonomous VFS resource server beside live UI/general traffic over original 9P.
 * RU: Настоящий автономный VFS resource server рядом с UI/general через исходный 9P.
 */
#include "resource/server.h"
#include "protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_SERVER_CASE
#define STYX_SERVER_CASE 0 /* Successful autonomous acquisition. */
#endif


/* EN/RU: Owned prepared namespace from the native fixture; caller releases after server retains it.
 * Owned prepared namespace fixture, caller освобождает после удержания server.
 */
extern DVM_Namespace *exchangegroup(void);


/* EN/RU: Change live root policy before fixture-gated request admission, group borrowed.
 * Изменить policy живого root до допуска fixture-gated запроса, group заимствован.
 */
extern int exchangegroupdeny(DVM_Namespace *group);


/* EN/RU: Arm/reach/release deterministic native fault checkpoints, never guest execution.
 * Включить/дождаться/освободить native fault checkpoints, без guest execution.
 */
extern void servergatearm(void);
extern void servergatewait(void);
extern void servergaterelease(void);


/* EN/RU: Fatal always-enabled fixture invariant; why borrowed through return.
 * Явный fatal invariant в любом build profile, why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "autonomous Unibus server: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Consume one general record, leaving all resource inputs with the autonomous worker.
 * Прочитать одну общую запись, resource input остаётся автономному worker.
 */
static void general(StyxBidirRoute *route, unsigned long expected_tag, const char *expected)
{
    unsigned char bytes[512]; /* One copied general record. */
    unsigned long tag = 0; /* Published tag. */
    size_t size = 0; /* Published body size. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected_tag && (expected ? size == strlen(expected) && !memcmp(bytes, expected, size) : size > 0),
        "general traffic remains independent");
}


/* EN/RU: Borrow actual bidir grant, let production server own resource RX and VFS work independently.
 * Заимствовать настоящий bidir grant; production server независимо владеет resource RX и VFS work.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Exact original trusted grant. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType types[] = {{MOVIE_TYPE, MOVIE_VERSION}, {"test.unused", 1}}; /* Separate local consumer registrations. */
    int data_only = STYX_SERVER_CASE == 11; /* No UI decoder/context is required by the server. */
    unsigned slots = STYX_SERVER_CASE == 1 || STYX_SERVER_CASE == 6 || STYX_SERVER_CASE == 7 ? 1 : 2;
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 16384, slots,
        data_only ? NULL : "test.ide.fvd1", data_only ? 0 : 1, types, 2);
    DVM_Namespace *group = exchangegroup(); /* Owned prepared root, retained by real server on success. */
    check(route && group && !styxbidirmovieserver(route, 1, 101, group, 5000) &&
        !styxbidirmovieserver(route, 0, 0, group, 5000) && !styxbidirmovieserver(route, 0, 101, group, 0),
        "server validates RX schema, TX authority and budget before I/O");
    if (STYX_SERVER_CASE == 6 || STYX_SERVER_CASE == 7 || STYX_SERVER_CASE == 9)
    {
        servergatearm();
    }
    StyxBidirMovieServer *server = styxbidirmovieserver(route, 0, 101, group, STYX_SERVER_CASE == 5 ? 300 : 5000);
    if (STYX_SERVER_CASE == 9)
    {
        DVM_NamespacePlan plan; /* Caller ownership preserved after constructor failure. */
        check(!server && dvm_namespace_plan(group, "/", &plan), "failed worker start preserves caller group");
    }
    else
    {
        check(server != NULL, "owned autonomous VFS server");
    }
    if (STYX_SERVER_CASE == 10)
    {
        check(exchangegroupdeny(group), "live prepared root changes after server creation");
        check(styxbidirroutesend(route, 102, "start", 5), "fixture-gated request follows live namespace change");
    }
    dvm_namespace_release(group);
    general(route, 0, NULL);
    general(route, 5, "other");
    if (data_only)
    {
        general(route, 17, NULL);
    }
    else
    {
        DVM_UIMessage ui; /* UI stays outside the resource worker. */
        check(styxbidirroutewait(route, 1, 5000) &&
            styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42 && ui.size == 4,
            "UI survives autonomous resource work");
    }
    if (STYX_SERVER_CASE == 4)
    {
        check(styxbidirmovieserverstate(server) == STYX_MOVIE_SERVER_WAITING, "native server idle without request");
        styxbidirmovieserverstop(server);
    }
    if (STYX_SERVER_CASE == 6)
    {
        servergatewait();
        styxbidirmovieserverstop(server);
        check(styxbidirmovieserverwait(server, 5000) &&
            styxbidirmovieserverstate(server) == STYX_MOVIE_SERVER_STOPPED && styxbidirmovieserverfree(server),
            "server cancellation joins its worker while independent TX is gated");
        server = NULL;
        servergaterelease();
        check(styxbidirrouteflush(route, 5000), "already accepted prefix survives server retirement");
    }
    else if (server)
    {
        int expected = STYX_SERVER_CASE == 2 || STYX_SERVER_CASE == 10 ? STYX_MOVIE_SERVER_REFUSED :
            STYX_SERVER_CASE == 3 || STYX_SERVER_CASE == 5 || STYX_SERVER_CASE == 7 ? STYX_MOVIE_SERVER_FAILED :
            STYX_SERVER_CASE == 4 ? STYX_MOVIE_SERVER_STOPPED : STYX_MOVIE_SERVER_DONE; /* Fixture terminal policy. */
        check(styxbidirmovieserverwait(server, 5000) && styxbidirmovieserverstate(server) == expected,
            "autonomous worker publishes correct terminal reason");
        check(styxbidirmovieserverfree(server), "join and consume only server-owned worker");
        server = NULL;
        if (expected == STYX_MOVIE_SERVER_DONE || expected == STYX_MOVIE_SERVER_REFUSED)
        {
            general(route, 9, "done");
        }
    }
    if (STYX_SERVER_CASE == 8 || STYX_SERVER_CASE == 9)
    {
        check(styxbidirroutetypedwait(route, 0, 5000), "one-shot/failure leaves unconsumed resource request intact");
    }
    if (STYX_SERVER_CASE != 7)
    {
        check(!styxbidirroutestate(route) && styxbidirroutesendwait(route, 102, "alive", 5, 5000, NULL),
            "server retirement leaves independent shared TX usable");
        general(route, 9, "alive");
    }
    else
    {
        check(styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED, "uncertain prefix has no automatic replay");
    }
    styxbidirroutefree(route);
    check(styxclientalive(client), "original 9P session survives server and route retirement");
}
