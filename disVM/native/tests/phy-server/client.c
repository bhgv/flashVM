/* Copyright (c) 2026 bhgv
 * EN: Native client and watch/cancel caller over PHY, sharing the link with actual file RPCs.
 * RU: Native client и watch/cancel caller через PHY при параллельных actual file RPCs.
 */
#ifdef STYX_WALK_TEST
#include "../../client/mux_internal.h"
#else
#include "client.h"
#endif
#include "drivers/posix/fd.h"
#include "../phy/limited.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef STYX_LAUNCH_TEST


/* EN/RU: Native pre-init acceptance; client/name/expected borrowed through return.
 * Native pre-init проверка, client/name/expected borrowed до return. */
extern void launchremote(StyxClient *, const char *, const char *);
#endif


/* EN/RU: Explicit fixture invariant, no build-mode dependence.
 * Явный fixture invariant независимо от build mode. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprintf(stderr, "PHY peer: %s\n", why);
        exit(2);
    }
}


#ifdef STYX_WALK_TEST


/* EN/RU: Require an actually acknowledged idle probe on the native broker, not guessed elapsed sleep or ordinary traffic.
 * Native fixture borrows client/mux through wait; main's application history must not be copied by broker maintenance.
 * Потребовать actual idle probe acknowledgement на native broker, без guessed sleep; client/mux borrowed до return.
 */
static void heartbeat(StyxClient *client)
{
    check(styxclientkeepalive(client, 5, 1000, 4), "profile heartbeat admission");
    struct timespec deadline;
    check(!clock_gettime(CLOCK_REALTIME, &deadline), "heartbeat deadline");
    deadline.tv_sec += 4;
    StyxMux *mux = client->mux;
    pthread_mutex_lock(&mux->lock);
    while (!mux->beat.completed && !mux->failed)
    {
        int status = pthread_cond_timedwait(&mux->ended, &mux->lock, &deadline);
        check(!status, "profile heartbeat was not acknowledged");
    }
    check(mux->beat.completed && !mux->failed, "profile heartbeat broke carrier");
    pthread_mutex_unlock(&mux->lock);
    check(styxclientkeepalive(client, 0, 0, 0), "profile heartbeat disable");
}
#endif


/* EN/RU: Native caller owns exact read storage; main pins fid/token through join.
 * Native caller владеет read storage; main удерживает fid/token до join. */
typedef struct Work
{
    StyxFid *fid; /* Borrowed main-owned event fid. */
    StyxCancel *cancel; /* Borrowed one-operation token. */
    StyxTransfer out; /* Worker-only result until join. */
    char bytes[16]; /* Worker-only read output. */
} Work;


/* EN/RU: Execute one real watch read on native caller; no guest, dispatch callback or polling.
 * Выполнить real watch read на native caller без guest/callback/polling. */
static void *reading(void *state)
{
    Work *work = state;
    styxclientreadwatch(work->fid, 0, work->bytes, sizeof(work->bytes), work->cancel, &work->out);
    return NULL;
}


/* EN/RU: Walk/open one granted readonly name, owned fid/NULL fatal fixture failure.
 * Пройти/open granted readonly name, свой fid; failure завершает fixture. */
static StyxFid *opened(StyxClient *client, const char *name)
{
    StyxFid *fid = styxclientwalk(client, name);
    check(fid && styxclientopen(fid, 0), "ordinary file open");
    return fid;
}


/* EN/RU: Ordinary file RPC must proceed while another native watch read is pending.
 * Обычный file RPC обязан выполняться при pending native watch read. */
static void ordinary(StyxClient *client)
{
    StyxFid *fid = opened(client, "data");
    StyxTransfer out;
    char bytes[16];
    styxclientread(fid, 0, bytes, sizeof(bytes), &out);
    check(out.success && out.count == 6 && !memcmp(bytes, "serial", 6), "ordinary concurrent bytes");
    styxclientclunk(fid);
}


/* EN/RU: Native both-peer acceptance; Python gates only after real server pending admission.
 * Native both-peer acceptance; Python gate только после real pending admission сервера. */
int main(int argc, char **argv)
{
    check(argc == 3, "inherited PTY descriptor/mode");
    RemotePhy *phy = phyfixture(phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 0);
#ifdef STYX_WALK_TEST
    unsigned long domain[4] = {1, 2, 3, 4};
    WalkEntity source;
    WalkScope history = {0};
    check(walk_domain(domain) && walk_entity(&source) &&
        walk_enter(&history, &source, 0, 0, WALK_INFO, "client/search", "") == WALK_OK, "client active history");
    StyxClient *client = styxclientwalkmountphy(phy, 256, "fixture", "", 3000, 4, 39);
#else
    StyxClient *client = styxclientmountphy(phy, 256, "fixture", "", 3000, 4);
#endif
    check(client && !styxclientbidir(client) && styxclientwatchable(client), "native PHY mount");
#ifdef STYX_LAUNCH_TEST
    launchremote(client, "data", "serial");
#endif
#ifdef STYX_WALK_TEST
    heartbeat(client);
#endif
    Work work = {0};
    work.fid = opened(client, "events");
    work.cancel = styxcancelcreate();
    pthread_t worker;
    check(work.cancel && !pthread_create(&worker, NULL, reading, &work), "native watch caller");
    ordinary(client);
    puts("ORDINARY");
    fflush(stdout);
    check(getchar() == 'g', "external native admission gate");
    if (!strcmp(argv[2], "cancel"))
    {
        styxcancelrequest(work.cancel);
    }
    else if (!strcmp(argv[2], "disconnect"))
    {
        styxclientshutdown(client);
    }
    check(!pthread_join(worker, NULL), "native watch join");
    if (!strcmp(argv[2], "cancel"))
    {
        check(work.out.cancelled && !work.out.success && styxclientalive(client), "cancel closed healthy carrier");
        ordinary(client);
    }
    else if (!strcmp(argv[2], "normal"))
    {
        check(work.out.success && work.out.count == 6 && !memcmp(work.bytes, "notice", 6), "native source event bytes");
    }
    else
    {
        check(!work.out.success && !styxclientalive(client), "disconnect preserved stale watch");
    }
    styxcancelfree(work.cancel);
    styxclientclunk(work.fid);
    styxclientshutdown(client);
    styxclientfree(client);
#ifdef STYX_WALK_TEST
    check(walk_depth() == 1 && walk_leave(&history) && !walk_depth(), "client history retirement");
#endif
    puts("PASS both native PHY peers with original deferred core");
    return 0;
}
