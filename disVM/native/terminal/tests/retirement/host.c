/* Copyright (c) 2026 bhgv
 * EN: Full route queues must not hide transport retirement from native resource/queue waits.
 * RU: Полные route queues не скрывают transport retirement от native resource/queue waits.
 */
#define _POSIX_C_SOURCE 200809L
#include "../../route/internal.h"
#include "../../resource/announced.h"
#include "../../resource/server.h"
#include "../../internal.h"
#include "cancel.h"
#include "announcement.h"
#include <stdio.h>
#ifndef STYX_RETIREMENT_CASE
#define STYX_RETIREMENT_CASE 0 /* Full general queue plus uncertain TX. */
#endif


/* EN/RU: Arm fixture observation before complete S-record fill; route pinned until dispatcher join.
 * Включить fixture observation до fill S-records; route живёт до dispatcher join.
 */
extern void retirementarm(StyxBidirRoute *, unsigned);


/* EN/RU: Observe actual native dispatcher backpressure; outer process timeout bounds fixture failure.
 * Наблюдать настоящий native dispatcher backpressure; timeout процесса ограничивает отказ fixture.
 */
extern void retirementblocked(void);


/* EN/RU: Deliver test-admitted duplicate callbacks only after the route allocation is freed.
 * Выдать test-admitted duplicate callbacks только после освобождения route allocation.
 */
extern void retirementlate(void);


/* EN/RU: Arm the original native-write wrapper once, retaining real precise-prefix I/O.
 * Включить original native-write wrapper один раз, сохраняя real precise-prefix I/O.
 */
extern void servergatearm(void);


/* EN/RU: Create owned actual VFS service namespace/NULL, caller releases after server construction.
 * Свой actual VFS service namespace/NULL, caller освобождает после создания server.
 */
extern DVM_Namespace *exchangegroup(void);


/* EN/RU: Explicit fixture error in all compiler profiles, why borrowed through return/exit.
 * Явный fixture error во всех compiler profiles, why заимствован до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "route retirement: %s\n", why);
        exit(2);
    }
}


/* EN/RU: One native observation owns outputs until join, consumes no record or cancellation authority.
 * Одно native наблюдение владеет outputs до join, не потребляет records/cancel authority.
 */
typedef struct Waiting
{
    StyxBidirRoute *route; /* Borrowed through join. */
    DVM_Cancel *cancel; /* Optional caller-owned source through join. */
    unsigned kind; /* General/UI/typed/typed-cancel selection. */
    int result; /* Written only by owner, read after join. */
    long elapsed; /* Monotonic milliseconds of native wait. */
} Waiting;


/* EN/RU: Wait on an empty mailbox for up to 5s; retirement must wake it without waiting for that timeout.
 * Ждать empty mailbox до 5s; retirement будит без ожидания timeout, без guest callbacks.
 */
static void *waiting(void *state)
{
    Waiting *wait = state; /* Main pins through thread join. */
    struct timespec start; /* Original native wait start. */
    struct timespec end; /* Actual return time. */
    check(!clock_gettime(CLOCK_MONOTONIC, &start), "native start clock");
    wait->result = wait->kind < 2 ? styxbidirroutewait(wait->route, wait->kind, 5000) :
        wait->kind == 2 ? styxbidirroutetypedwait(wait->route, 2, 5000) :
            styxbidirroutetypedwaitcancel(wait->route, 2, 5000, wait->cancel);
    check(!clock_gettime(CLOCK_MONOTONIC, &end), "native end clock");
    wait->elapsed = (end.tv_sec - start.tv_sec) * 1000L + (end.tv_nsec - start.tv_nsec) / 1000000L;
    return NULL;
}


/* EN/RU: Drain one initial general record from a single owner, preserving other mailbox ownership.
 * Прочитать одну initial general record одним owner, не трогая другие mailboxes.
 */
