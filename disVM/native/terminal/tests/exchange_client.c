/* Copyright (c) 2026 bhgv
 * EN: Real one-shot resource request/reply exchange, including isolated native cancellation and timeout.
 * RU: Настоящий одноразовый resource exchange, включая изолированную native-отмену и timeout.
 */
#include "resource/client.h"
#include "protocol.h"
#include "cancel.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_EXCHANGE_CASE
#define STYX_EXCHANGE_CASE 0 /* Successful executable resource intent. */
#endif
#if STYX_EXCHANGE_CASE == 9
static unsigned tx_attempts; /* Owner-only deterministic queue admission failure count. */


/* EN/RU: Original send is borrowed only for unrelated traffic, never for the injected refusal.
 * Исходный send используется только для другого трафика, не для заданного отказа.
 */
extern int __real_styxbidirroutesend(StyxBidirRoute *, unsigned long, const void *, size_t);


/* EN/RU: Refuse the resource request before queue acceptance; count attempts to detect automatic replay.
 * Отклонить resource request до принятия очередью, считать попытки для обнаружения replay.
 */
int __wrap_styxbidirroutesend(StyxBidirRoute *route, unsigned long tag, const void *bytes, size_t size)
{
    if (tag == 101)
    {
        tx_attempts++;
        return 0;
    }
    return __real_styxbidirroutesend(route, tag, bytes, size);
}
#endif


/* EN/RU: Host-owned context, pinned through both synchronous fetch and independent native consumer join.
 * Host-состояние живёт до конца fetch и join независимого native-потребителя.
 */
typedef struct Exchange
{
    StyxBidirRoute *route; /* Borrowed through final client and consumer retirement. */
    StyxBidirMovieClient *client; /* Owned one-shot resource authority. */
    DVM_Cancel *outer; /* Optional host-owned outer cancellation scope source. */
    pthread_t owner; /* Only this native owner may invoke the staging callback. */
    unsigned calls; /* Owner-only data callbacks, no replays. */
    size_t size; /* Owner-only collected bytes. */
    unsigned char bytes[16]; /* Bounded owner-only staging. */
} Exchange;


/* EN/RU: Report an explicit fixture invariant before proceeding with a broken exchange.
 * Явно сообщить нарушение fixture invariant до продолжения ошибочного обмена.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "Unibus exchange: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Stage copied bytes only on the original caller, with native cancellation scope propagated.
 * Сохранить копию bytes только на исходном caller, с переданным native cancellation scope.
 */
static FVM_Result staging(const void *bytes, size_t size, void *state)
{
    Exchange *test = state; /* Host-owned native staging state. */
    check(pthread_equal(pthread_self(), test->owner) && dvm_cancel_current(), "sink stays on its native owner/scope");
    test->calls++;
    if (STYX_EXCHANGE_CASE == 6)
    {
        return FVM_INVALID_ARGUMENT;
    }
    check(size <= sizeof(test->bytes) - test->size, "bounded staging");
    memcpy(test->bytes + test->size, bytes, size);
    test->size += size;
    return FVM_OK;
}


/* EN/RU: One independent native consumer for general records, borrowing test through thread join.
 * Независимый native-потребитель общей очереди, test заимствован до join.
 */
