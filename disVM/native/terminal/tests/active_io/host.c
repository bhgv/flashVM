/* Copyright (c) 2026 bhgv
 * EN: Route retirement cancels active native provider I/O while another operation on that provider survives.
 * RU: Route retirement отменяет active native provider I/O, сохраняя другую операцию того же provider.
 */
#include "resource/server.h"
#include "protocol.h"
#include "provider.h"
#include "gate.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef STYX_ACTIVE_IO_CASE
#define STYX_ACTIVE_IO_CASE 0 /* Real uncertain TX while metadata is blocked. */
#endif


/* EN/RU: Owned native factory/NULL, retained by caller and group until their independent releases.
 * Свой native factory/NULL, caller и group удерживают до независимых release.
 */
extern DVM_Provider *activeprovider(void);


/* EN/RU: Owned prepared namespace/NULL, borrows root only through construction and retains it on success.
 * Свой prepared namespace/NULL, root borrowed до construction и retain при успехе.
 */
extern DVM_Namespace *activegroup(DVM_Provider *root);


/* EN/RU: After joined resource worker, 1 all successful source opens consumed exactly once/0 unbalanced.
 * После joined resource worker 1 все successful source opens поглощены один раз/0 unbalanced.
 */
extern int activebalanced(void);


/* EN/RU: Arm one native constructor/TX injection after shared route creation; no guest execution.
 * Включить один native constructor/TX fault после создания shared route, без guest execution.
 */
extern void servergatearm(void);


/* EN/RU: Deliver retained fixture callback duplicates after server free, release their independent source pin.
 * Выдать удержанные fixture callback duplicates после server free, освободить их source pin.
 */
extern void activelate(void);


/* EN/RU: Explicit native fixture contract, active in every compiler profile, why borrowed until return/exit.
 * Явный native fixture contract во всех compiler profiles, why borrowed до return/exit.
 */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "active resource I/O: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Independent native metadata caller owns one scope; no cancellation/heap sharing with the server.
 * Независимый native metadata caller владеет scope, без общей cancellation/heap с server.
 */
typedef struct Other
{
    DVM_Provider *root; /* Main pins factory through join. */
    DVM_Cancel *cancel; /* Independent native source, main pins through join. */
    FVM_Result result;  /* Owner writes, main reads only after join. */
} Other;


/* EN/RU: Perform native metadata through the same factory under another scope, returning only on test release.
 * Выполнить native metadata того же factory под другим scope, вернуться только после test release.
 */
static void *independent(void *state)
{
    Other *other = state; /* Main-owned through join. */
    DVM_CancelScope scope = {0}; /* Independent native lifetime. */
    FVM_FileInfo info; /* Caller-owned coherent metadata. */
    check(dvm_cancel_enter(&scope, other->cancel), "independent native scope");
    other->result = dvm_provider_info(other->root, "other", &info);
    check(dvm_cancel_leave(&scope), "independent scope retired on original owner");
    return NULL;
}


/* EN/RU: Read one copied general record while the native resource worker exclusively owns typed RX.
 * Прочитать одну copied general record, typed RX принадлежит native resource worker.
 */
static void general(StyxBidirRoute *route, unsigned long expected, const char *payload)
{
    unsigned char bytes[512]; /* One-record caller buffer. */
    unsigned long tag; /* Copied original numeric tag. */
    size_t size; /* Exact copied payload extent. */
    check(styxbidirroutewait(route, 0, 5000) &&
        styxbidirroutenext(route, &tag, bytes, sizeof(bytes), &size) == STYX_CONTROL_RECORD && tag == expected &&
        (!payload || (size == strlen(payload) && !memcmp(bytes, payload, size))), "independent general traffic");
}


/* EN/RU: Exercise cancellation while inside actual VFS operations, not a fake guest or pending queue only.
 * Проверить отмену внутри настоящих VFS operations, не fake guest и не только pending queue.
 */
