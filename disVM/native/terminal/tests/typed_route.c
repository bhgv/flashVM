/* Copyright (c) 2026 bhgv
 * EN: Real original-9P acceptance of independent opaque typed queues beside UI and general traffic.
 * RU: Проверки независимых типизированных очередей рядом с UI/general через настоящий исходный 9P.
 */
#include "route/types.h"
#include "route/launch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#ifndef STYX_TYPED_CASE
#define STYX_TYPED_CASE 0 /* Ordinary mixed session. */
#endif


/* EN/RU: Report an explicit contract failure in every build profile; why borrowed through return.
 * Явно сообщить нарушение контракта в любом build profile, why заимствован до возврата.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "typed terminal route: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Read one expected general record; borrow route/expected, preserve independent queue ownership.
 * Прочитать одну ожидаемую общую запись, route/expected заимствованы, остальные очереди не читать.
 */
static void general(StyxBidirRoute *route, unsigned long expected_tag, const void *expected, size_t length)
{
    unsigned char bytes[512]; /* Caller-owned record copy. */
    unsigned long tag = 999; /* Unchanged until successful record consumption. */
    size_t size = 999; /* Output size sentinel. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
        tag == expected_tag, "one expected general record");
    if (expected_tag)
    {
        check(size == length && (!size || (expected && !memcmp(bytes, expected, size))), "general payload unchanged");
    }
    else
    {
        check(size > 0, "binding declaration remains visible");
    }
}


/* EN/RU: Read one exact typed payload; SMALL preserves head/tag/bytes, including binary NUL data.
 * Прочитать один точный payload; SMALL сохраняет head/tag/bytes, включая binary NUL.
 */
static void typed(StyxBidirRoute *route, unsigned index, unsigned long expected_tag,
    const void *expected, size_t length)
{
    unsigned char bytes[16]; /* Distinct caller-owned output storage. */
    memset(bytes, 77, sizeof(bytes));
    unsigned long tag = 999; /* Sentinel verifies SMALL does not reveal/consume tag. */
    size_t size = 999; /* Exact required length on SMALL. */
    check(styxbidirroutetypedwait(route, index, 5000), "native typed readiness");
    if (length)
    {
        check(styxbidirroutetypednext(route, index, &tag, bytes, length - 1, &size) == STYX_CONTROL_SMALL &&
            size == length && tag == 999 && bytes[0] == 77, "small destination preserves typed head");
    }
    check(styxbidirroutetypednext(route, index, &tag, length ? bytes : NULL, length, &size) ==
        STYX_CONTROL_RECORD && tag == expected_tag && size == length &&
        (!length || !memcmp(bytes, expected, length)), "one exact typed record, empty is not EOF");
}


/* EN/RU: One native consumer for one borrowed typed mailbox; route remains owned until join.
 * Один native-потребитель заимствованной типизированной очереди, route живёт до join.
 */
typedef struct Consumer
{
    StyxBidirRoute *route; /* Borrowed through worker join. */
    unsigned index; /* Stable typed queue index, zero for opaque and seven for file. */
} Consumer;


/* EN/RU: Consume exact FIFO on a separate native thread; no VM/guest or other queue access.
 * Прочитать точную FIFO отдельным native-потоком, без VM/guest и доступа к другой очереди.
 */
static void *consume(void *opaque)
{
    Consumer *consumer = opaque; /* Host stack state pinned through join. */
    if (consumer->index == 0)
    {
        static const unsigned char binary[] = {0, 128, 255}; /* Exact independent payload. */
        typed(consumer->route, 0, 5, binary, sizeof(binary));
        typed(consumer->route, 0, 6, NULL, 0);
        typed(consumer->route, 0, 5, "tail", 4);
    }
    else
    {
        typed(consumer->route, 7, 23, "part", 4);
        typed(consumer->route, 7, 23, "next", 4);
    }
    return NULL;
}


/* EN/RU: Check rejected configurations before a usable client/grant is supplied; no worker ownership.
 * Проверить отказ до выдачи client/grant, без владения workers; count/type/version bounded.
 */
static void preflight(void)
{
    StyxRouteType duplicates[] = {{"test.file", 1}, {"test.file", 1}}; /* Ambiguous destinations. */
    StyxRouteType bad = {"bad type", 1}; /* Forbidden whitespace. */
    check(!styxbidirbindingroutes(NULL, NULL, 256, 2, NULL, 0, duplicates, 2) &&
        !styxbidirbindingroutes(NULL, NULL, 256, 2, "test.file", 1, duplicates, 1) &&
        !styxbidirbindingroutes(NULL, NULL, 256, 2, NULL, 0, &bad, 1) &&
        !styxbidirbindingroutes(NULL, NULL, 256, 2, NULL, 1, NULL, 0) &&
        !styxbidirbindingroutes(NULL, NULL, 256, 2, NULL, 0, NULL, 1) &&
        !styxbidirbindingroutes(NULL, NULL, 256, 2, NULL, 0, duplicates, STYX_ROUTE_TYPES_MAX + 1),
        "reject ambiguous/invalid typed setup before I/O");
}


/* EN/RU: Consume tag5 from the live fixture route as a provider launch request.
 * Принять tag5 из живого fixture route как запрос запуска у провайдера. */
static void launch_record(StyxBidirRoute *route)
{
    DVM_LaunchSpec spec = {0};
    spec.policy = DVM_LAUNCH_TERMINAL_FIRST;
    spec.terminal_origin.role = DVM_ORIGIN_TERMINAL;
    memset(spec.terminal_origin.system, 'A', DVM_ORIGIN_ID_BYTES - 1);
    memset(spec.terminal_origin.export_id, 'E', DVM_ORIGIN_ID_BYTES - 1);
    memset(spec.terminal_origin.session, 'I', DVM_ORIGIN_ID_BYTES - 1);
    memset(spec.expected_provider, 'M', DVM_ORIGIN_ID_BYTES - 1);
    strcpy(spec.module, "fixture/console.dis");
    StyxTerminalPaths paths = {"records", "records", NULL};
    DVM_LaunchService *service = dvm_launch_service_create(spec.expected_provider, 1);
    check(service != NULL, "launch service");
    DVM_LaunchEndpoint *endpoint = dvm_launch_service_endpoint(service);
    check(endpoint != NULL, "launch endpoint");
    unsigned char bytes[3];
    size_t size = 0;
    DVM_LaunchTicket *ticket = NULL;
    check(styxbidirroutetypedwait(route, 0, 5000), "live launch record readiness");
    check(styxbidirroutelaunchnext(route, 0, &spec, FVM_ACCESS_MASK, &paths, endpoint, 5,
        bytes, 2, &size, &ticket) == STYX_CONTROL_SMALL && size == sizeof(bytes) && !ticket,
        "small launch buffer preserves record");
    check(styxbidirroutelaunchnext(route, 0, &spec, FVM_ACCESS_MASK, &paths, endpoint, 5,
        bytes, sizeof(bytes), &size, &ticket) == STYX_CONTROL_RECORD && ticket &&
        dvm_launch_ticket_status(ticket) == DVM_LAUNCH_PENDING, "live record is queued");
    DVM_LaunchReview *review = dvm_launch_service_next(service);
    check(review != NULL, "launch review");
    check(dvm_launch_arguments(dvm_launch_review_request(review), 0, bytes, sizeof(bytes), &size) &&
        size == sizeof(bytes) && bytes[0] == 0 && bytes[1] == 128 && bytes[2] == 255,
        "provider receives exact live bytes");
    dvm_launch_review_free(review);
    dvm_launch_ticket_free(ticket);
    dvm_launch_endpoint_free(endpoint);
    dvm_launch_service_free(service);
}


/* EN/RU: Borrow real launch grant, exercise native-only independent routing and bounded shutdown.
 * Заимствовать настоящий launch grant, проверить native-маршрутизацию и ограниченное завершение.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    preflight();
    DVM_TerminalChannels channels = *descriptor; /* Borrow original bidir authority. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    char opaque[] = "test.opaque"; /* Constructor must copy this identity before publishing worker. */
    StyxRouteType types[] = { /* Exercise maximum registration count and stable index order. */
        {opaque, 1}, {"unused.1", 1}, {"unused.2", 1}, {"unused.3", 1},
        {"unused.4", 1}, {"unused.5", 1}, {"unused.6", 1},
        {"test.file", STYX_TYPED_CASE == 4 ? 3 : 1}
    };
    int no_ui = STYX_TYPED_CASE == 2; /* Data-only session must not require display authority. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 512,
        STYX_TYPED_CASE == 3 ? 1 : 2, no_ui ? NULL : "test.ide.fvd1", no_ui ? 0 : 1,
        types, sizeof(types) / sizeof(types[0]));
    check(route != NULL, "own native mixed dispatcher");
    memset(opaque, 'x', sizeof(opaque) - 1);
    unsigned char output = 77; /* Invalid-call sentinel, never exposed to dispatcher. */
    unsigned long tag = 999; /* Invalid-call tag sentinel. */
    size_t size = 999; /* Invalid-call size sentinel. */
    check(styxbidirroutetypednext(route, 8, &tag, &output, 1, &size) == STYX_CONTROL_END &&
        tag == 999 && size == 999 && output == 77 &&
        !styxbidirroutetypedwait(route, 8, 0) && !styxbidirroutetypedwait(route, 0, 120001) &&
        !styxbidirroutetypedwait(route, 1, 0), "invalid and empty queues preserve outputs");
    general(route, 0, NULL, 0);
    pthread_t workers[2]; /* Optional independently joined native mailbox consumers. */
    Consumer consumers[] = {{route, 0}, {route, 7}}; /* Stack state retained through both joins. */
    if (STYX_TYPED_CASE == 6)
    {
        check(!pthread_create(&workers[0], NULL, consume, &consumers[0]) &&
            !pthread_create(&workers[1], NULL, consume, &consumers[1]), "independent native consumers");
    }
    static const unsigned char binary[] = {0, 128, 255}; /* Independent exact RX bytes. */
    if (STYX_TYPED_CASE == 3)
    {
        check(styxbidirroutetypedwait(route, 0, 5000), "full one-slot typed queue");
        general(route, 9, NULL, 0);
        styxbidirroutestop(route);
        check(styxbidirroutetypedwait(route, 0, 0), "accepted typed head survives stop");
        typed(route, 0, 5, binary, sizeof(binary));
        typed(route, 0, 6, NULL, 0);
        typed(route, 0, 5, "tail", 4);
        check(styxbidirroutetypednext(route, 0, &tag, &output, 1, &size) == STYX_CONTROL_END,
            "all accumulated records drain after stop");
        styxbidirroutefree(route);
        check(styxclientalive(client), "stop with full typed queue preserves original session");
        return;
    }
    if (STYX_TYPED_CASE == 1 || STYX_TYPED_CASE == 4)
    {
        general(route, 23, "part", 4);
    }
    if (STYX_TYPED_CASE != 6)
    {
        if (STYX_TYPED_CASE == 0)
        {
            launch_record(route);
        }
        else
        {
            typed(route, 0, 5, binary, sizeof(binary));
        }
        typed(route, 0, 6, NULL, 0);
    }
    DVM_UIMessage ui; /* Private UI copy, independent from typed/raw FIFOs. */
    if (no_ui)
    {
        static const char golden[] = "fvd1:000000010000002a0000004d00000000800000007fffffffA\0\320\257";
        general(route, 17, golden, sizeof(golden) - 1);
        check(styxbidirrouteui(route, &ui) == STYX_CONTROL_EMPTY, "data-only route does not decode UI");
    }
    else
    {
        check(styxbidirroutewait(route, 1, 5000) &&
            styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42 && ui.size == 4,
            "UI keeps its own validated mailbox beside opaque data");
    }
    if (STYX_TYPED_CASE == 1)
    {
        general(route, 0, NULL, 0);
    }
    if (STYX_TYPED_CASE != 4 && STYX_TYPED_CASE != 1 && STYX_TYPED_CASE != 6)
    {
        typed(route, 7, 23, "part", 4);
    }
    if (STYX_TYPED_CASE == 5)
    {
        check(!styxbidirroutetypedwait(route, 7, 5000) &&
            styxbidirroutestate(route) == STYX_CONTROL_PROTOCOL, "conflict stops without reclassifying accepted queues");
        styxbidirroutefree(route);
        check(styxclientalive(client), "malformed declarations preserve shared session");
        return;
    }
    if (STYX_TYPED_CASE == 4)
    {
        general(route, 23, "next", 4);
        check(!styxbidirroutetypedwait(route, 7, 0), "unsupported version stays general");
    }
    else if (STYX_TYPED_CASE != 6)
    {
        typed(route, 7, 23, "next", 4);
    }
    if (STYX_TYPED_CASE != 6)
    {
        typed(route, 0, 5, "tail", 4);
    }
    general(route, 9, NULL, 0);
    general(route, 24, "vtwo", 4);
    if (STYX_TYPED_CASE == 6)
    {
        check(!pthread_join(workers[0], NULL) && !pthread_join(workers[1], NULL),
            "independent consumers complete before route retirement");
    }
    static const unsigned char sent[] = {0, 1, 128, 255, 0}; /* Independently granted TX tag, not RX-derived. */
    check(styxbidirroutesend(route, 101, sent, sizeof(sent)) && styxbidirrouteflush(route, 5000),
        "outgoing binary also uses one complete S-record");
    general(route, 9, "done", 4);
    styxbidirroutefree(route);
    check(styxclientalive(client), "typed teardown preserves original bidir session");
}
