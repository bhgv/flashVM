/* Copyright (c) 2026 bhgv
 * EN: Original libstyx TCP dispatch with per-client publication admission and roots.
 * RU: Исходный libstyx TCP dispatch с admission публикации и корнями на клиента.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "peer.h"
#include "tree.h"

typedef struct Session Session;
struct Session
{
    StyxPeer *peer; /* Owned native admission, no packet credentials. */
    StyxBacking *root; /* Owned selected root. */
    StyxBacking *file; /* Owned visited fixture node; same Qid in every session. */
};
static Styxserver server; /* One isolated native test owner. */
static unsigned serial; /* Test-only local slot selection, never authentication. */
static int stopping;
extern StyxPeer *newpeer(unsigned slot);
extern void closepublisher(void);


/* EN/RU: Borrow only the currently dispatched connection state.
 * Заимствовать только состояние текущего соединения. */
static Session *session(void)
{
    Client *client = styxclient(&server);
    return client ? client->u : nil;
}


/* EN/RU: Host assigns a local fixture grant before reading any wire identity.
 * Host назначает local grant теста до чтения wire identity. */
static char *newclient(Client *client)
{
    Session *state = calloc(1, sizeof(*state));
    client->u = state;
    if(!state) return Enomem;
    state->peer = newpeer(serial++);
    return state->peer ? nil : Eperm;
}


/* EN/RU: libstyx has already closed all fid contexts before this callback.
 * libstyx уже закрыл все fid contexts до этого callback. */
static char *freeclient(Client *client)
{
    Session *state = client->u;
    if(state){
        styxbackingfree(state->file);
        styxbackingfree(state->root);
        styxpeerfree(state->peer);
        free(state);
    }
    return nil;
}


/* EN/RU: uname is an untrusted hint; only the native peer can request a root.
 * uname — недоверенная подсказка; корень запрашивает только native peer. */
static char *attach(char *uname, char *aname)
{
    Session *state = session();
    StyxBacking *root;
    int status;
    USED(uname);
    status = styxpeerattach(state->peer, aname, 2, &root);
    if(status != 1) return status == 0 ? "publication pending" : Eperm;
    styxbackingfree(state->root);
    state->root = root;
    return nil;
}


/* EN/RU: Keep visited nodes per client, even when providers expose equal IDs.
 * Хранить посещённые nodes на клиента даже при одинаковых IDs providers. */
static char *walk(Qid *qid, char *name)
{
    Session *state = session();
    StyxBacking *node;
    if(qid->path != Qroot) return Enonexist;
    if(!strcmp(name, "ctl")){ *qid = styxqid(3, 0); return nil; }
    node = styxwalknode(state->root, name);
    if(!node) return Enonexist;
    if(!strcmp(name, "file")){
        styxbackingfree(state->file);
        state->file = node;
        *qid = styxqid(2, 0);
    }else{
        styxbackingfree(node);
        return Ebadarg;
    }
    return nil;
}


/* EN/RU: Open pins one chosen provider independently of peer/publisher lifetime.
 * Open удерживает выбранный provider независимо от peer/publisher lifetime. */
static char *openfile(Qid *qid, int mode)
{
    StyxOpen *opened;
    if(qid->path == 3) return mode == OWRITE ? nil : Eperm;
    if(qid->path != 2) return Eperm;
    opened = styxfileopen(session()->file, mode);
    if(!opened) return Eperm;
    if(styxsetaux(&server, opened) < 0){ styxfileclose(opened); return Eperm; }
    return nil;
}


/* EN/RU: Reads operate on the retained fid, without looking up a different root.
 * Читать удержанный fid без поиска в другом корне. */
static char *readfile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    StyxTransfer result;
    if(qid.path != 2 || offset < 0) return Eperm;
    styxfileread(styxgetaux(&server), offset, bytes, *n, &result);
    if(!result.confirmed || result.uncertain || (!result.success && !result.count)) return Eperm;
    *n = result.count;
    return nil;
}


/* EN/RU: Native fixture controls are not part of the exported admission API.
 * Native-управление тестом не является частью exported admission API. */
static char *writefile(Qid qid, char *bytes, ulong *n, vlong offset)
{
    if(qid.path != 3 || offset != 0) return Eperm;
    if(*n == 5 && !memcmp(bytes, "close", 5)) closepublisher();
    else if(*n == 4 && !memcmp(bytes, "stop", 4)) stopping = 1;
    else return Ebadarg;
    return nil;
}


/* EN/RU: Exercise the real POSIX TCP server; shutdown consumes all client roots.
 * Проверить настоящий POSIX TCP server; shutdown поглощает все корни клиентов. */
int peerwire(void)
{
    Styxops ops;
    struct sockaddr_in address;
    socklen_t size = sizeof(address);
    memset(&ops, 0, sizeof(ops));
    ops.newclient = newclient;
    ops.freeclient = freeclient;
    ops.attach = attach;
    ops.walk = walk;
    ops.open = openfile;
    ops.read = readfile;
    ops.write = writefile;
    ops.freefid = styxfileclose;
    if(styxinit(&server, &ops, "0", 0555, 0) != nil) return 1;
    if(getsockname(server.connfd, (struct sockaddr*)&address, &size) < 0) return 2;
    print("%d\n", ntohs(address.sin_port));
    while(!stopping) if(styxwait(&server) != nil || styxprocess(&server) != nil) return 3;
    return styxend(&server) == nil ? 0 : 4;
}