void terminalprobe(StyxClient *client, const DVM_TerminalChannels *descriptor)
{
    const unsigned scenario = STYX_ACTIVE_IO_CASE; /* Compile-time deterministic failure/stage selection. */
    DVM_TerminalChannels channels = *descriptor; /* Exact trusted bidir grant. */
    strcpy(channels.receive, "records");
    strcpy(channels.send, "records");
    StyxRouteType type = {MOVIE_TYPE, MOVIE_VERSION}; /* Exclusive resource request queue. */
    StyxBidirRoute *route = styxbidirbindingroutes(client, &channels, MOVIE_FRAME + 32, 2, "test.ide.fvd1", 1, &type, 1);
    check(route != NULL, "shared exact-client route");
    general(route, 0, NULL);
    general(route, 5, "other");
    DVM_UIMessage ui; /* Independent optional UI message. */
    check(styxbidirroutewait(route, 1, 5000) && styxbidirrouteui(route, &ui) == STYX_CONTROL_RECORD && ui.id == 42,
        "UI alongside active resource operations");
    DVM_Provider *root = activeprovider(); /* Caller-owned shared factory. */
    DVM_Namespace *group = root ? activegroup(root) : NULL; /* Independent prepared namespace owner. */
    check(root && group, "prepared scoped provider grant");
    if (scenario == 15)
    {
        styxbidirroutestop(route);
    }
    if (scenario == 14)
    {
        servergatearm();
    }
    StyxBidirMovieServer *server = styxbidirmovieserver(route, 0, 101, group, 5000); /* Own native resource worker/scope. */
    if (scenario == 13 || scenario == 14)
    {
        DVM_NamespacePlan plan; /* Caller-owned root survives constructor rollback. */
        check(!server && dvm_namespace_plan(group, "/", &plan) && styxclientalive(client),
            "subscription/worker construction refusal preserves caller namespace and client");
        server = styxbidirmovieserver(route, 0, 101, group, 5000);
    }
    check(server != NULL, "server worker and native scope published");
    if (scenario == 15)
    {
        check(styxbidirmovieserverwait(server, 1000) && styxbidirmovieserverstate(server) == STYX_MOVIE_SERVER_FAILED &&
            styxbidirmovieserverfree(server), "sticky route retirement cancels before provider admission");
    }
    else
    {
        Other other = {root, dvm_cancel_create(), FVM_ERROR}; /* Independent caller output/scope. */
        pthread_t thread; /* Joined before factory/cancel disposal. */
        check(other.cancel && !pthread_create(&thread, NULL, independent, &other), "independent operation on shared provider");
        activeentered();
        int stopped = scenario >= 8 && scenario <= 12; /* Only explicit server stop yields STOPPED. */
        if (scenario < 4)
        {
            servergatearm();
            int admitted = styxbidirroutesendwait(route, 102, "tail", 4, 5000, NULL); /* A previously queued reply may fail first. */
            check((admitted || styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED) &&
                !styxbidirrouteflush(route, 5000) && styxbidirroutestate(route) == STYX_CONTROL_WRITE_FAILED,
                "actual uncertain TX cancels active provider scope");
        }
        else if (scenario < 8 || scenario >= 13)
        {
            styxbidirroutestop(route);
        }
        else
        {
            if (scenario == 12)
            {
                styxbidirroutestop(route);
            }
            styxbidirmovieserverstop(server);
        }
        check(styxbidirmovieserverwait(server, 1000) && styxbidirmovieserverstate(server) ==
            (stopped ? STYX_MOVIE_SERVER_STOPPED : STYX_MOVIE_SERVER_FAILED),
            "active scope wakes with explicit STOPPED or route FAILED, never fabricated success");
        check(styxbidirmovieserverfree(server) && activebalanced(), "native worker joins and each source closes exactly once");
        if (scenario == 12)
        {
            activelate();
        }
        activeindependent();
        check(!pthread_join(thread, NULL) && other.result == FVM_OK && !dvm_cancel_requested(other.cancel),
            "same provider's independent operation survives server scope cancellation");
        dvm_cancel_release(other.cancel);
        if (scenario >= 8 && scenario <= 11)
        {
            check(!styxbidirroutestate(route) && styxbidirroutesendwait(route, 102, "alive", 5, 5000, NULL),
                "local server stop preserves shared route/TX");
            general(route, 9, "alive");
        }
    }
    dvm_namespace_release(group);
    dvm_provider_release(root);
    styxbidirroutefree(route);
    check(styxclientalive(client), "ordinary Styx client survives cancelled native resource operation");
}
