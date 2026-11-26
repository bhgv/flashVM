/* Copyright (c) 2026 bhgv
 * EN: Export the active caller capability through ordinary 9P operations.
 * RU: Экспортировать активный контекст клиента обычными операциями 9P.
 */
#include "caller_styx_state.h"
#include "file_provider.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


typedef struct CallerOpen
{
    StyxOpen *file; /* Exclusive opened VFS cursor. */
    uint64_t generation; /* Publication which granted this open, zero for a portal snapshot. */
} CallerOpen;

static __thread StyxCallerServer *owner; /* Current dispatcher on its native thread. */


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
    if (!generation && path > PROG_QID)
    {
        *cache = &owner->permanent;
        return offset < (*cache)->count ? (*cache)->nodes[offset] : NULL;
    }
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
    if (!parent)
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
    if (cache == &owner->permanent && styxbackingsamenode(owner->shared_root, node))
    {
        styxbackingfree(node);
        *qid = styxqid(Qroot, 1);
        return nil;
    }
    for (size_t i = 0; i < cache->count; i++)
    {
        if (styxbackingsamenode(cache->nodes[i], node))
        {
            styxbackingfree(node);
            *qid = styxqid(cache->generation * STYX_CALLER_NODES + i, meta.directory);
            return nil;
        }
    }
    if (cache->count == STYX_CALLER_NODES)
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
        StyxBacking *root = styxtree(owner->shared);
        styxbackingfree(owner->shared_root);
        owner->shared_root = root;
        return walk_child(qid, name, root, &owner->permanent);
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
    DVM_Provider *view = styxcallerscopeview(&owner->scopes, owner->shared);
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


/* EN: Create a regular file in the original shared view. PID grants remain separate.
 * RU: Создать обычный файл в исходном общем дереве. Разрешения PID независимы. */
static char *create_node(Qid *qid, char *name, int permissions, int mode)
{
    if (!owner->creatable || mode != OWRITE || permissions != 0666 ||
        owner->permanent.count == STYX_CALLER_NODES || styxgetaux(&owner->server))
    {
        return Eperm;
    }
    CallerNodes *cache = NULL;
    StyxBacking *parent = qid->path == Qroot ? styxtree(owner->shared) : find(qid->path, &cache);
    CallerOpen *opened = NULL;
    StyxBacking *child = NULL;
    if (parent && (!cache || cache == &owner->permanent))
    {
        opened = calloc(1, sizeof(*opened));
        if (opened)
        {
            opened->file = styxcreatefile(parent, name, &child);
        }
    }
    if (qid->path == Qroot)
    {
        styxbackingfree(parent);
    }
    if (!opened)
    {
        return Eperm;
    }
    if (!opened->file || styxsetaux(&owner->server, opened) < 0)
    {
        styxbackingfree(child);
        close_node(opened);
        return Eperm;
    }
    *qid = styxqid(owner->permanent.count, 0);
    owner->permanent.nodes[owner->permanent.count++] = child;
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
    .walk = walk, .open = open_node, .create = create_node, .stat = stat_node, .read = read_node, .write = write_node,
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
    clear(&owner->permanent);
    styxbackingfree(owner->shared_root);
    dvm_provider_release(owner->shared);
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


/* EN: Create one owner with an optional trusted write-create provider.
 * RU: Создать владельца с необязательным доверенным провайдером создания. */
static StyxCallerServer *start_view(RemotePhy *reverse, DVM_Provider *shared, int creatable)
{
    StyxCallerServer *server = calloc(1, sizeof(*server));
    if (!server || pthread_mutex_init(&server->scopes.lock, NULL))
    {
        free(server);
        phyfree(reverse);
        return NULL;
    }
    if (shared)
    {
        server->shared_root = styxtree(shared);
        if (!server->shared_root || !dvm_provider_retain(shared))
        {
            styxbackingfree(server->shared_root);
            pthread_mutex_destroy(&server->scopes.lock);
            free(server);
            phyfree(reverse);
            return NULL;
        }
        server->shared = shared;
        server->creatable = creatable;
        server->permanent.count = PROG_QID + 1;
    }
    server->references = 1;
    if (styxphyinit(&server->server, &operations, reverse, creatable ? 0777 : 0555, 0, 8))
    {
        styxbackingfree(server->shared_root);
        dvm_provider_release(server->shared);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    server->wake = styxphywakepin(&server->server);
    if (!server->wake || pthread_create(&server->worker, NULL, serve, server))
    {
        styxend(&server->server);
        styxphywakefree(server->wake);
        styxbackingfree(server->shared_root);
        dvm_provider_release(server->shared);
        pthread_mutex_destroy(&server->scopes.lock);
        free(server);
        return NULL;
    }
    return server;
}



/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver_view(RemotePhy *reverse, DVM_Provider *shared)
{
    return start_view(reverse, shared, 0);
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver_view_create(RemotePhy *reverse, DVM_Provider *shared)
{
    return start_view(reverse, shared, 1);
}


/* EN/RU: See caller_styx.h. / См. caller_styx.h. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse)
{
    return styxcallerserver_view(reverse, NULL);
}
