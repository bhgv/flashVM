/* Copyright (c) 2026 bhgv
 * EN: Original Inferno libstyx serves a granted movie file over real TCP for acceptance.
 * RU: Original Inferno libstyx обслуживает granted movie file через настоящий TCP.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "file.h"
#include "tree.h"
#include "../offer_ids.h"
extern int movie_test_end(const void *bytes, size_t size); /* Scalar test-only codec boundary. */
static Styxserver server; /* Socket-owner state, no guest VM execution. */
static StyxBacking *backing; /* Borrowed through movieserve. */
static int fault_reads; /* Number of native reads used by deliberate framing faults. */
static int stopped; /* Client disconnect terminates this bounded fixture. */


/* EN/RU: Select exactly the granted file root; attach hints never choose another namespace.
 * Выбрать только granted file root; attach hints не выбирают другой namespace. */
static char *attach(char *user, char *name)
{
    USED(user);
    if (getenv("MOVIE_TEST_ANNOUNCE"))
    {
        if (!*name)
        {
            return styxattachroot(&server, nil, styxqid(Qroot, 1)) ? Eperm : nil;
        }
        return Eperm; /* Existing libstyx connection identity remains unchanged. */
    }
    USED(name);
    return styxattachroot(&server, nil, styxqid(42, 0)) ? Eperm : nil;
}


/* EN/RU: Walk only the scoped virtual directory; no mount, dial or attach identity changes.
 * Пройти scoped virtual directory без mount/dial/смены attach identity. */
static char *walk(Qid *qid, char *name)
{
    if (!getenv("MOVIE_TEST_ANNOUNCE"))
    {
        return Enonexist;
    }
    StyxBacking *parent = nil;
    if(qid->path == Qroot || qid->path == STYX_MOVIE_ROOT_ID)
    {
        parent = backing;
    }
    else if(qid->path == STYX_MOVIE_DIRECTORY_ID)
    {
        parent = styxwalknode(backing, "movie");
    }
    StyxBacking *child = parent ? styxwalknode(parent, name) : nil;
    if(parent != backing)
    {
        styxbackingfree(parent);
    }
    StyxMeta info;
    int valid = child && styxbackinginfo(child, &info);
    styxbackingfree(child);
    if (!valid)
    {
        return Enonexist;
    }
    *qid = styxqid(info.identity, info.directory);
    if (!info.directory && info.identity < STYX_MOVIE_ROOT_ID)
    {
        print("WALK movie/%llud\n", info.identity);
    }
    return nil;
}


/* EN/RU: Open an independent native stream and transfer it to the current fid.
 * Открыть независимый native stream и передать его current fid. */
static char *openfile(Qid *qid, int mode)
{
    (void)qid;
    StyxBacking *selected = nil;
    if (getenv("MOVIE_TEST_ANNOUNCE"))
    {
        if(qid->path == STYX_MOVIE_CONTROL_ID)
        {
            selected = styxwalknode(backing, "ctl");
        }
        else
        {
            char name[64];
            snprint(name, sizeof(name), "%llud", qid->path);
            StyxBacking *directory = styxwalknode(backing, "movie");
            selected = directory ? styxwalknode(directory, name) : nil;
            styxbackingfree(directory);
        }
        if (!selected)
        {
            return Eperm;
        }
    }
    StyxOpen *opened = styxfileopen(selected ? selected : backing, mode);
    styxbackingfree(selected);
    if (!opened)
    {
        return Eperm;
    }
    if (styxsetaux(&server, opened))
    {
        styxfileclose(opened);
        return Eperm;
    }
    return nil;
}


/* EN/RU: Preserve exact native acknowledgement, including partial read/write prefixes.
 * Сохранить точное native acknowledgement, включая partial prefixes. */
static char *result(StyxTransfer transfer, ulong *count)
{
    if (!transfer.confirmed || transfer.uncertain || (!transfer.success && !transfer.count))
    {
        return Eperm;
    }
    *count = transfer.count;
    return nil;
}


/* EN/RU: Dispatch one native read from the current opened fid with its full offset.
 * Выполнить один native read current fid по полному offset. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer transfer;
    (void)qid;
    char *fault = getenv("MOVIE_TEST_FAULT");
    if (fault && (!strcmp(fault, "eof") || (!strcmp(fault, "truncate") && fault_reads++)))
    {
        *count = 0;
        return nil;
    }
    ulong capacity = *count;
    styxfileread(styxgetaux(&server), offset, bytes, *count, &transfer);
    char *error = result(transfer, count);
    if (!error && fault && *count)
    {
        if (!strcmp(fault, "coalesce") && *count < capacity)
        {
            StyxTransfer next;
            ulong count2 = capacity-*count;
            styxfileread(styxgetaux(&server), offset+*count, bytes+*count, count2, &next);
            char *second = result(next, &count2);
            if (second)
            {
                return second;
            }
            *count += count2;
        }
        if (!strcmp(fault, "noend") && movie_test_end(bytes, *count))
        {
            *count = 0;
        }
        if (!strcmp(fault, "crc"))
        {
            bytes[0] ^= 1;
        }
        if (!strcmp(fault, "truncate") && *count > 5)
        {
            *count = 5;
        }
    }
    return error;
}


/* EN/RU: Dispatch one native write; no acknowledgement is fabricated on failure.
 * Выполнить один native write без выдуманных acknowledgements при отказе. */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxTransfer transfer;
    (void)qid;
    styxfilewrite(styxgetaux(&server), offset, bytes, *count, &transfer);
    return result(transfer, count);
}


/* EN/RU: End the bounded fixture after original libstyx retires the disconnected client.
 * Завершить bounded fixture после retirement отключившегося клиента в libstyx. */
static char *closed(Client *client)
{
    USED(client);
    stopped = 1;
    return nil;
}


/* EN/RU: Borrow granted backing until TCP client retirement; return explicit native status.
 * Fixture binds loopback-capable local server without authentication; production host owns admission.
 * Borrow backing до retirement TCP client; fixture без authentication, admission принадлежит host. */
int movieserve(StyxBacking *root)
{
    Styxops ops = {0};
    struct sockaddr_in address;
    socklen_t size = sizeof(address);
    backing = root;
    ops.attach = attach;
    ops.walk = walk;
    ops.open = openfile;
    ops.read = readfile;
    ops.write = writefile;
    ops.freefid = styxfileclose;
    ops.freeclient = closed;
    if (styxinit(&server, &ops, "0", getenv("MOVIE_TEST_ANNOUNCE") ? 0555 : 0666, 0) ||
        getsockname(server.connfd, (struct sockaddr *)&address, &size))
    {
        return 2;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopped)
    {
        if (styxwait(&server) || styxprocess(&server))
        {
            return 3;
        }
    }
    return styxend(&server) ? 4 : 0;
}
