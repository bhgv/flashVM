/* Copyright (c) 2026 bhgv
 * EN: Real client shutdown/carrier loss must retire full RX and route queues without consuming their records.
 * RU: Настоящий client shutdown/carrier loss завершает full RX и route queues без потребления records.
 */
#include "../../route/internal.h"
#include "../../internal.h"
#include "../../resource/announced.h"
#include "../../resource/server.h"
#include "mount.h"
#include "announcement.h"
#include <stdio.h>
static pthread_mutex_t notice_lock = PTHREAD_MUTEX_INITIALIZER; /* Retained until all callbacks and owners retire. */
static pthread_cond_t notice_changed = PTHREAD_COND_INITIALIZER; /* Non-consuming native retirement checkpoint. */
static int noticed; /* Sticky callback observed, protected by notice_lock. */


/* EN/RU: Create owned original server/client fixture; NULL means setup failure.
 * Создать свой original server/client fixture; NULL означает setup failure.
 */
extern StyxClient *linkfixture(void);


/* EN/RU: Break the actual native carrier without touching control APIs, fixture remains owned.
 * Разорвать настоящий native carrier без control API, fixture остаётся owned.
 */
extern void linkfixturebreak(void);


/* EN/RU: Consume physical fixture after all fids/clients/controls retire, joins server.
 * Поглотить physical fixture после всех fids/clients/controls, join server.
 */
extern void linkfixturefree(void);


/* EN/RU: Wait for actual RX capacity blockage, no guest action or queue consumption.
 * Ждать настоящую RX capacity blockage, без guest action либо queue consumption.
 */
extern void linkblocked(void);


/* EN/RU: Reset startup capacity checkpoints before new fixture input, no queue consumption.
 * Сбросить startup capacity checkpoints до новых fixture records, без queue consumption.
 */
extern void linkreset(void);


/* EN/RU: Arm one synchronous watch behavior; mode1 refusal, 2 sticky shutdown, 3 delayed callback.
 * Включить один watch behavior: mode1 отказ, 2 sticky shutdown, 3 delayed callback.
 */
extern void linkwatcharm(int);


/* EN/RU: Finish post-free callback test; 1 verified, 0 missing lifecycle checkpoint.
 * Завершить post-free callback test; 1 verified, 0 отсутствует lifecycle checkpoint.
 */
extern int linklate(void);


/* EN/RU: Own actual prepared VFS service namespace/NULL for independent native resource server.
 * Свой actual prepared VFS service namespace/NULL для независимого native resource server.
 */
extern DVM_Namespace *exchangegroup(void);


/* EN/RU: Explicit test contract in optimized builds; why borrowed through return/exit.
 * Явный test contract в optimized builds; why borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "control link retirement: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Native nonwaiting retirement notice, only signal fixture-owned condition, no owner access.
 * Native nonwaiting retirement notice, только signal condition fixture, без доступа к owner.
 */
static void retired(void *unused)
{
    (void)unused;
    pthread_mutex_lock(&notice_lock);
    noticed = 1;
    pthread_cond_broadcast(&notice_changed);
    pthread_mutex_unlock(&notice_lock);
}


/* EN/RU: Require retirement within 2s while all filled queues remain untouched; no polling.
 * Требовать retirement за 2s, не трогая заполненные очереди, без polling.
 */
static void waitretired(void)
{
    struct timespec deadline; /* One bounded native condition deadline. */
    check(!clock_gettime(CLOCK_REALTIME, &deadline), "native notice clock");
    deadline.tv_sec += 2;
    pthread_mutex_lock(&notice_lock);
    while (!noticed)
    {
        if (pthread_cond_timedwait(&notice_changed, &notice_lock, &deadline))
        {
            break;
        }
    }
    int ended = noticed; /* Do not retain lock through fatal diagnostics. */
    pthread_mutex_unlock(&notice_lock);
    check(ended, "client terminal must retire full RX without another RPC or queue drain");
}


/* EN/RU: Drain one initial general record, route borrowed through synchronous call.
 * Прочитать одну initial general record, route borrowed до return.
 */
