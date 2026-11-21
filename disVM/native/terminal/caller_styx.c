/* Copyright (c) 2026 bhgv
 * EN: Export the active caller capability through ordinary 9P operations.
 * RU: Экспортировать активный контекст клиента обычными операциями 9P.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "caller_styx.h"
#include "server.h"
#include "file_provider.h"
#include "mount.h"
#include "tree.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define CALLER_NODES 512U /* Maximum distinct Qids issued for a single invocation. */

struct StyxCallerServer
{
    Styxserver server; /* Owned original Styx dispatcher. */
    pthread_t worker; /* Owns dispatcher, nodes and stat scratch. */
    pthread_mutex_t lock; /* Serializes publication and root snapshot. */
    size_t references; /* Control and bound mount owners, under lock. */
    int stopped; /* Worker finished and cannot publish again. */
    DVM_Provider *active; /* Borrowed from the synchronous call lease. */
    Path generation; /* Monotonic publication serial. */
    Path observed; /* Last generation observed by dispatcher. */
    Path base; /* First Qid for the current generation. */
    size_t count; /* Number of retained nodes in the current generation. */
    StyxBacking *nodes[CALLER_NODES]; /* Owned exact visited nodes. */
    StyxMeta stat; /* Borrowed by Rstat encoding until callback completes. */
};

typedef struct CallerOpen
{
    StyxOpen *file; /* Exclusive opened VFS cursor. */
    Path generation; /* Publication which granted this open. */
} CallerOpen;

static __thread StyxCallerServer *owner; /* Current dispatcher on its native thread. */


/* EN: Publish one call root until the matching end. Refuse overlap without mutation.
 * RU: Опубликовать корень на время вызова. Параллельный вызов отклонить. */
static void *begin(void *state, DVM_Provider *target, DVM_Provider *tree)
{
    StyxCallerServer *server = state;
    (void)target;
    pthread_mutex_lock(&server->lock);
    int accepted = !server->stopped && !server->active && server->generation < (~(Path)0 - 1) / CALLER_NODES;
    if (accepted)
    {
        server->active = tree;
        server->generation++;
    }
    pthread_mutex_unlock(&server->lock);
    return accepted ? tree : NULL;
}


/* EN: Remove the publication as soon as the exchange retires.
 * RU: Снять публикацию сразу после завершения обмена. */
static void end(void *state, void *lease)
{
    StyxCallerServer *server = state;
    pthread_mutex_lock(&server->lock);
    if (server->active == lease)
    {
        server->active = NULL;
    }
    pthread_mutex_unlock(&server->lock);
}


/* EN: Drop unopened Qid lookups. Open fids independently own their handles.
 * RU: Освободить узлы поиска. Открытые fid удерживают собственные файлы. */
static void clear(StyxCallerServer *server)
{
    for (size_t i = 0; i < server->count; i++)
    {
        styxbackingfree(server->nodes[i]);
    }
    server->count = 0;
}


/* EN: Borrow a visited node for this dispatcher generation. The root is dynamic.
 * RU: Заимствовать посещённый узел поколения. Корень динамический. */
static StyxBacking *find(Path path)
{
    pthread_mutex_lock(&owner->lock);
    Path generation = owner->generation;
    DVM_Provider *tree = owner->active;
    if (!dvm_provider_retain(tree))
    {
        tree = NULL;
    }
    pthread_mutex_unlock(&owner->lock);
    if (!tree || generation != owner->observed)
    {
        clear(owner);
        owner->observed = generation;
        owner->base = generation * CALLER_NODES;
    }
    if (tree && !owner->count)
    {
        StyxBacking *root = styxtree(tree);
        if (root)
        {
            owner->nodes[owner->count++] = root;
        }
    }
    dvm_provider_release(tree);
    if (path == Qroot)
    {
        return owner->count ? owner->nodes[0] : NULL;
    }
    return path >= owner->base && path - owner->base < owner->count ?
        owner->nodes[path - owner->base] : NULL;
}


/* EN: Resolve one component in the active capability and issue a bounded Qid.
 * RU: Найти компонент в активном дереве и выдать Qid в пределах лимита. */
