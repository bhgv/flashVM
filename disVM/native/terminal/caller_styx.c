/* Copyright (c) 2026 bhgv
 * EN: Export the active caller capability through ordinary 9P operations.
 * RU: Экспортировать активный контекст клиента обычными операциями 9P.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "caller_styx.h"
#include "caller_scope.h"
#include "server.h"
#include "file_provider.h"
#include "mount.h"
#include "tree.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PROG_QID 1U /* Permanent virtual directory for active process IDs. */

typedef struct CallerNodes
{
    uint64_t generation; /* Publication represented by this dispatcher cache. */
    size_t count; /* Number of visited nodes for this publication. */
    StyxBacking *nodes[STYX_CALLER_NODES]; /* Owned visited nodes. */
} CallerNodes;

struct StyxCallerServer
{
    Styxserver server; /* Owned original Styx dispatcher. */
    StyxPhyWake *wake; /* Owned dispatcher wake pin through mount retirement. */
    pthread_t worker; /* Owns dispatcher, nodes and stat scratch. */
    StyxCallerScopes scopes; /* Concurrent per-PID publications. */
    size_t references; /* Control and bound mount owners, under scopes.lock. */
    CallerNodes cache[STYX_CALLER_SLOTS]; /* Dispatcher-owned node caches. */
    StyxMeta stat; /* Borrowed by Rstat encoding until callback completes. */
};

typedef struct CallerOpen
{
    StyxOpen *file; /* Exclusive opened VFS cursor. */
    uint64_t generation; /* Publication which granted this open, zero for a portal snapshot. */
} CallerOpen;

static __thread StyxCallerServer *owner; /* Current dispatcher on its native thread. */


/* EN: Publish a caller tree for its PID until the matching release.
 * RU: Опубликовать дерево клиента для его PID до освобождения. */
static void *begin(void *state, DVM_Provider *target, DVM_Provider *tree, uint32_t pid)
{
    StyxCallerServer *server = state;
    (void)target;
    return styxcallerscopeadd(&server->scopes, tree, pid) ? tree : NULL;
}


/* EN: Revoke this invocation without affecting other PIDs.
 * RU: Отозвать этот вызов, сохранив остальные PID. */
static void end(void *state, void *lease)
{
    StyxCallerServer *server = state;
    styxcallerscopedrop(&server->scopes, lease);
    styxphywake(server->wake);
}


/* EN: Release visited nodes from one dispatcher cache.
 * RU: Освободить посещённые узлы одного кеша диспетчера. */
static void clear(CallerNodes *cache)
{
    for (size_t i = 0; i < cache->count; i++)
    {
        styxbackingfree(cache->nodes[i]);
    }
    cache->count = 0;
}


/* EN: Borrow a previously walked node only from its live generation.
 * RU: Заимствовать пройденный узел только из его активного поколения. */
static StyxBacking *find(Path path, CallerNodes **cache)
{
    uint64_t generation = path / STYX_CALLER_NODES;
    size_t offset = path % STYX_CALLER_NODES;
    int index = generation ? styxcallerscopefind(&owner->scopes, 0, generation, NULL) : -1;
    if (index < 0)
    {
        return NULL;
    }
    *cache = &owner->cache[index];
    return (*cache)->generation == generation && offset < (*cache)->count ? (*cache)->nodes[offset] : NULL;
}


/* EN: Sign a child node with the generation that grants access to it.
 * RU: Привязать дочерний узел к поколению, выдавшему доступ. */
static char *walk_child(Qid *qid, const char *name, StyxBacking *parent, CallerNodes *cache)
{
    if (!parent || cache->count == STYX_CALLER_NODES)
    {
        return Enonexist;
    }
    StyxBacking *node = styxwalknode(parent, name);
    StyxMeta meta;
    if (!node || !styxbackinginfo(node, &meta))
    {
        styxbackingfree(node);
        return Enonexist;
    }
    *qid = styxqid(cache->generation * STYX_CALLER_NODES + cache->count, meta.directory);
    cache->nodes[cache->count++] = node;
    return nil;
}


/* EN: Find one canonical decimal PID and enter its pinned virtual directory.
 * RU: Найти канонический десятичный PID и войти в удерживаемый каталог. */
static char *walk_pid(Qid *qid, const char *name)
{
    char *end;
    errno = 0;
    unsigned long pid = strtoul(name, &end, 10);
    if (errno || *name < '1' || *name > '9' || *end || pid > UINT32_MAX)
    {
        return Enonexist;
    }
    StyxCallerGrant grant;
    int index = styxcallerscopefind(&owner->scopes, (uint32_t)pid, 0, &grant);
    if (index < 0)
    {
        return Enonexist;
    }
    CallerNodes *cache = &owner->cache[index];
    if (cache->generation != grant.generation)
    {
        clear(cache);
        cache->generation = grant.generation;
    }
    if (cache->count)
    {
        dvm_provider_release(grant.tree);
        *qid = styxqid(grant.generation * STYX_CALLER_NODES, 1);
        return nil;
    }
    StyxBacking *root = styxtree(grant.tree);
    dvm_provider_release(grant.tree);
    StyxBacking *prog = root ? styxwalknode(root, ".prog") : NULL;
    char *error = walk_child(qid, name, prog, cache);
    styxbackingfree(prog);
    styxbackingfree(root);
    return error;
}


