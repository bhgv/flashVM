/* Copyright (c) 2026 bhgv
 * EN: Original C Styx embedding of ordinary event file with native readiness/deferred reads.
 * RU: Original C Styx embedding ordinary event file с native readiness/deferred reads.
 */
#include <lib9.h>
#include <styx.h>
#include "styxserver.h"
#include "fixture.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>


/* EN/RU: Native socket owner exclusively owns pending wire request references.
 * Native socket owner исключительно владеет pending wire request references. */
typedef struct Pending
{
    struct Pending *next; /* Owner's pending chain. */
    StyxRead *request; /* Owned caller reference through library retirement. */
} Pending;
static Styxserver server; /* Original POSIX transport and dispatch. */
static StyxBacking *backing; /* Borrowed fixture prepared file capability. */
static Pending *head; /* Owner-only pending event reads. */
static int stopping; /* Owner-only fixture control; no public event protocol. */


/* EN/RU: Always-active fixture invariant.
 * Постоянный fixture invariant. */
static void check(int valid, const char *why)
{
    if (!valid)
    {
        fprint(2, "event resource TCP: %s\n", why);
        exit(2);
    }
}


/* EN/RU: Retire one detached native request before its fid context is freed.
 * Убрать detached native request до освобождения fid context. */
static void retired(void *state)
{
    Pending *pending = state;
    for (Pending **link = &head; *link; link = &(*link)->next)
    {
        if (*link == pending)
        {
            *link = pending->next;
            break;
        }
    }
    styxreadfree(pending->request);
    free(pending);
}


/* EN/RU: Fixture tree chooses its own resource names; library installs no reserved path.
 * Fixture tree сама выбирает имена; library не устанавливает reserved path. */
static char *walk(Qid *qid, char *name)
{
    if (qid->path != Qroot)
    {
        return Enonexist;
    }
    if (!strcmp(name, "events"))
    {
        *qid = styxqid(1, 0);
    }
    else if (!strcmp(name, "ctl"))
    {
        *qid = styxqid(2, 0);
    }
    else if (strcmp(name, ".."))
    {
        return Enonexist;
    }
    return nil;
}


/* EN/RU: Capture a real provider-backed open only for event resource; ctl is fixture control.
 * Захватить provider-backed open для event resource; ctl только fixture control. */
static char *openfile(Qid *qid, int mode)
{
    if (qid->path == 1)
    {
        StyxOpen *opened = styxfileopen(backing, mode);
        if (!opened)
        {
            return "provider open failed";
        }
        if (styxsetaux(&server, opened) < 0)
        {
            styxfileclose(opened);
            return "fid context refused";
        }
        return nil;
    }
    return (qid->path == Qroot && mode == OREAD) || (qid->path == 2 && mode == OWRITE) ? nil : Eperm;
}


/* EN/RU: Use actual SDK read outcome; only AGAIN defers, zero successful read is true EOF.
 * Использовать SDK outcome; только AGAIN откладывается, successful zero read — EOF. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 1)
    {
        return Eperm;
    }
    StyxTransfer result;
    styxfileread(styxgetaux(&server), offset, bytes, *count, &result);
    if (result.again)
    {
        Pending *pending = calloc(1, sizeof(*pending));
        if (!pending)
        {
            return Enomem;
        }
        pending->request = styxdeferread(&server, pending, retired);
        if (!pending->request)
        {
            free(pending);
            return Eperm;
        }
        pending->next = head;
        head = pending;
        *count = 0;
        print("pending\n");
        return nil;
    }
    if (!result.success || !result.confirmed || result.uncertain)
    {
        return "event resource read failed";
    }
    *count = result.count;
    return nil;
}


/* EN/RU: Complete ready requests on the socket owner; producer never touches these pointers.
 * Завершить ready requests на socket owner; producer не касается pointers. */
static void complete(void)
{
    char bytes[MSGMAX];
    Pending *next;
    for (Pending *pending = head; pending; pending = next)
    {
        next = pending->next;
        StyxTransfer result;
        styxfileread(styxreadaux(pending->request), styxreadoffset(pending->request),
                     bytes, styxreadlimit(pending->request), &result);
        if (result.again)
        {
            continue;
        }
        check(styxreadreply(pending->request, bytes, result.success ? result.count : 0,
                           result.success ? nil : "event resource read failed"), "deferred response");
    }
}


/* EN/RU: Synthetic metadata follows retained provider open and never grants resource access.
 * Synthetic metadata следуют provider open и не дают resource access. */
static char *statfile(Qid qid, Dir *out)
{
    memset(out, 0, sizeof(*out));
    out->qid = qid;
    out->mode = qid.path == Qroot ? DMDIR|0555 : qid.path == 2 ? 0222 : 0444;
    out->name = qid.path == Qroot ? "/" : qid.path == 2 ? "ctl" : "events";
    out->uid = out->gid = out->muid = "fixture";
    return nil;
}


/* EN/RU: Stop command is fixture-only; actual resource remains ordinary readonly file.
 * Stop command только fixture; actual resource остаётся ordinary readonly file. */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    if (qid.path != 2 || offset != 0 || *count != 4 || memcmp(bytes, "stop", 4))
    {
        return Eperm;
    }
    stopping = 1;
    return nil;
}


/* EN/RU: Independent producer touches only journal/inbox scalar APIs, never Client/Fid/guest.
 * Independent producer использует journal/inbox scalar API, не Client/Fid/guest. */
static void *producer(void *unused)
{
    (void)unused;
    char command;
    for (;;)
    {
        int n;
        do
        {
            n = read(0, &command, 1);
        } while (n < 0 && errno == EINTR);
        if (n <= 0 || command == 's')
        {
            return nil;
        }
        eventfixturepublish(command);
    }
}


/* EN/RU: Native readiness is registered only while reads wait, preventing level-trigger busy loops.
 * Native readiness регистрируется только для ожидающих reads, без level-trigger busy loops. */
int main(void)
{
    Styxops ops;
    pthread_t worker;
    struct sockaddr_in address;
    socklen_t size = sizeof(address);
    memset(&ops, 0, sizeof(ops));
    ops.walk = walk;
    ops.open = openfile;
    ops.read = readfile;
    ops.stat = statfile;
    ops.write = writefile;
    ops.freefid = styxfileclose;
    backing = eventfixturecreate();
    check(styxinit(&server, &ops, "0", 0555, 0) == nil, "server init");
    check(!pthread_create(&worker, nil, producer, nil), "native producer");
    check(!getsockname(server.connfd, (struct sockaddr *)&address, &size), "port");
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        check(styxwait(&server) == nil && styxprocess(&server) == nil, "event loop");
        complete();
        check(!styxnotifyfd(&server, head ? eventfixturefd() : -1), "native readiness registration");
    }
    check(styxend(&server) == nil && !pthread_join(worker, nil) && !head, "native teardown");
    eventfixturefree();
    print("PASS real event resource/ordinary provider/native producer\n");
    return 0;
}
