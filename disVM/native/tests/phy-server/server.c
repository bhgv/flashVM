/* Copyright (c) 2026 bhgv
 * EN: Original server over raw PTY PHY with real deferred events and independently pinned native producer.
 * RU: Исходный server через raw PTY PHY с real deferred events и independently pinned native producer.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "drivers/posix/fd.h"
#include "../phy/limited.h"
#include <pthread.h>
static Styxserver server; /* Owned on this native I/O thread through final end. */
static StyxRead *pending; /* Owned deferred request, never read on producer. */
static unsigned retired; /* Owner-only release count. */
static unsigned clients; /* Owner-only retired client count. */
static unsigned files; /* Owner-only retired fid contexts. */
static int available; /* Atomic one-event fixture queue, not a public protocol. */


/* EN/RU: Explicit release-build fixture invariant, diagnostic borrowed.
 * Явный release-build fixture invariant, diagnostic borrowed. */
static void check(int valid, char *why)
{
    if (!valid)
    {
        fprint(2, "PHY server: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Detach request before original fid/client retirement; release its owned public pin once.
 * Отсоединить request до fid/client retirement; освободить public pin один раз. */
static void release(void *unused)
{
    USED(unused);
    StyxRead *request = pending;
    pending = nil;
    retired++;
    styxreadfree(request);
}


/* EN/RU: Each real opened fid gets native fixture state, released through unchanged original core.
 * Каждый real opened fid получает native state с release через unchanged core. */
static char *openfile(Qid *qid, int mode)
{
    if (qid->path == Qroot && mode == OREAD)
    {
        return nil;
    }
    if ((qid->path != 1 && qid->path != 2) || mode != OREAD)
    {
        return Eperm;
    }
    void *state = malloc(1);
    if (!state)
    {
        return Enomem;
    }
    if (styxsetaux(&server, state))
    {
        free(state);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Native fid context cleanup, once on clunk/reset/disconnect; no guest callbacks.
 * Native fid cleanup один раз при clunk/reset/disconnect; без guest callbacks. */
static void freefile(void *state)
{
    files++;
    free(state);
}


/* EN/RU: Ordinary read file or actual original deferred event; return one event only.
 * Обычный read file либо original deferred event; вернуть одно событие. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path == 1)
    {
        *count = offset < 0 || offset > 6 ? 0 : styxreadstr(offset, bytes, *count, "serial");
        return nil;
    }
    if (qid.path != 2)
    {
        return Eperm;
    }
    if (__atomic_exchange_n(&available, 0, __ATOMIC_ACQ_REL))
    {
        if (*count < 6)
        {
            __atomic_store_n(&available, 1, __ATOMIC_RELEASE);
            return Ebadarg;
        }
        memmove(bytes, "notice", 6);
        *count = 6;
        return nil;
    }
    check(!pending, "only one fixture deferred read at a time");
    pending = styxdeferread(&server, nil, release);
    check(pending != nil, "original deferred read admission");
    *count = 0;
    print("PENDING\n");
    return nil;
}


/* EN/RU: Original client cleanup must follow pending/fid cleanup exactly once.
 * Original client cleanup следует за pending/fid cleanup один раз. */
static char *freeclient(Client *client)
{
    USED(client);
    check(!pending, "client retired before deferred request");
    clients++;
    return nil;
}


/* EN/RU: Own no request/server pointer on producer; copied atomic event and independent wake pin only.
 * Producer не владеет request/server pointer; только atomic event и independent wake pin. */
static void *produce(void *state)
{
    StyxPhyWake *wake = state;
    char command;
    while (read(0, &command, 1) == 1 && command != 'q')
    {
        if (command == 'e')
        {
            __atomic_store_n(&available, 1, __ATOMIC_RELEASE);
            styxphywake(wake);
        }
    }
    styxphywake(wake); /* Intentionally valid even after server end, until this final pin drops. */
    styxphywakefree(wake);
    return nil;
}


/* EN/RU: Serve one admitted physical peer on native owner; no network listener or guest execution.
 * Обслужить одного physical peer на native owner без network listener/guest execution. */
int main(int argc, char **argv)
{
    check(argc == 2, "inherited PTY descriptor");
    RemotePhy *phy = phyfixture(phyposixfd(atoi(argv[1]), PHY_STREAM|PHY_ORDERED|PHY_RELIABLE), 0);
    check(phy != nil, "bounded PTY carrier");
    Styxops ops = {0};
    ops.open = openfile;
    ops.read = readfile;
    ops.freefid = freefile;
    ops.freeclient = freeclient;
    check(!styxphyinit(&server, &ops, phy, 0555, 1, 2), "PHY original server init");
    check(styxaddfile(&server, Qroot, 1, "data", 0444, "fixture") &&
        styxaddfile(&server, Qroot, 2, "events", 0444, "fixture"), "ordinary served files");
    check(styxnotifyfd(&server, -1) == 0 && styxnotifyfd(&server, 0) < 0, "no hidden OS descriptor wait requirement");
    StyxPhyWake *wake = styxphywakepin(&server);
    pthread_t producer;
    check(wake && !pthread_create(&producer, nil, produce, wake), "independent producer pin");
    print("READY\n");
    for (;;)
    {
        if (styxwait(&server) || styxprocess(&server))
        {
            break;
        }
        if (pending && styxphyspace(&server) && __atomic_exchange_n(&available, 0, __ATOMIC_ACQ_REL))
        {
            check(styxreadreply(pending, "notice", 6, nil), "original deferred event reply");
        }
    }
    check(!styxend(&server) && !styxend(&server) && !pending && clients == 1, "original end ordering");
    print("ENDED\n");
    check(!pthread_join(producer, nil), "native producer retirement");
    print("PASS original PHY server clients=%ud files=%ud retired=%ud\n", clients, files, retired);
    return 0;
}
