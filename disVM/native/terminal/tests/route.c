/* Copyright (c) 2026 bhgv
 * EN: Automatic native UI routing over original 9P, including bounded mailbox shutdown.
 * RU: Автоматическая native-маршрутизация UI через исходный 9P и завершение полных очередей.
 */
#include "route/route.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_ROUTE_CASE
#define STYX_ROUTE_CASE 0 /* Normal complete request/reply fixture. */
#endif


/* EN/RU: Fail this native peer explicitly on a violated fixture contract.
 * Явно завершить native peer при нарушении проверяемого контракта.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "terminal route: %s\n", why);
        exit(2);
    }
}


/* EN: Borrow the real handoff through return, create and join only this route's native workers.
 * Compile-time variants exercise blocked cancellation, malformed UI, conflicting declarations
 * and a version mismatch. No Limbo poll/callback, fabricated channel or network socket is used.
 * RU: Заимствовать настоящий handoff, создать и join только native-потоки своего route.
 * Варианты проверяют отмену при backpressure, повреждение UI, конфликт и несовпадение версии.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Borrowed original launch authority. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    check(!styxbidirbindingroute(client, &channels, 256, 1, "", 1) &&
        !styxbidirbindingroute(client, &channels, 256, 1, "bad type", 1), "schema preflight");
    unsigned long version = STYX_ROUTE_CASE == 4 ? 2 : 1; /* Negotiated fixture version. */
    StyxBidirRoute *route = styxbidirbindingroute(client, &channels, 256, 1, "test.ide.fvd1", version);
    check(route != NULL, "owned native dispatcher");
    check(styxbidirroutewait(route, 0, 5000), "automatic declaration reception without a guest poll");
    DVM_UIMessage ui; /* Caller-owned message output and eventual reply. */
    memset(&ui, 0, sizeof(ui));
    check(styxbidirrouteui(route, &ui) == STYX_CONTROL_EMPTY,
        "full general mailbox backpressures dispatcher without dropping records");
    if (STYX_ROUTE_CASE == 1)
    {
        styxbidirroutestop(route);
        check(styxbidirroutestate(route) == STYX_CONTROL_STOPPED, "stop latches before join");
    }
    unsigned general = STYX_ROUTE_CASE == 1 ? 1 : STYX_ROUTE_CASE == 4 ? 4 : STYX_ROUTE_CASE == 5 ? 5 : 3;
    for (unsigned i = 0; i < general; i++)
    {
        unsigned char bytes[256]; /* Bounded copied general payload. */
        unsigned long tag = 999; /* Sentinel until an actual record is consumed. */
        size_t size = 999; /* Required/accepted byte count. */
        check(styxbidirroutewait(route, 0, 5000), "one general mailbox item ready");
        if (!i)
        {
            check(styxbidirroutenext(route, &tag, NULL, 0, &size) == STYX_CONTROL_SMALL && tag == 999,
                "small destination preserves general head and tag");
        }
        check(styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD,
            "one general record per read");
        check((i == 0 && tag == 0 && size > 0) ||
            (i == 1 && tag == 5 && size == 3 && bytes[1] == 128) ||
            (i == 2 && tag == 9 && size == 0) || (i == 3 && tag == 17 && size > 53) ||
            (i == 4 && tag == 0 && size > 0),
            "declaration, opaque and empty records remain ordered and visible");
    }
    if (STYX_ROUTE_CASE == 0 || STYX_ROUTE_CASE == 5 || STYX_ROUTE_CASE == 6)
    {
        check(styxbidirroutewait(route, 1, 5000), "automatic UI delivery after general queue drains");
        if (STYX_ROUTE_CASE == 6)
        {
            styxbidirroutestop(route);
            check(styxbidirroutestate(route) == STYX_CONTROL_STOPPED, "stop with a full UI mailbox");
        }
        check(styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.kind == DVM_UI_REQUEST &&
            ui.id == 42 && ui.first == -2147483647L - 1 && ui.second == 2147483647L &&
            ui.size == 4 && !memcmp(ui.text, "A\0\320\257", 4), "copied UI golden values");
        check(styxbidirrouteui(route, &ui) ==
            (STYX_ROUTE_CASE == 6 ? STYX_CONTROL_END : STYX_CONTROL_EMPTY), "one UI event per call");
        if (STYX_ROUTE_CASE == 6)
        {
            styxbidirroutefree(route);
            check(styxclientalive(client), "full UI mailbox stop joins only owned workers");
            return;
        }
        unsigned char declaration[] = {0x81, 0x80}; /* Independent TX control fixture. */
        unsigned char payload[240]; /* Independent TX binary fixture. */
        for (unsigned i = 0; i < sizeof(payload); i++)
        {
            payload[i] = (unsigned char)i;
        }
        check(styxbidirroutesend(route, 0, declaration, sizeof(declaration)) &&
            styxbidirrouteflush(route, 5000), "independent TX declaration");
        check(styxbidirroutesend(route, 101, payload, sizeof(payload)) &&
            styxbidirrouteflush(route, 5000), "independent TX opaque record");
        ui.kind = DVM_UI_REPLY;
        ui.id = 43;
        ui.first = 0;
        ui.second = 0;
        ui.size = 5;
        memcpy(ui.text, "reply", 5);
        check(!styxbidirrouteuisend(route, 0, &ui), "control tag cannot be UI output");
        check(styxbidirrouteuisend(route, 17, &ui) && styxbidirrouteflush(route, 5000),
            "UI reply acknowledged over original short-write stream");
    }
    else if (STYX_ROUTE_CASE == 2 || STYX_ROUTE_CASE == 3)
    {
        check(!styxbidirroutewait(route, 1, 5000) &&
            styxbidirroutestate(route) == STYX_CONTROL_PROTOCOL, "invalid routed input terminates atomically");
        check(styxbidirrouteui(route, &ui) == STYX_CONTROL_END, "invalid UI never published");
    }
    else if (STYX_ROUTE_CASE == 4)
    {
        check(styxbidirrouteui(route, &ui) == STYX_CONTROL_EMPTY, "another version stays general");
    }
    styxbidirroutestop(route);
    check(!styxbidirrouteuisend(route, 17, &ui), "stopped route refuses output");
    styxbidirroutefree(route);
    check(styxclientalive(client), "route teardown preserves shared bidir session");
}