static void general(Exchange *test, unsigned long expected_tag, const char *expected)
{
    unsigned char bytes[512]; /* One general record copy. */
    unsigned long tag = 0; /* Exact incoming tag after successful consume. */
    size_t size = 0; /* Exact payload size. */
    check(styxbidirroutewait(test->route, 0, 5000) &&
        styxbidirroutenext(test->route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected_tag && (expected ? size == strlen(expected) && !memcmp(bytes, expected, size) : size > 0),
        "independent general traffic and native checkpoint");
}


/* EN/RU: Service UI/general traffic without touching the resource RX queue, then trigger native policy.
 * Обслужить UI/general, не касаясь resource RX queue, затем выполнить native policy fixture.
 */
static void *consume(void *state)
{
    Exchange *test = state; /* Main-owned context retained through join. */
    general(test, 0, NULL);
    general(test, 5, "other");
    DVM_UIMessage ui; /* Independent copied UI value. */
    check(styxbidirroutewait(test->route, 1, 5000) &&
        styxbidirrouteui(test->route, &ui) == STYX_CONTROL_RECORD && ui.id == 42, "UI beside active resource request");
    if (STYX_EXCHANGE_CASE == 3 || STYX_EXCHANGE_CASE == 9 || STYX_EXCHANGE_CASE == 10)
    {
        return NULL;
    }
    general(test, 9, "request");
    if (STYX_EXCHANGE_CASE == 4)
    {
        styxbidirmovieclientstop(test->client);
    }
    if (STYX_EXCHANGE_CASE == 8)
    {
        dvm_cancel_request(test->outer);
    }
    if (STYX_EXCHANGE_CASE == 7)
    {
        char resolved[64] = "contender"; /* Must remain unchanged when same-client concurrent request refuses. */
        check(styxbidirmoviefetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging,
            test, resolved, sizeof(resolved), test->client) == FVM_ERROR && !strcmp(resolved, "contender"),
            "second concurrent caller cannot send or steal replies");
        check(styxbidirroutesend(test->route, 102, "next", 4) && styxbidirrouteflush(test->route, 5000),
            "fixture gate releases response only after contender refuses");
    }
    return NULL;
}


/* EN/RU: Request cancellation from an independent native thread; source pinned by host through join.
 * Запросить отмену отдельным native-потоком, host удерживает source до join.
 */
static void *cancelwait(void *source)
{
    dvm_cancel_request(source);
    return NULL;
}


/* EN/RU: Race sticky cancellation with subscription and native wait; no polling, sleeps or route stop.
 * Проверить гонку sticky cancel с подпиской/ожиданием, без polling, sleep и остановки route.
 */
static void waitraces(StyxBidirRoute *route)
{
    for (unsigned i = 0; i < 16; i++)
    {
        DVM_Cancel *source = dvm_cancel_create(); /* Host pin through cancellation thread join. */
        pthread_t worker; /* Native producer, no guest context. */
        check(source && !pthread_create(&worker, NULL, cancelwait, source), "cancel producer");
        check(!styxbidirroutetypedwaitcancel(route, 1, 120000, source), "cancel wakes only native typed wait");
        check(!pthread_join(worker, NULL), "native cancel producer retired");
        dvm_cancel_release(source);
        check(!styxbidirroutestate(route), "cancelled wait leaves shared route live");
    }
}


