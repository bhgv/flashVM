/* Copyright (c) 2026 bhgv
 * EN: Original Styx stress of unread recipient/service queues, late bindings, exact accounting and NOMEM.
 * RU: Original Styx stress непрочитанных recipient/service queues, late bindings, точного учёта и NOMEM.
 */
#include "route/traffic.h"
#include "mount.h"
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


#ifdef STYX_PRESSURE_TEST


/* EN/RU: Configure and check native pressure diagnostics on this real route without consuming its messages.
 * Настроить и проверить native pressure diagnostics real route без consumption messages.
 */
extern void pressurestart(StyxBidirRoute *);


/* EN/RU: Validate automatic backlog notices/metrics before data drain; route remains pinned by main.
 * Проверить automatic backlog notices/metrics до data drain; route удерживается main.
 */
extern void pressurefilled(StyxBidirRoute *);


/* EN/RU: Validate recovery/counters after original exact FIFO drain, still before route disposal.
 * Проверить recovery/counters после original exact FIFO drain, до disposal route.
 */
extern void pressuredrained(StyxBidirRoute *);
#endif


/* EN/RU: Create owned real bidir client, native original server and isolated carrier.
 * Создать свой real bidir client, native original server и изолированный carrier.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Consume physical fixture after all client users retire, join original server.
 * Поглотить physical fixture после всех client users, join original server.
 */
extern void linkfixturefree(void);


/* EN/RU: Wait for actual final enqueue/failure, no consumption, guest polling or fabricated readiness.
 * Ждать настоящий final enqueue/failure, без consumption, guest polling либо fake readiness.
 */
extern void dynamiccomplete(void);


/* EN/RU: Explicit optimized-build fixture contract, diagnostic borrowed through return/exit.
 * Явный fixture contract в optimized build, diagnostic borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "dynamic recipient queue: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Consume exactly one expected general record; empty data and tag-zero bindings are ordinary copies.
 * Потребить ровно одну ожидаемую general record; tag0 bindings здесь лишь observation copies.
 */
static void general(StyxBidirRoute *route, unsigned long expected, const void *value, size_t length)
{
    unsigned char bytes[256]; /* Single copied body. */
    unsigned long tag; /* Exact original tag. */
    size_t size; /* Exact original body size. */
    check(styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected,
        "one ordered general record");
    if (expected)
    {
        check(size == length && (!size || (value && !memcmp(bytes, value, size))), "general payload oracle");
    }
}


/* EN/RU: Copy coherent metrics, require non-consuming snapshot and monotonic historical counts.
 * Копировать coherent metrics, проверить non-consuming snapshot и historical counts.
 */
static StyxRouteQueueInfo snapshot(StyxBidirRoute *route, unsigned kind, unsigned index, size_t records)
{
    StyxRouteQueueInfo info; /* Coherent copied output. */
    check(styxbidirroutequeueinfo(route, kind, index, &info) && info.records == records &&
        info.peak_records >= info.records && info.peak_bytes >= info.bytes, "coherent queue counters");
    return info;
}


/* EN/RU: One independent native consumer waiting only for its selected queue, output read after join.
 * Независимый native consumer ждёт лишь свою очередь, output читается после join.
 */
typedef struct RecipientWait
{
    StyxBidirRoute *route; /* Main pins owner until joined. */
    int service; /* Select native service inbox instead of late typed recipient. */
    int ready; /* Worker-only result before join. */
} RecipientWait;


/* EN/RU: Block this consumer on its queue condition; never consume data or own the shared transport reader.
 * Блокировать consumer на condition своей очереди; без consumption либо владения общим reader.
 */
static void *recipientwait(void *state)
{
    RecipientWait *wait = state; /* Borrowed main-owned context through join. */
    wait->ready = wait->service ? styxbidirrouteservicewait(wait->route, 120000) :
        styxbidirroutetypedwait(wait->route, 1, 120000);
    return NULL;
}


/* EN/RU: Own actual bidir route while leaving all recipients untouched until complete input admission.
 * Владеть actual bidir route, не трогая адресатов до завершения admission всего input.
 */
