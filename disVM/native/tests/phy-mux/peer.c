/* Copyright (c) 2026 bhgv
 * EN: Simultaneous original 9P client/server over one PTY; native deferred reads and reverse RPC.
 * RU: Одновременные original 9P client/server через один PTY; native deferred reads и reverse RPC.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "client.h"
#include "server.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include "../phy/limited.h"
#include <pthread.h>
static Styxserver server; /* Owned by native server thread. */
static StyxRead *pending; /* Original deferred request, server owner only. */
static int event; /* Atomic native source notification. */
static char identity[2]; /* Immutable provider label. */


/* EN/RU: Explicit invariant, borrowed diagnostic; fatal on fixture failure. / Явный invariant, fatal failure. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "duplex mux: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Retire original deferred pin exactly once before fid/client cleanup. / Завершить original pin один раз. */
static void release(void *state)
{
    USED(state);
    StyxRead *request = pending;
    pending = nil;
    styxreadfree(request);
}


/* EN/RU: Allow only original readonly file/device open; qid/mode borrowed values. / Только readonly open. */
static char *openfile(Qid *qid, int mode)
{
    return (qid->path == 1 || qid->path == 2) && mode == OREAD ? nil : Eperm;
}


/* EN/RU: Identity file or pending one-event read; all request state stays on server owner.
 * Identity file либо pending one-event read; request state только на server owner. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path == 1)
    {
        *count = styxreadstr(offset, bytes, *count, identity);
        return nil;
    }
    check(qid.path == 2 && !pending, "event fid and one pending request");
    pending = styxdeferread(&server, nil, release);
    check(pending != nil, "original deferred read");
    *count = 0;
    print("PENDING\n");
    return nil;
}


/* EN/RU: Native owner serves reverse session while independent forward session is pending.
 * Native owner обслуживает reverse session независимо от pending forward session. */
static void *serving(void *unused)
{
    USED(unused);
    while (!styxwait(&server) && !styxprocess(&server))
    {
        if (pending && styxphyspace(&server) && __atomic_exchange_n(&event, 0, __ATOMIC_ACQ_REL))
        {
            check(styxreadreply(pending, "notice", 6, nil), "event reply");
        }
    }
    styxend(&server);
    check(!pending, "pending read retired on close");
    return nil;
}


/* EN/RU: Native watch caller owns result until join, fid borrowed from main. / Native watch caller, fid borrowed. */
typedef struct Read
{
    StyxFid *fid; /* Main-owned retained fid. */
    StyxCancel *cancel; /* Main-owned one-read cancellation lifetime. */
    StyxTransfer result; /* Worker-owned until join. */
    char bytes[8]; /* Worker output. */
} Read;


/* EN/RU: Block only native read caller, no guest suspension/cooperation. / Ждёт только native caller. */
static void *watching(void *state)
{
    Read *work = state;
    styxclientreadwatch(work->fid, 0, work->bytes, sizeof(work->bytes), work->cancel, &work->result);
    return nil;
}


/* EN/RU: Walk/open ordinary granted name; owned fid or fatal fixture failure. / Walk/open, свой fid. */
static StyxFid *opened(StyxClient *client, char *name)
{
    StyxFid *fid = styxclientwalk(client, name);
    check(fid && styxclientopen(fid, OREAD), "walk/open");
    return fid;
}


/* EN/RU: Side0/1 owns both opposite 9P roles; Python gates only after both watches are admitted.
 * Сторона0/1 владеет противоположными 9P roles; Python gate после admission обеих watches. */
int main(int argc, char **argv)
{
    check(argc == 4, "inherited PTY and side");
    unsigned side = atoi(argv[2]);
    int cancel = !strcmp(argv[3], "cancel") && side == 0;
    check(side < 2, "side range");
    identity[0] = 'A'+side;
    RemotePhy *carrier = phyfixture(phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 0);
    PhyBidir *hub = phybidircreate(carrier, 1024);
    check(hub != nil, "physical mux");
    RemotePhy *forward = phybidirchannel(hub, side);
    RemotePhy *reverse = phybidirchannel(hub, 1-side);
    check(forward && reverse && !phybidirchannel(hub, side), "single logical ownership");
    Styxops ops = {0};
    ops.open = openfile;
    ops.read = readfile;
    check(!styxphyinit(&server, &ops, reverse, 0555, 1, 2), "original reverse server");
    check(styxaddfile(&server, Qroot, 1, "identity", 0444, "fixture") &&
        styxaddfile(&server, Qroot, 2, "events", 0444, "fixture"), "exported names");
    StyxPhyWake *wake = styxphywakepin(&server);
    pthread_t owner;
    check(wake && !pthread_create(&owner, nil, serving, nil), "native server owner");
    StyxClient *client = styxclientmountphy(forward, 256, "fixture", "", 5000, 4);
    check(client != nil, "forward original 9P mount");
    Read work = {0};
    work.fid = opened(client, "events");
    work.cancel = styxcancelcreate();
    check(work.cancel != nil, "watch cancel token");
    pthread_t watch;
    check(!pthread_create(&watch, nil, watching, &work), "native pending read");
    StyxFid *data = opened(client, "identity");
    StyxTransfer out;
    char bytes[8];
    styxclientread(data, 0, bytes, sizeof(bytes), &out);
    check(out.success && out.count == 1 && bytes[0] == 'A'+(1-side), "reverse provider identity");
    styxclientclunk(data);
    print("ORDINARY\n");
    char command;
    check(read(0, &command, 1) == 1 && command == 'e', "both native reads admitted");
    if (cancel)
    {
        styxcancelrequest(work.cancel);
    }
    if (strcmp(argv[3], "cancel") || side == 0)
    {
        __atomic_store_n(&event, 1, __ATOMIC_RELEASE);
        styxphywake(wake);
    }
    check(!pthread_join(watch, nil), "watch join");
    if (cancel)
    {
        check(work.result.cancelled && !work.result.success && styxclientalive(client), "Tflush cancels one watch on healthy stream");
        data = opened(client, "identity");
        styxclientread(data, 0, bytes, sizeof(bytes), &out);
        check(out.success && out.count == 1, "ordinary RPC after Tflush");
        styxclientclunk(data);
    }
    else
    {
        check(work.result.success && work.result.count == 6 && !memcmp(work.bytes, "notice", 6), "reverse event delivery");
    }
    styxcancelfree(work.cancel);
    styxclientclunk(work.fid);
    print("COMPLETE\n");
    check(read(0, &command, 1) == 1 && command == 'q', "both directions completed before close");
    styxclientfree(client);
    check(!pthread_join(owner, nil), "logical CLOSE retires reverse server");
    print("ENDED\n");
    check(read(0, &command, 1) == 1 && command == 't', "both peer servers witnessed CLOSE before pump stop");
    phybidirfree(hub);
    styxphywake(wake); /* Late native pin remains valid after hub stop. */
    styxphywakefree(wake);
    print("PASS simultaneous original 9P sessions on one carrier\n");
    return 0;
}
