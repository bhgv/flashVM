/* Copyright (c) 2026 bhgv
 * EN: Original C libstyx deep hierarchy for cancellable metadata/walk/open RPCs.
 * RU: Исходный C libstyx deep hierarchy для cancellable metadata/walk/open RPCs.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>

static Styxserver server; /* Isolated native test owner. */
static int stopping; /* Explicit native fixture stop. */
static int closed; /* Opened remote fids retired exactly once. */
static int clients; /* Client backend retirement count. */


/* EN/RU: One component at a time, seventeen directories then a file.
 * По одному component: семнадцать каталогов, затем файл. */
static char *walk(Qid *qid, char *name)
{
    if (qid->path == 0 && !strcmp(name, "ctl"))
    {
        *qid = styxqid(100, 0);
        return nil;
    }
    if (qid->path < 17 && !strcmp(name, "d"))
    {
        *qid = styxqid(qid->path+1, 1);
        return nil;
    }
    if (qid->path == 17 && !strcmp(name, "file"))
    {
        *qid = styxqid(18, 0);
        return nil;
    }
    return Enonexist;
}


/* EN/RU: Permit only the explicit fixture file/control open modes.
 * Разрешить только явные file/control open modes теста. */
static char *openfile(Qid *qid, int mode)
{
    return (qid->path == 18 && mode == OREAD) || (qid->path == 100 && mode == OWRITE) ? nil : Eperm;
}


/* EN/RU: Return ordinary metadata with the requested remote identity.
 * Вернуть обычные metadata с запрошенной remote identity. */
static char *statfile(Qid qid, Dir *out)
{
    memset(out, 0, sizeof(*out));
    out->qid = qid;
    out->name = qid.path == 18 ? "file" : qid.path == 100 ? "ctl" : "d";
    out->uid = out->gid = out->muid = "fixture";
    out->length = qid.path == 18 ? 5 : 0;
    out->mode = qid.path < 18 ? DMDIR|0555 : qid.path == 18 ? 0444 : 0222;
    return nil;
}


/* EN/RU: Real Tread uses retained fid identity and an explicit64-bit offset.
 * Настоящий Tread использует удержанный fid и explicit64-bit offset. */
static char *readfile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    if (qid.path != 18 || offset < 0)
    {
        return Eperm;
    }
    *n = styxreadstr(offset, bytes, *n, "depth");
    return nil;
}


/* EN/RU: Explicit test control, not a production publication command.
 * Явное управление тестом, не production publication command. */
static char *writefile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    if (qid.path != 100 || offset != 0 || *n != 4 || memcmp(bytes, "stop", 4))
    {
        return Eperm;
    }
    stopping = 1;
    return nil;
}


/* EN/RU: Count successful-open retirement; unopened partial walks do not call close.
 * Учесть successful-open retirement; unopened partial walks не вызывают close. */
static char *closefile(Qid qid, int mode)
{
    USED(qid.path);
    USED(mode);
    closed++;
    return nil;
}


/* EN/RU: Count final backend retirement after all fids have gone.
 * Учесть финальный backend retirement после всех fid. */
static char *freeclient(Client *client)
{
    USED(client);
    clients++;
    stopping = 1;
    return nil;
}


/* EN/RU: Run original POSIX Styx TCP and require exact open/client retirement.
 * Запустить исходный POSIX Styx TCP с проверкой open/client retirement. */
int main(void)
{
    Styxops ops;
    struct sockaddr_in address;
    socklen_t length = sizeof(address);
    memset(&ops, 0, sizeof(ops));
    ops.walk = walk;
    ops.open = openfile;
    ops.stat = statfile;
    ops.read = readfile;
    ops.write = writefile;
    ops.close = closefile;
    ops.freeclient = freeclient;
    if (styxinit(&server, &ops, "0", 0555, 0) != nil)
    {
        return 1;
    }
    if (getsockname(server.connfd, (struct sockaddr *)&address, &length) < 0)
    {
        return 2;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        if (styxwait(&server) != nil || styxprocess(&server) != nil)
        {
            return 3;
        }
    }
    if (styxend(&server) != nil || closed < 1 || clients != 1)
    {
        return 4;
    }
    print("PASS control server: closed %d handles, released 1 client\n", closed);
    return 0;
}