static void general(StyxBidirRoute *route, unsigned long expected)
{
    unsigned char bytes[256]; /* One complete copied record. */
    unsigned long tag; /* Copied original tag. */
    size_t size; /* Copied body extent. */
    check(styxbidirroutewait(route, 0, 3000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected,
        "initial general record");
}


/* EN/RU: Choose raw RX or filled general/UI/typed route; return owned route or NULL for raw transport.
 * Выбрать raw RX либо full general/UI/typed route; свой route либо NULL для raw transport.
 */
static StyxBidirRoute *transport(StyxClient *client, const DVM_TerminalChannels *channels,
    int scenario, StyxBidirControl **control)
{
    if (scenario < 2 || scenario >= 8)
    {
        *control = styxbidircontrol(client, channels, MOVIE_FRAME + 32, 1);
        return NULL;
    }
    StyxRouteType types[] = { /* Independent native resource queues behind the blocked dispatcher. */
        {"test.blocked", 1}, {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}, {MOVIE_TYPE, MOVIE_VERSION}
    };
    StyxBidirRoute *route = styxbidirbindingroutes(client, channels, MOVIE_FRAME + 32, 1,
        "test.ide.fvd1", 1, types, 3); /* Own dispatcher and underlying control. */
    check(route != NULL, "native route");
    *control = route->control;
    general(route, 0);
    general(route, 5);
    DVM_UIMessage ui; /* Initial independent UI message. */
    check(styxbidirroutewait(route, 1, 3000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD,
        "initial independent UI");
    linkreset();
    check(styxbidirroutesendwait(route, 102, "fill", 4, 3000, NULL) && styxbidirrouteflush(route, 3000),
        "actual fragmented fill command");
    return route;
}


/* EN/RU: Main owns fixture, grant and all native handles through teardown; no guest or graphics execution.
 * Main владеет fixture, grant и native handles до teardown; без guest либо graphics execution.
 */
int main(int argc, char **argv)
{
    check(argc == 2, "scenario argument");
    int scenario = atoi(argv[1]); /* Registered CTest case. */
    check(scenario >= 0 && scenario <= 11, "scenario range");
    StyxClient *client = linkfixture(); /* Owned client pin independent of provider. */
    check(client && styxclientbidir(client) && styxclientretain(client), "native bidir client");
    DVM_Provider *grant = styxmountproviderat(client, ".", FVM_ACCESS_MASK); /* Consumes retained client reference. */
    DVM_TerminalChannels channels = {0}; /* Trusted fixture descriptor, no ambient discovery. */
    channels.provider = grant;
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    check(grant != NULL, "native Styx grant");
    if (scenario >= 8 && scenario <= 10)
    {
        linkwatcharm(scenario - 7);
    }
    StyxBidirControl *control = NULL; /* Owned directly or borrowed from owned route. */
    StyxBidirRoute *route = transport(client, &channels, scenario, &control); /* Owned optional dispatcher. */
    if (scenario == 8)
    {
        check(!control && styxclientalive(client), "watch refusal rolls back only private control");
        control = styxbidircontrol(client, &channels, MOVIE_FRAME + 32, 1);
    }
    if (scenario == 9)
    {
        check(!control && !styxclientalive(client), "sticky terminal during subscription prevents publication");
    }
    else
    {
        check(control != NULL, "owned control");
        DVM_CancelTap *tap = styxbidircontrolwatch(control, retired, NULL, NULL); /* Owned non-consuming checkpoint. */
        check(tap != NULL, "retirement checkpoint subscription");
        linkblocked();
        StyxBidirMovieReceiver *receiver = NULL; /* Owned only for route scenarios. */
        StyxBidirMovieServer *service = NULL; /* Independent empty request queue owner. */
        if (route)
        {
            DVM_Namespace *group = exchangegroup(); /* Actual VFS service root. */
            receiver = styxbidirmoviereceiver(route, 1, 3000);
            service = styxbidirmovieserver(route, 2, 101, group, 3000);
            dvm_namespace_release(group);
            check(receiver && service, "native resource consumers");
        }
        if (scenario == 11 || scenario == 10)
        {
            styxbidircontrolstop(control);
        }
        if (scenario % 2)
        {
            linkfixturebreak();
        }
        else
        {
            styxclientshutdown(client);
        }
        waitretired();
        int expected = scenario >= 10 ? STYX_CONTROL_STOPPED : STYX_CONTROL_READ_FAILED; /* First outcome wins. */
        check(styxbidircontrolstate(control) == expected, "sticky control outcome");
        if (route)
        {
            check(styxbidirroutestate(route) == expected && styxbidirmoviereceiverwait(receiver, 1, 1000) == -1 &&
                styxbidirmovieserverwait(service, 1000) && styxbidirmovieserverstate(service) == STYX_MOVIE_SERVER_FAILED,
                "full route retires native resource workers");
            unsigned char bytes[64]; /* One retained queue item. */
            unsigned long tag; /* Original tag. */
            size_t size; /* Original body size. */
            unsigned selected = (unsigned)(scenario / 2 - 1); /* General, UI, typed. */
            DVM_UIMessage ui; /* Retained UI item when selected. */
            int head = selected == 0 ? styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) : selected == 1 ?
                styxbidirrouteui(route, &ui) : styxbidirroutetypednext(route, 0, &tag, bytes, sizeof(bytes), &size);
            check(head == STYX_CONTROL_RECORD && (selected == 1 ? ui.id == 42 : size == 4 && !memcmp(bytes, "held", 4)),
                "accepted route head survives physical retirement");
            check(styxbidirmoviereceiverfree(receiver) && styxbidirmovieserverfree(service), "join resource workers only");
        }
        else
        {
            unsigned char bytes[256]; /* One retained original binding declaration. */
            unsigned long tag; /* Exact original tag. */
            size_t size; /* Exact original body size. */
            check(styxbidircontrolnext(control, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == 0 &&
                styxbidircontrolnext(control, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_END,
                "full raw RX retains one accepted record and admits no blocked tail");
        }
        dvm_cancel_unsubscribe(tap);
    }
    if (route)
    {
        styxbidirroutefree(route);
    }
    else
    {
        styxbidircontrolfree(control);
    }
    if (scenario == 10)
    {
        styxclientshutdown(client); /* Join client owner and finish delayed admission before assertion. */
        check(linklate(), "late client callbacks access detached fence after control free");
    }
    dvm_provider_release(grant);
    styxclientshutdown(client);
    styxclientfree(client);
    linkfixturefree();
    puts("PASS full queues retire independently of RX/TX I/O");
    return 0;
}