/* EN/RU: Use the actual bidir launch grant; run one exchange with separate native UI/general handling.
 * Использовать настоящий bidir launch grant, один exchange с отдельным native UI/general handling.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Original trusted grant, never inferred from a filename. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType types[] = {{MOVIE_TYPE, MOVIE_VERSION}, {"test.unused", 1}}; /* Independent typed consumers. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 16384, 2, "test.ide.fvd1", 1, types, 2);
    check(route && !styxbidirmovieclient(route, 1, 101, 5000) && !styxbidirmovieclient(route, 0, 0, 5000) &&
        !styxbidirmovieclient(route, 0, 101, 0) && !styxbidirmovieclient(route, 0, 101, 120001),
        "preflight enforces registered resource schema and explicit TX/budget");
    Exchange test = {0}; /* Main-owned context used by synchronous sink and joined native consumer. */
    test.route = route;
    test.owner = pthread_self();
    test.client = styxbidirmovieclient(route, 0, 101, STYX_EXCHANGE_CASE == 5 ? 500 : 5000);
    check(test.client != NULL, "owned one-exchange client");
    char resolved[64] = "unchanged"; /* Caller output must not publish before successful completion. */
    check(styxbidirmoviefetch(99, "/ok.swf", "/base.swf", staging, &test, resolved, sizeof(resolved), test.client) ==
        FVM_INVALID_ARGUMENT, "invalid intent does not consume one-shot admission");
    DVM_CancelScope scope = {0}; /* Optional external native scope, restored after fetch. */
    if (STYX_EXCHANGE_CASE == 8)
    {
        test.outer = dvm_cancel_create();
        check(test.outer && dvm_cancel_enter(&scope, test.outer), "outer native scope");
    }
    if (STYX_EXCHANGE_CASE == 3)
    {
        styxbidirmovieclientstop(test.client);
    }
    if (STYX_EXCHANGE_CASE == 10)
    {
        check(styxbidirroutetypedwait(route, 0, 5000), "prequeued stale reply fixture");
    }
    pthread_t consumer; /* UI/general stays serviced while fetch blocks natively. */
    check(!pthread_create(&consumer, NULL, consume, &test), "independent native UI/general consumer");
    FVM_Result result = styxbidirmoviefetch(STYX_EXCHANGE_CASE == 1 ? FVM_DESKTOP_FETCH_DATA : FVM_DESKTOP_FETCH_CODE,
        STYX_EXCHANGE_CASE == 2 ? "/denied.swf" : "/ok.swf", "/base.swf", staging, &test,
        resolved, sizeof(resolved), test.client);
    check(!pthread_join(consumer, NULL), "consumer retired without touching resource queue");
    check(dvm_cancel_current() == test.outer, "native fetch restored the caller scope");
    if (STYX_EXCHANGE_CASE == 0 || STYX_EXCHANGE_CASE == 1 || STYX_EXCHANGE_CASE == 7)
    {
        static const unsigned char expected[] = {0, 128, 255, 0, 'x'}; /* Independent native VFS content oracle. */
        check(result == FVM_OK && test.calls == 1 && test.size == sizeof(expected) &&
            !memcmp(test.bytes, expected, sizeof(expected)) && !strcmp(resolved, "/ok.swf"),
            "real VFS resource reached original staging owner after explicit END");
    }
    else
    {
        check(result != FVM_OK && !strcmp(resolved, "unchanged") &&
            (STYX_EXCHANGE_CASE == 6 ? test.calls == 1 && result == FVM_INVALID_ARGUMENT : !test.calls),
            "refused/cancelled/unfinished resource does not publish success or retry sink");
    }
    strcpy(resolved, "second");
    check(styxbidirmoviefetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging, &test,
        resolved, sizeof(resolved), test.client) == FVM_ERROR && !strcmp(resolved, "second"),
        "completed or failed one-shot client cannot be reused");
#if STYX_EXCHANGE_CASE == 9
    check(tx_attempts == 1, "TX admission refusal is not automatically retried");
#endif
    if (STYX_EXCHANGE_CASE == 10)
    {
        check(styxbidirroutetypedwait(route, 0, 0), "stale reply was refused without consumption");
    }
    if (STYX_EXCHANGE_CASE == 4 || STYX_EXCHANGE_CASE == 5 || STYX_EXCHANGE_CASE == 8)
    {
        check(styxbidirroutesend(route, 102, "next", 4) && styxbidirrouteflush(route, 5000) &&
            styxbidirroutetypedwait(route, 0, 5000), "late fixture reply stays queued after cancelled exchange");
        check(styxbidirmoviefetch(FVM_DESKTOP_FETCH_CODE, "/ok.swf", "/base.swf", staging, &test,
            resolved, sizeof(resolved), test.client) == FVM_ERROR && !test.calls, "late reply is not a new request's data");
    }
    if (scope.cancel)
    {
        check(dvm_cancel_leave(&scope), "leave original caller scope");
        dvm_cancel_release(test.outer);
    }
    styxbidirmovieclientfree(test.client);
    waitraces(route);
    styxbidirroutefree(route);
    check(styxclientalive(client), "resource exchange and cancelled waits preserve original session");
}
