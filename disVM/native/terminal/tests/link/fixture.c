/* Copyright (c) 2026 bhgv
 * EN: Original Styx server and explicit bidir pumps on an isolated native carrier for terminal-loss tests.
 * RU: Original Styx server и явные bidir pumps на изолированном native carrier для terminal-loss tests.
 */
#include <sys/socket.h>
#include <unistd.h>
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "client.h"
#include "server.h"
#include "mux/bidir.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
static Styxserver server;    /* Original native server, owned through join. */
static pthread_t worker;     /* Dedicated original server owner. */
static PhyBidir *local;      /* Host-side explicit bidir pump. */
static PhyBidir *remote;     /* Server-side explicit bidir pump. */
static int breaker = -1;    /* Owned duplicate used only to break the real physical carrier. */


/* EN/RU: Original fragmented S-record fixture callbacks, called solely by original server owner.
 * Исходные fragmented S-record callbacks, только на original server owner.
 */
extern char *terminalread(Styxserver *, char *, ulong *, vlong);


/* EN/RU: Original precise-prefix S-record command writer; borrowed buffers on server owner.
 * Исходный precise-prefix writer S-команд, buffers borrowed на server owner.
 */
extern char *terminalwrite(char *, ulong *, vlong);


/* EN/RU: Serve only the granted records node; original core owns walk, permissions and fid lifecycle.
 * Обслужить records node; walk, права и fid lifecycle принадлежат original core.
 */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    return qid.path == 7 ? terminalread(&server, bytes, count, offset) : Ebadarg;
}


/* EN/RU: Write only records; buffers/count borrowed from original core through return.
 * Записать только records; buffers/count borrowed из original core до return.
 */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    return qid.path == 7 ? terminalwrite(bytes, count, offset) : Ebadarg;
}
static Styxops operations = {.read = readfile, .write = writefile}; /* Server borrows static operations until join. */


/* EN/RU: Own original server callbacks until physical/logical teardown, consume server PHY once.
 * Владеть original server callbacks до teardown, поглотить server PHY один раз.
 */
static void *serve(void *unused)
{
    USED(unused);
    while (!styxwait(&server) && !styxprocess(&server))
    {
    }
    styxend(&server);
    return nil;
}


/* EN/RU: Own real client/NULL on one logical stream of an explicitly bidir carrier, no guest/GUI.
 * Свой real client/NULL на logical stream явного bidir carrier, без guest/GUI.
 */
StyxClient *linkfixture(void)
{
    int pair[2]; /* Connected native fixture carrier; each PHY consumes one original descriptor. */
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair) || (breaker = dup(pair[0])) < 0)
    {
        exit(3);
    }
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE; /* Explicit carrier capabilities. */
    local = phybidircreate(phyposixfd(pair[0], caps), 4096);
    remote = phybidircreate(phyposixfd(pair[1], caps), 4096);
    if (!local || !remote || styxphyinit(&server, &operations, phybidirchannel(remote, 0), 0555, 1, 2) ||
        !styxaddfile(&server, Qroot, 7, "records", 0666, "fixture") || pthread_create(&worker, nil, serve, nil))
    {
        exit(3);
    }
    return styxclientmountphy(phybidirchannel(local, 0), 256, "fixture", "", 3000, 4);
}


/* EN/RU: Break the actual carrier independently of control/client API; no fake client terminal notification.
 * Разорвать настоящий carrier независимо от control/client API, без fake terminal notification.
 */
void linkfixturebreak(void)
{
    if (shutdown(breaker, SHUT_RDWR))
    {
        exit(3);
    }
}


/* EN/RU: After all client/control users retire, join server and consume physical owners/descriptors.
 * После завершения client/control users сделать join server и поглотить physical owners/fds.
 */
void linkfixturefree(void)
{
    if (pthread_join(worker, nil))
    {
        exit(3);
    }
    phybidirfree(local);
    phybidirfree(remote);
    close(breaker);
}
