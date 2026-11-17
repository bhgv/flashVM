/* Copyright (c) 2026 bhgv
 * EN: Native client and watch/cancel caller over PHY, sharing the link with actual file RPCs.
 * RU: Native client и watch/cancel caller через PHY при параллельных actual file RPCs.
 */
#include "client.h"
#include "drivers/posix/fd.h"
#include "../phy/limited.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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
    StyxClient *client = styxclientmountphy(phy, 256, "fixture", "", 3000, 4);
    check(client && styxclientwatchable(client), "native PHY mount");
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
    puts("PASS both native PHY peers with original deferred core");
    return 0;
}