static char *walk(Qid *qid, char *name)
{
    StyxBacking *parent = find(qid->path);
    if (!parent || owner->count == CALLER_NODES)
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
    *qid = styxqid(owner->base + owner->count, meta.directory);
    owner->nodes[owner->count++] = node;
    return nil;
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
    StyxBacking *node = find(qid->path);
    if (!node)
    {
        return Enonexist;
    }
    CallerOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return Enomem;
    }
    opened->file = styxfileopen(node, mode);
    opened->generation = owner->observed;
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
    pthread_mutex_lock(&owner->lock);
    int valid = opened && owner->active && opened->generation == owner->generation;
    pthread_mutex_unlock(&owner->lock);
    return valid ? opened->file : NULL;
}


/* EN: Copy display metadata into a stat record. Name storage remains borrowed.
 * RU: Скопировать метаданные в stat. Хранилище имени заимствуется. */
static void attributes(const StyxMeta *meta, Qid qid, Dir *dir)
{
    memset(dir, 0, sizeof(*dir));
    dir->qid = qid;
    dir->mode = meta->mode | (meta->directory ? DMDIR : 0);
    dir->length = meta->length;
    dir->name = (char *)meta->name;
    dir->uid = dir->gid = dir->muid = "caller";
}


/* EN: Stat the opened resource or current visited node.
 * RU: Получить stat открытого файла или текущего узла поиска. */
static char *stat_node(Qid qid, Dir *dir)
{
    StyxOpen *opened = active_open();
    if (!(styxgetaux(&owner->server) ? opened && styxfileinfo(opened, &owner->stat) :
            styxbackinginfo(find(qid.path), &owner->stat)))
    {
        return Enonexist;
    }
    attributes(&owner->stat, qid, dir);
    return nil;
}


/* EN: Encode complete directory entries, advancing only after copying each entry.
 * RU: Кодировать полные записи каталога, продвигая курсор после копирования записи. */
static char *list(StyxOpen *opened, char *bytes, ulong *count, vlong offset)
{
    if (offset < 0 || !styxdirbegin(opened, offset))
    {
        return Eperm;
    }
    ulong used = 0;
    while (used < *count)
    {
        StyxMeta meta;
        int next = styxdirpeek(opened, &meta);
        if (!next)
        {
            break;
        }
        if (next < 0)
        {
            if (!used)
            {
                return Eperm;
            }
            break;
        }
        Dir dir;
        attributes(&meta, styxqid(meta.identity, meta.directory), &dir);
        uint size = sizeD2M(&dir);
        if (size > *count - used)
        {
            if (!used)
            {
                return Ebadarg;
            }
            break;
        }
        if (convD2M(&dir, (uchar *)bytes + used, *count - used) != size || !styxdiradvance(opened, size))
        {
            return Ebadarg;
        }
        used += size;
    }
    *count = used;
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
        return list(opened, bytes, count, offset);
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
    }
    styxend(&owner->server);
    clear(owner);
    pthread_mutex_lock(&owner->lock);
    owner->stopped = 1;
    pthread_mutex_unlock(&owner->lock);
    owner = NULL;
    return NULL;
}


/* EN/RU: See caller_styx.h. The binding is installed only on a fresh mount.
 * См. caller_styx.h. Связь устанавливается только на свежий маунт. */
StyxCallerServer *styxcallerserver(RemotePhy *reverse)
{
    StyxCallerServer *server = calloc(1, sizeof(*server));
    if (!server || pthread_mutex_init(&server->lock, NULL))
    {
        free(server);
        phyfree(reverse);
        return NULL;
    }
    server->references = 1;
    if (styxphyinit(&server->server, &operations, reverse, 0555, 0, 8))
    {
        pthread_mutex_destroy(&server->lock);
        free(server);
        return NULL;
    }
    if (pthread_create(&server->worker, NULL, serve, server))
    {
        styxend(&server->server);
        pthread_mutex_destroy(&server->lock);
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
    pthread_mutex_lock(&server->lock);
    int last = --server->references == 0;
    pthread_mutex_unlock(&server->lock);
    if (last)
    {
        pthread_mutex_destroy(&server->lock);
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
    pthread_mutex_lock(&server->lock);
    int retained = !server->stopped && server->references < (size_t)-1;
    if (retained)
    {
        server->references++;
    }
    pthread_mutex_unlock(&server->lock);
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