static void general(StyxBidirRoute *route, unsigned long expected)
{
    unsigned char bytes[512]; /* One-record caller storage. */
    unsigned long tag; /* Exact consumed numeric tag. */
    size_t size; /* Exact consumed body size. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected,
        "initial independent general record");
}


/* EN/RU: Exercise real TX failure or public control stop while dispatcher waits on a full native queue.
 * Проверить real TX failure либо public control stop при ожидании dispatcher полной native queue.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    DVM_TerminalChannels channels = *descriptor; /* Existing trusted bidir handoff. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType types[] = { /* Exclusive resource owners and one fixture-only blocked queue. */
        {"test.blocked", 1}, {MOVIE_OFFER_TYPE, MOVIE_OFFER_VERSION}, {MOVIE_TYPE, MOVIE_VERSION}
    };
    if (STYX_RETIREMENT_CASE == 6)
    {
        check(!styxbidirbindingroutes(client, &channels, MOVIE_FRAME + 32, 1, "test.ide.fvd1", 1, types, 3) &&
            styxclientalive(client), "retirement subscription refusal rolls back private route only");
    }
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, MOVIE_FRAME + 32, 1, "test.ide.fvd1", 1, types, 3);
    check(route != NULL, "owned registered route after optional construction refusal");
    general(route, 0);
    general(route, 5);
    DVM_UIMessage ui; /* Initial independent UI message. */
    check(styxbidirroutewait(route, 1, 5000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD,
        "initial UI traffic before fixture fill");
    if (STYX_RETIREMENT_CASE == 7)
    {
        styxbidirroutefree(route);
        retirementlate();
        check(styxclientalive(client), "late callbacks use detached fence, never freed route");
        return;
    }
    unsigned full = STYX_RETIREMENT_CASE % 3; /* General/UI/typed queue selected by both fixture peers. */
    retirementarm(route, full);
    check(styxbidirroutesendwait(route, 102, "fill", 4, 5000, NULL) && styxbidirrouteflush(route, 5000), "fill command crossed real Styx");
    retirementblocked();
    DVM_Cancel *cancel = dvm_cancel_create(); /* Independent uncancelled source for typed waiter. */
    DVM_Namespace *group = exchangegroup(); /* Actual prepared VFS service root. */
    StyxBidirMovieReceiver *receiver = styxbidirmoviereceiver(route, 1, 5000); /* Own native empty offer wait. */
    StyxBidirMovieServer *server = styxbidirmovieserver(route, 2, 101, group, 5000); /* Own native empty request wait. */
    dvm_namespace_release(group);
    check(cancel && receiver && server, "independent native resource workers");
    Waiting waits[] = { /* No waiter consumes the intentionally full queue. */
        {route, NULL, full == 0 ? 1U : 0U, -1, -1},
        {route, NULL, 2, -1, -1}, {route, cancel, 3, -1, -1}
    };
    pthread_t threads[3]; /* Joined before route/cancel disposal. */
    for (unsigned i = 0; i < 3; i++)
    {
        check(!pthread_create(&threads[i], NULL, waiting, &waits[i]), "native queue waiter creation");
    }
    int expected = STYX_RETIREMENT_CASE >= 3 && STYX_RETIREMENT_CASE < 6 ? STYX_CONTROL_STOPPED : STYX_CONTROL_WRITE_FAILED;
    if (expected == STYX_CONTROL_STOPPED)
    {
        styxbidircontrolstop(route->control);
    }
    else
    {
        servergatearm();
        check(styxbidirroutesendwait(route, 102, "tail", 4, 5000, NULL) && !styxbidirrouteflush(route, 5000),
            "real acknowledged prefix becomes uncertain without replay");
    }
    check(styxbidirroutestate(route) == expected && styxbidirmovieserverwait(server, 1000) &&
        styxbidirmovieserverstate(server) == STYX_MOVIE_SERVER_FAILED &&
        styxbidirmoviereceiverwait(receiver, 1, 1000) == -1, "native resources retire while dispatcher has no capacity");
    for (unsigned i = 0; i < 3; i++)
    {
        check(!pthread_join(threads[i], NULL) && !waits[i].result && waits[i].elapsed < 1500,
            "empty general/UI/typed/cancel wait wakes promptly instead of waiting 5s");
    }
    check(!dvm_cancel_requested(cancel), "route retirement does not mutate caller cancellation source");
    dvm_cancel_release(cancel);
    unsigned char bytes[512]; /* One retained queue head. */
    unsigned long tag; /* Exact retained tag. */
    size_t size; /* Retained payload extent. */
    int head = full == 0 ? styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) : full == 1 ?
        styxbidirrouteui(route, &ui) : styxbidirroutetypednext(route, 0, &tag, bytes, sizeof(bytes), &size);
    check(head == STYX_CONTROL_RECORD && (full == 1 ? ui.id == 42 :
        size == 4 && !memcmp(bytes, "held", 4) && tag == (full == 0 ? 9UL : 23UL)), "accepted head survives retirement");
    head = full == 0 ? styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) : full == 1 ?
        styxbidirrouteui(route, &ui) : styxbidirroutetypednext(route, 0, &tag, bytes, sizeof(bytes), &size);
    check(head == STYX_CONTROL_END, "waiting dispatcher never admits a second record after retirement");
    check(styxbidirmoviereceiverfree(receiver) && styxbidirmovieserverfree(server), "only resource-owned workers joined");
    retirementarm(NULL, 0);
    styxbidirroutefree(route);
    check(styxclientalive(client), "ordinary bidir client survives local transport/route disposal");
}
