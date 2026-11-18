/* Copyright (c) 2026 bhgv
 * EN: One existing resource reply over typed Unibus records beside live UI/general 9P traffic.
 * RU: Ответ ресурса существующего формата в typed Unibus рядом с живым UI/general 9P-трафиком.
 */
#include "route/types.h"
#include "reply.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_REPLY_ROUTE_TEST
#define STYX_REPLY_ROUTE_TEST 0 /* Successful staging and explicit END. */
#endif


/* EN/RU: Native staging sink, exclusively used on the calling fixture owner, never a guest pointer.
 * Native staging sink только текущего fixture owner, без указателей guest.
 */
typedef struct Sink
{
    unsigned calls; /* Actual data callbacks. */
    size_t size; /* Accepted staging bytes. */
    unsigned char bytes[16]; /* Bounded independent collector. */
} Sink;


/* EN/RU: Fail explicitly on a violated fixture contract, borrowing why through return.
 * Явно завершить fixture при нарушении контракта, why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "Unibus resource reply: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Copy exact borrowed data into native staging or inject one sink refusal, no replay.
 * Копировать точные bytes в native staging либо вернуть заданный отказ, без replay.
 */
static FVM_Result staged(const void *bytes, size_t size, void *opaque)
{
    Sink *sink = opaque; /* Caller-owned collector. */
    sink->calls++;
    if (STYX_REPLY_ROUTE_TEST == 2)
    {
        return FVM_INVALID_ARGUMENT;
    }
    check(size <= sizeof(sink->bytes) - sink->size, "bounded sink");
    memcpy(sink->bytes + sink->size, bytes, size);
    sink->size += size;
    return FVM_OK;
}


/* EN/RU: Consume one exact general item; a declaration must remain visible without feeding the sink.
 * Прочитать одну общую запись, объявление остаётся видимым без передачи в sink.
 */
static void general(StyxBidirRoute *route, unsigned long expected_tag, const char *expected)
{
    unsigned char bytes[512]; /* Private copied general body. */
    unsigned long tag = 999; /* Published only on actual consumption. */
    size_t size = 0; /* Exact body size. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected_tag && (expected ? size == strlen(expected) && !memcmp(bytes, expected, size) : size > 0),
        "general traffic stays independent");
}


/* EN/RU: Feed one copied typed body to the shared reply validator; stop feeding after terminal result.
 * Передать одну копию typed body общей проверке ответа; после terminal result больше не передавать.
 */
static void next(StyxBidirRoute *route, MovieReply *reply, int *status)
{
    unsigned char bytes[MOVIE_CHUNK + 1]; /* One bounded copied record body. */
    unsigned long tag = 0; /* Original numeric RX tag, never used as a TX grant. */
    size_t size = 0; /* Exact record body length. */
    check(styxbidirroutetypedwait(route, 0, 5000) &&
        styxbidirroutetypednext(route, 0, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == 23,
        "one resource body on its registered type queue");
    if (*status == MOVIE_REPLY_MORE)
    {
        *status = movie_reply_accept(reply, bytes, size);
    }
}


/* EN/RU: Borrow actual launch grant, validate one resource stream without interpreting UI/opaque traffic.
 * Заимствовать настоящий launch grant, проверить один resource stream отдельно от UI/opaque traffic.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Borrow exact original bidir grant. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType schema = {MOVIE_TYPE, MOVIE_VERSION}; /* Existing schema, no new wire identity. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 512, 2,
        "test.ide.fvd1", 1, &schema, 1);
    check(route != NULL, "owned native route");
    Sink sink = {0}; /* Native collector outlives the synchronous validator. */
    MovieReply reply; /* One active request, no inferred concurrent-request correlation. */
    char resolved[64] = "unchanged"; /* Published only after successful END. */
    check(movie_reply_init(&reply, staged, &sink, resolved, sizeof(resolved)), "reply state");
    general(route, 0, NULL);
    general(route, 5, "other");
    int status = MOVIE_REPLY_MORE; /* Feed while this resource has no terminal result. */
    next(route, &reply, &status);
    next(route, &reply, &status);
    DVM_UIMessage ui; /* UI remains in its own checked queue. */
    check(styxbidirroutewait(route, 1, 5000) &&
        styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42 && ui.size == 4,
        "UI survives resource transfer/refusal");
    next(route, &reply, &status);
    if (STYX_REPLY_ROUTE_TEST != 3)
    {
        next(route, &reply, &status);
    }
    general(route, 9, "ready");
    if (STYX_REPLY_ROUTE_TEST == 0)
    {
        static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent binary oracle. */
        check(status == MOVIE_REPLY_DONE && movie_reply_result(&reply) == FVM_OK && sink.calls == 2 &&
            sink.size == sizeof(expected) && !memcmp(sink.bytes, expected, sizeof(expected)) &&
            !strcmp(resolved, "/vfs/a.swf"), "successful END alone publishes staged resource origin");
    }
    else
    {
        check(movie_reply_result(&reply) != FVM_OK && !strcmp(resolved, "unchanged") &&
            (STYX_REPLY_ROUTE_TEST == 3 ? status == MOVIE_REPLY_MORE : status == MOVIE_REPLY_FAILED),
            "failed or unfinished acquisition never publishes success/origin");
        if (STYX_REPLY_ROUTE_TEST == 2)
        {
            check(sink.calls == 1 && !sink.size && movie_reply_result(&reply) == FVM_INVALID_ARGUMENT,
                "sink failure is not retried");
        }
        if (STYX_REPLY_ROUTE_TEST == 4)
        {
            check(!sink.calls, "malformed origin refuses before any sink callback");
        }
    }
    static const unsigned char sent[] = {0, 1, 128, 255, 0}; /* Independently established outgoing fixture. */
    check(styxbidirroutesend(route, 101, sent, sizeof(sent)) && styxbidirrouteflush(route, 5000),
        "unrelated outgoing data stays S-framed");
    general(route, 9, "done");
    styxbidirroutefree(route);
    check(styxclientalive(client), "resource receiver retirement preserves original session");
}
