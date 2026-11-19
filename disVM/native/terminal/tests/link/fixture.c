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
#ifdef ORDERED_GUEST_FILE_TEST
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <sys/wait.h>
#endif
static Styxserver server;    /* Original native server, owned through join. */
#ifdef ORDERED_GUEST_FILE_TEST
static pid_t worker;         /* Independent server process, reaped after client close. */
#else
static pthread_t worker;     /* Dedicated original server owner. */
#endif
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

#ifdef ORDERED_GUEST_FILE_TEST


/* EN/RU: Own actual compiled readonly asset storage through original server join.
 * Владеть настоящим compiled readonly asset-storage до join original server.
 */
extern void fixtureimagesinit(Styxserver *);


/* EN/RU: Serve one original remote offset/count, borrowed buffers, short prefixes only.
 * Обслужить исходные remote offset/count, buffers borrowed, только short prefixes.
 */
extern char *fixtureimagesread(Qid, char *, ulong *, vlong);


/* EN/RU: Release image storage after every original server callback has retired.
 * Освободить image-storage после завершения всех original server callbacks.
 */
extern void fixtureimagesfree(void);
#endif


/* EN/RU: Serve only the granted records node; original core owns walk, permissions and fid lifecycle.
 * Обслужить records node; walk, права и fid lifecycle принадлежат original core.
 */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
#ifdef ORDERED_GUEST_FILE_TEST
    if (qid.path != 7)
    {
        return fixtureimagesread(qid, bytes, count, offset);
    }
#endif
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


/* EN/RU: Consume fd and start the original server on the explicit logical stream.
 * Поглотить fd и настроить исходный server на явном logical stream. */
static void server_start(int fd, unsigned caps)
{
    remote = phybidircreate(phyposixfd(fd, caps), 4096);
    if (!remote || styxphyinit(&server, &operations, phybidirchannel(remote, 0), 0555, 1, 2) ||
        !styxaddfile(&server, Qroot, 7, "records", 0666, "fixture"))
    {
        exit(3);
    }
}


#ifdef ORDERED_GUEST_FILE_TEST
/* EN/RU: Build a private TCP connection on loopback; return both owned descriptors.
 * Создать частное TCP-соединение через loopback, вернуть два собственных fd. */
static void loopback_pair(int pair[2])
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in address = {0};
    socklen_t size = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (listener < 0 || bind(listener, (struct sockaddr *)&address, size) ||
        getsockname(listener, (struct sockaddr *)&address, &size) || listen(listener, 1))
    {
        exit(3);
    }
    pair[0] = socket(AF_INET, SOCK_STREAM, 0);
    if (pair[0] < 0 || connect(pair[0], (struct sockaddr *)&address, size))
    {
        exit(3);
    }
    pair[1] = accept(listener, NULL, NULL);
    int immediate = 1; /* The fixture deliberately splits reads and write acknowledgements. */
    if (pair[1] < 0 || close(listener) ||
        setsockopt(pair[0], IPPROTO_TCP, TCP_NODELAY, &immediate, sizeof(immediate)) ||
        setsockopt(pair[1], IPPROTO_TCP, TCP_NODELAY, &immediate, sizeof(immediate)))
    {
        exit(3);
    }
}
#endif


/* EN/RU: Own real client/NULL on one logical stream of an explicitly bidir carrier.
 * Свой real client/NULL на logical stream явного bidir carrier.
 */
StyxClient *linkfixture(void)
{
    int pair[2]; /* Connected carrier; each PHY consumes one descriptor. */
    unsigned caps = PHY_STREAM | PHY_ORDERED | PHY_RELIABLE;
#ifdef ORDERED_GUEST_FILE_TEST
    loopback_pair(pair);
    worker = fork(); /* No native worker has started yet. */
    if (worker < 0)
    {
        exit(3);
    }
    if (!worker)
    {
        close(pair[0]);
        signal(SIGPIPE, SIG_IGN);
        server_start(pair[1], caps);
        fixtureimagesinit(&server);
        serve(NULL);
        fixtureimagesfree();
        phybidirfree(remote);
        _exit(0);
    }
    close(pair[1]);
#else
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair))
    {
        exit(3);
    }
    server_start(pair[1], caps);
    if (pthread_create(&worker, nil, serve, nil))
    {
        exit(3);
    }
#endif
    breaker = dup(pair[0]);
    local = phybidircreate(phyposixfd(pair[0], caps), 4096);
    if (breaker < 0 || !local)
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
#ifdef ORDERED_GUEST_FILE_TEST
    shutdown(breaker, SHUT_RDWR); /* Wake the remote process before joining it. */
    phybidirfree(local);
    close(breaker);
    int status;
    if (waitpid(worker, &status, 0) != worker || !WIFEXITED(status) || WEXITSTATUS(status))
    {
        exit(3);
    }
#else
    if (pthread_join(worker, nil))
    {
        exit(3);
    }
    phybidirfree(local);
    phybidirfree(remote);
    close(breaker);
#endif
}
