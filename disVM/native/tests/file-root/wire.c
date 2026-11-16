/* Copyright (c) 2026 bhgv
 * EN: Original C Styx server selecting a true file Rattach root, with native deferred preparation.
 * RU: Original C Styx server с настоящим file Rattach root и native deferred preparation.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "backend.h"
#include "file.h"


/* EN/RU: Connection-local grant and pending attach lifetime, exclusively on socket owner.
 * Connection-local grant/pending attach lifetime исключительно на socket owner. */
typedef struct Session
{
    StyxPeer *peer; /* Owned native publication requester. */
    StyxBacking *root; /* Owned selected single-file capability. */
    StyxAttach *pending; /* Owned deferred wire transaction until retirement callback. */
} Session;
static Styxserver server; /* Original server/socket-owner state. */
static int stopping; /* Finish fixture after its native client disconnects. */
extern int reviewfd(void);
extern int reviewdrain(void);


/* EN/RU: Borrow currently dispatched connection context, never across worker threads.
 * Заимствовать current connection context, никогда не передавать worker threads. */
static Session *session(void)
{
    return styxclient(&server)->u;
}


/* EN/RU: Create one locally authorized native peer before wire attach identity is read.
 * Создать locally authorized native peer до чтения wire attach identity. */
static char *newclient(Client *client)
{
    Session *state = calloc(1, sizeof(*state));
    client->u = state;
    if (!state)
    {
        return Enomem;
    }
    state->peer = filerootpeer();
    return state->peer ? nil : Eperm;
}


/* EN/RU: Dispatcher retires pending requests/fids before connection state release.
 * Dispatcher завершает pending requests/fids до освобождения connection state. */
static char *freeclient(Client *client)
{
    Session *state = client->u;
    if (state)
    {
        styxbackingfree(state->root);
        styxpeerfree(state->peer);
        free(state);
    }
    stopping = 1;
    return nil;
}


/* EN/RU: Cancellation/success first retires native ticket, then caller's wire request handle.
 * Cancellation/success сначала завершает native ticket, затем caller wire request handle. */
static void retired(void *opaque)
{
    Session *state = opaque;
    styxpeercancel(state->peer);
    styxattachfree(state->pending);
    state->pending = nil;
}


/* EN/RU: Attach the logical file directly or defer for native preparation; wire uname grants no rights.
 * Attach logical file напрямую либо defer для native preparation; wire uname не выдаёт права. */
static char *attach(char *uname, char *aname)
{
    USED(uname);
    Session *state = session();
    int status = styxpeerattach(state->peer, aname, 1, &state->root);
    if (!status)
    {
        state->pending = styxdeferattach(&server, state, retired);
        return state->pending ? nil : Enomem;
    }
    return status == 1 && !styxattachroot(&server, nil, styxqid(42, 0)) ? nil : Eperm;
}


/* EN/RU: Complete only after native notification, on owner; workers never touch request/fid pointers.
 * Завершить после native notification на owner; workers не касаются request/fid pointers. */
static void complete(void)
{
    for (Client *client = server.clients; client; client = client->next)
    {
        Session *state = client->u;
        if (!state || !state->pending)
        {
            continue;
        }
        int status = styxpeerattach(state->peer, "file", 1, &state->root);
        if (status)
        {
            int selected = status == 1 && !styxattachroot(&server, state->pending, styxqid(42, 0));
            styxattachreply(state->pending, selected ? nil : Eperm);
        }
    }
}


/* EN/RU: Every zero-walk clone opens the same granted file with an independent native cursor.
 * Каждый zero-walk clone открывает granted file с independent native cursor. */
static char *openfile(Qid *qid, int mode)
{
    StyxOpen *opened = qid->path == 42 ? styxfileopen(session()->root, mode) : nil;
    if (!opened)
    {
        return Eperm;
    }
    if (styxsetaux(&server, opened) < 0)
    {
        styxfileclose(opened);
        return Enomem;
    }
    return nil;
}


/* EN/RU: Ordinary held-fid file reads preserve exact SDK byte acknowledgement.
 * Ordinary held-fid reads сохраняют exact SDK byte acknowledgement. */
static char *readfile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    (void)qid;
    StyxTransfer result;
    styxfileread(styxgetaux(&server), offset, bytes, *count, &result);
    *count = result.count;
    return result.success ? nil : Eperm;
}


/* EN/RU: Ordinary writes affect the selected file only; no replay or directory projection.
 * Ordinary writes меняют только selected file; без replay/directory projection. */
static char *writefile(Qid qid, char *bytes, ulong *count, vlong offset)
{
    (void)qid;
    StyxTransfer result;
    styxfilewrite(styxgetaux(&server), offset, bytes, *count, &result);
    *count = result.count;
    return result.success ? nil : Eperm;
}


/* EN/RU: Metadata has file Qid/type through stat and attach; no artificial exported directory.
 * Metadata имеет file Qid/type в stat/attach; без искусственного exported directory. */
static char *statfile(Qid qid, Dir *out)
{
    StyxMeta meta;
    if (!styxbackinginfo(session()->root, &meta))
    {
        return Enonexist;
    }
    memset(out, 0, sizeof(*out));
    out->qid = qid;
    out->mode = meta.mode;
    out->length = meta.length;
    out->name = ".";
    out->uid = out->gid = out->muid = "fixture";
    return nil;
}


/* EN/RU: Run direct/prepared file-root publication on original POSIX transport until client retirement.
 * Выполнить direct/prepared file-root publication на original POSIX transport до client retirement. */
int main(int argc, char **argv)
{
    Styxops ops = {0};
    struct sockaddr_in address;
    socklen_t size = sizeof(address);
    filerootsetup(argc > 1 && !strcmp(argv[1], "prepare"));
    ops.newclient = newclient;
    ops.freeclient = freeclient;
    ops.attach = attach;
    ops.open = openfile;
    ops.read = readfile;
    ops.write = writefile;
    ops.stat = statfile;
    ops.freefid = styxfileclose;
    if (styxinit(&server, &ops, "0", 0555, 0) || getsockname(server.connfd, (struct sockaddr *)&address, &size))
    {
        return 2;
    }
    if (reviewfd() >= 0 && styxnotifyfd(&server, reviewfd()))
    {
        return 3;
    }
    print("%d\n", ntohs(address.sin_port));
    while (!stopping)
    {
        if (styxwait(&server) || styxprocess(&server))
        {
            return 4;
        }
        if (reviewdrain())
        {
            complete();
        }
    }
    if (styxend(&server))
    {
        return 5;
    }
    filerootend();
    return 0;
}