int main(void)
{
    StyxClient *client = linkfixture(); /* Extra caller pin independent of grant/control ownership. */
    check(client && styxclientretain(client), "native client pin");
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes retained client. */
    DVM_TerminalChannels channels = {0}; /* Actual pinned fixture root. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType types[] = {{"test.blocked", 1}, {"test.late", 1}}; /* Late binding destination initially unknown. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, 512, 1, "test.ide.fvd1", 1, types, 2);
    check(route && styxbidirroutewait(route, 1, 3000), "UI received while initial general records remain unread");
#ifdef STYX_PRESSURE_TEST
    pressurestart(route);
#endif
    RecipientWait waits[] = {{route, 1, -1}, {route, 0, -1}}; /* Independent service and late-client waits. */
    pthread_t workers[2]; /* Joined before route/users retire. */
    for (unsigned i = 0; i < 2; i++)
    {
        check(!pthread_create(&workers[i], NULL, recipientwait, &waits[i]), "independent blocking queue consumer");
    }
    check(styxbidirroutesendwait(route, 102, "fill", 4, 3000, NULL) && styxbidirrouteflush(route, 3000), "actual fill request");
    dynamiccomplete();
#ifdef STYX_PRESSURE_TEST
    pressurefilled(route);
#endif
    int failure = STYX_DYNAMIC_CASE >= 3; /* Actual allocator refusal, not a slow-recipient heuristic. */
    for (unsigned i = 0; i < 2; i++)
    {
        check(!pthread_join(workers[i], NULL), "independent queue waiter join");
    }
    check(waits[0].ready && waits[1].ready == !failure,
        "blocked service/client consumers wake for their own queues independently of unread backlog");
    unsigned selected = STYX_DYNAMIC_CASE % 3; /* General/UI/typed recipient intentionally unread throughout. */
    check(styxbidirroutestate(route) == (failure ? STYX_CONTROL_NOMEM : STYX_CONTROL_LIVE), "precise exhaustion outcome");
    StyxRouteQueueInfo sentinel = {9, 8, 7, 6}; /* Invalid query must leave all fields unchanged. */
    check(!styxbidirroutequeueinfo(route, STYX_ROUTE_TYPED, 2, &sentinel) && sentinel.records == 9 && sentinel.bytes == 8 &&
        !styxbidirroutequeueinfo(route, STYX_ROUTE_GENERAL, 1, &sentinel), "invalid stats preserve output");
    size_t extra = failure ? 0 : 1; /* Final general done is sent only after the marker allocation succeeds. */
    snapshot(route, STYX_ROUTE_GENERAL, 0, 3 + (selected == 0 ? 512 : 0) + extra);
    snapshot(route, STYX_ROUTE_UI, 0, selected == 1 ? 513 : 1);
    snapshot(route, STYX_ROUTE_TYPED, 0, selected == 2 ? 512 : 0);
    StyxRouteQueueInfo before = snapshot(route, STYX_ROUTE_SERVICE, 0, 64); /* Independent unread service backlog. */
    check(before.bytes == 256, "service bytes counted separately");
    size_t size = 99; /* SMALL sentinel. */
    check(styxbidirrouteservicenext(route, NULL, 0, &size) == STYX_CONTROL_SMALL && size == 4,
        "SMALL service read preserves head");
    snapshot(route, STYX_ROUTE_SERVICE, 0, 64);
    for (unsigned i = 0; i < 64; i++)
    {
        unsigned char bytes[4]; /* One exact opaque system body. */
        static const unsigned char expected[] = {0xaa, 0, 128, 255}; /* Existing integer codec version42. */
        check(styxbidirrouteservicenext(route, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
            size == sizeof(bytes) && !memcmp(bytes, expected, sizeof(bytes)), "one service record per call");
    }
    StyxRouteQueueInfo drained = snapshot(route, STYX_ROUTE_SERVICE, 0, 0); /* Historical metrics survive drain. */
    check(!drained.bytes && drained.peak_records == before.peak_records && drained.peak_bytes == before.peak_bytes,
        "drained current counters preserve peaks");
    unsigned long tag; /* One copied typed tag. */
    unsigned char bytes[4]; /* Typed marker and indexed payload. */
    if (!failure)
    {
        check(styxbidirroutetypednext(route, 1, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
            tag == 31 && size == 4 && !memcmp(bytes, "tail", 4), "late binding applied despite every unread backlog");
    }
    else
    {
        check(styxbidirroutetypednext(route, 1, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_END,
            "failed allocation never publishes a partial marker");
    }
    general(route, 0, NULL, 0);
    general(route, 5, "other", 5);
    DVM_UIMessage ui; /* Independent UI consumer copy. */
    check(styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42, "initial UI retained");
    for (unsigned i = 0; i < 512; i++)
    {
        unsigned char expected[] = {0, 0, (unsigned char)(i >> 8), (unsigned char)i}; /* Independent FIFO oracle. */
        if (!selected)
        {
            general(route, 9, expected, sizeof(expected));
        }
        else if (selected == 1)
        {
            check(styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42, "one queued UI item per call");
        }
        else
        {
            check(styxbidirroutetypednext(route, 0, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD &&
                tag == 23 && size == 4 && !memcmp(bytes, expected, size), "typed FIFO order and exact binary payload");
        }
    }
    general(route, 0, NULL, 0); /* Late declaration observation remains ordered in general queue. */
    if (!failure)
    {
        general(route, 9, "done", 4);
    }
    snapshot(route, STYX_ROUTE_GENERAL, 0, 0);
    snapshot(route, STYX_ROUTE_UI, 0, 0);
    snapshot(route, STYX_ROUTE_TYPED, 0, 0);
#ifdef STYX_PRESSURE_TEST
    pressuredrained(route);
#endif
    check(styxclientalive(client), "local allocation failure/queue drain does not close shared native client");
    styxbidirroutefree(route);
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    puts("PASS independent dynamic recipient and service queues");
    return 0;
}