/* EN: Walk through the shared portal or within one live generation.
 * RU: Пройти общий каталог либо узел одного живого поколения. */
static char *walk(Qid *qid, char *name)
{
    if (qid->path == Qroot)
    {
        if (!strcmp(name, ".prog"))
        {
            *qid = styxqid(PROG_QID, 1);
            return nil;
        }
        return Enonexist;
    }
    if (qid->path == PROG_QID)
    {
        if (!strcmp(name, ".."))
        {
            *qid = styxqid(Qroot, 1);
            return nil;
        }
        return walk_pid(qid, name);
    }
    CallerNodes *cache = NULL;
    StyxBacking *parent = find(qid->path, &cache);
    if (parent && qid->path % STYX_CALLER_NODES == 0 && !strcmp(name, ".."))
    {
        *qid = styxqid(PROG_QID, 1);
        return nil;
    }
    return parent ? walk_child(qid, name, parent, cache) : Enonexist;
}


/* EN: Build an owned snapshot for opening or stating a portal directory.
 * RU: Собрать свой снимок для открытия или stat общего каталога. */
static StyxBacking *portal(Path path)
{
    DVM_Provider *view = styxcallerscopeview(&owner->scopes);
    StyxBacking *root = view ? styxtree(view) : NULL;
    dvm_provider_release(view);
    if (path == Qroot || !root)
    {
        return root;
    }
    StyxBacking *prog = styxwalknode(root, ".prog");
    styxbackingfree(root);
    return prog;
}


/* EN: Consume the native cursor and its fid-owned generation token.
 * RU: Освободить курсор и поколение, закреплённое за fid. */
static void close_node(void *state)
{
    CallerOpen *opened = state;
    styxfileclose(opened->file);
    free(opened);
}


/* EN: Open one node and pin the cursor and its generation on the fid.
 * RU: Открыть узел и удержать курсор и поколение в fid. */
static char *open_node(Qid *qid, int mode)
{
    int shared = qid->path <= PROG_QID;
    CallerNodes *cache = NULL;
    StyxBacking *node = shared ? portal(qid->path) : find(qid->path, &cache);
    if (!node)
    {
        return Enonexist;
    }
    CallerOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        if (shared)
        {
            styxbackingfree(node);
        }
        return Enomem;
    }
    opened->generation = cache ? cache->generation : 0;
    DVM_ProviderOpenScope previous = dvm_provider_open_scope(
        (DVM_ProviderOpenScope){owner, opened->generation});
    opened->file = styxfileopen(node, mode);
    dvm_provider_open_scope(previous);
    if (shared)
    {
        styxbackingfree(node);
    }
    if (!opened->file || styxsetaux(&owner->server, opened) < 0)
    {
        close_node(opened);
        return Eperm;
    }
    return nil;
}


/* EN: Borrow the fid's cursor only while the grant remains active.
 * RU: Заимствовать курсор fid пока разрешение активно. */
static StyxOpen *active_open(void)
{
    CallerOpen *opened = styxgetaux(&owner->server);
    int valid = opened && (!opened->generation ||
        styxcallerscopefind(&owner->scopes, 0, opened->generation, NULL) >= 0);
    return valid ? opened->file : NULL;
}


/* EN: Stat the opened resource or current visited node.
 * RU: Получить stat открытого файла или текущего узла поиска. */
static char *stat_node(Qid qid, Dir *dir)
{
    StyxOpen *opened = active_open();
    int valid;
    if (styxgetaux(&owner->server))
    {
        valid = opened && styxfileinfo(opened, &owner->stat);
    }
    else if (qid.path <= PROG_QID)
    {
        StyxBacking *node = portal(qid.path);
        valid = styxbackinginfo(node, &owner->stat);
        styxbackingfree(node);
    }
    else
    {
        CallerNodes *cache;
        valid = styxbackinginfo(find(qid.path, &cache), &owner->stat);
    }
    if (!valid)
    {
        return Enonexist;
    }
    styxcallerattributes(&owner->stat, qid, dir);
    return nil;
}


/* EN: Transfer one confirmed read or return a Styx error.
 * RU: Подтвердить единичное чтение либо вернуть ошибку Styx. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    StyxOpen *opened = active_open();
    if (!opened)
    {
        return Eperm;
    }
    if (qid.type & QTDIR)
    {
        return styxcallerlist(opened, bytes, count, offset);
    }
    StyxTransfer result;
    if (offset < 0)
    {
        return Ebadarg;
    }
    styxfileread(opened, offset, bytes, *count, &result);
    if (!result.confirmed || result.uncertain || (!result.success && !result.count))
    {
        return result.again ? "resource temporarily unavailable" : Eperm;
    }
    *count = result.count;
    return nil;
}


/* EN: Transfer one confirmed write using the mode fixed at open.
 * RU: Подтвердить единичную запись в режиме, зафиксированном при открытии. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    (void)qid;
    StyxOpen *opened = active_open();
    if (!opened)
    {
        return Eperm;
    }
    StyxTransfer result;
    if (offset < 0)
    {
        return Ebadarg;
    }
    styxfilewrite(opened, offset, bytes, *count, &result);
    if (!result.confirmed || result.uncertain || (!result.success && !result.count))
    {
        return result.again ? "resource temporarily unavailable" : Eperm;
    }
    *count = result.count;
    return nil;
}

static Styxops operations = { /* Ordinary Styx operations, with fid-owned opened aliases. */
    .walk = walk, .open = open_node, .stat = stat_node, .read = read_node, .write = write_node,
    .freefid = close_node
};


/* EN: Run the server until its carrier closes, then retire all owned nodes.
 * RU: Обслуживать сервер до закрытия транспорта и освободить узлы. */
static void *serve(void *state)
{
    owner = state;
    while (!styxwait(&owner->server) && !styxprocess(&owner->server))
    {
        for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
        {
            CallerNodes *cache = &owner->cache[i];
            if (cache->count && styxcallerscopefind(&owner->scopes, 0, cache->generation, NULL) < 0)
            {
                clear(cache);
            }
        }
    }
    styxend(&owner->server);
    for (size_t i = 0; i < STYX_CALLER_SLOTS; i++)
    {
        clear(&owner->cache[i]);
    }
    pthread_mutex_lock(&owner->scopes.lock);
    owner->scopes.stopped = 1;
    pthread_mutex_unlock(&owner->scopes.lock);
    owner = NULL;
    return NULL;
}


/* EN/RU: See caller_styx.h. The binding is installed only on a fresh mount.
 * См. caller_styx.h. Связь устанавливается только на свежий маунт. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse)
{
    StyxCallerServer *server = calloc(1, sizeof(*server));
    if (!server || pthread_mutex_init(&server->scopes.lock, NULL))
    {
        free(server);
        phyfree(reverse);
        return NULL;
    }
    server->references = 1;
    if (styxphyinit(&server->server, &operations, reverse, 0555, 0, 8))
    {
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    server->wake = styxphywakepin(&server->server);
    if (!server->wake || pthread_create(&server->worker, NULL, serve, server))
    {
        styxend(&server->server);
        styxphywakefree(server->wake);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    return server;
}


/* EN: Release one control or provider reference without joining the worker.
 * RU: Снять ссылку управления или провайдера без ожидания потока. */
static void drop(void *state)
{
    StyxCallerServer *server = state;
    pthread_mutex_lock(&server->scopes.lock);
    int last = --server->references == 0;
    pthread_mutex_unlock(&server->scopes.lock);
    if (last)
    {
        styxphywakefree(server->wake);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
    }
}


/* EN/RU: See caller_styx.h. On successful binding the provider owns a pin.
 * См. caller_styx.h. При успехе провайдер владеет ссылкой. */
int styxcallerbind(StyxCallerServer *server, DVM_Provider *forward)
{
    if (!server)
    {
        return 0;
    }
    pthread_mutex_lock(&server->scopes.lock);
    int retained = !server->scopes.stopped && server->references < (size_t)-1;
    if (retained)
    {
        server->references++;
    }
    pthread_mutex_unlock(&server->scopes.lock);
    if (!retained)
    {
        return 0;
    }
    DVM_CallerBinding binding = {.begin = begin, .end = end, .state = server, .release = drop};
    if (!dvm_provider_bind_caller(forward, &binding))
    {
        drop(server);
        return 0;
    }
    return 1;
}


/* EN/RU: See caller_styx.h. Do not expose a partially configured mount.
 * См. caller_styx.h. Публиковать только полностью настроенный маунт. */
DVM_Provider *styxcallermount(StyxCallerServer *server, StyxClient *forward, unsigned access)
{
    DVM_Provider *mount = styxmountprovider(forward, access);
    if (!mount || !styxcallerbind(server, mount))
    {
        dvm_provider_release(mount);
        return NULL;
    }
    return mount;
}


/* EN/RU: See caller_styx.h. The references held by mounts retire independently.
 * См. caller_styx.h. Ссылки маунтов освобождаются независимо. */
void styxcallerjoin(StyxCallerServer *server)
{
    if (server)
    {
        pthread_join(server->worker, NULL);
        drop(server);
    }
}
