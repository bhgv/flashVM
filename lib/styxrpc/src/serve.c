/* Copyright (c) 2026 bhgv
 * EN: Dynamic name resolution and directory/file dispatch over original Styx.
 * RU: Динамический поиск имён и обслуживание каталогов и файлов через Styx.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "styxrpc/program.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>


struct StyxRpcServer
{
    Styxserver wire;                /* Native single-owner transport dispatcher. */
    const StyxRpcTree *tree;        /* Borrowed callback table. */
    void *context;                  /* Borrowed application state. */
    char name[STYXRPC_TREE_NAME + 1]; /* Own stat name through its response encoding. */
    StyxRpcPrograms *programs;       /* Exclusive borrowed registry for a program server. */
    pthread_mutex_t program_lock;   /* Serializes requests and changes to that registry. */
};

typedef struct TreeFid
{
    StyxRpcFile file; /* Owned state of an opened byte file, empty on directories. */
    uint64_t offset;  /* Expected next directory byte position. */
    size_t index;     /* Next directory entry index. */
    uint32_t revision; /* Revision captured when at offset zero. */
} TreeFid;

static __thread StyxRpcServer *active; /* Current host dispatcher on this native thread. */


/* EN: Obtain current metadata for an exact ID, within the exported contract.
 * RU: Получить текущие метаданные точного ID в рамках контракта экспорта. */
static int lookup(StyxRpcServer *server, uint64_t id, StyxRpcNode *node)
{
    return server->tree->stat(server->context, id, node) == 1 &&
        node->entry.id == id && node->entry.name && strlen(node->entry.name) <= STYXRPC_TREE_NAME &&
        !(node->entry.mode & ~0777U) &&
        (id ? *node->entry.name && !strchr(node->entry.name, '/') &&
            strcmp(node->entry.name, ".") && strcmp(node->entry.name, "..") && node->parent != id :
            node->entry.directory && !node->parent);
}


/* EN: Convert a stable node to a wire identity and copy the declared access mode.
 * RU: Преобразовать устойчивый узел в сетевой идентификатор с его режимом доступа. */
static Qid identity(const StyxRpcNode *node)
{
    return (Qid){.path = node->entry.id, .vers = node->revision,
        .type = node->entry.directory ? QTDIR : 0};
}


/* EN: Populate Dir now with an owned name, valid until the next encoding.
 * RU: Заполнить Dir своим именем, действительным до следующего кодирования. */
static void describe(StyxRpcServer *server, const StyxRpcNode *node, Dir *out)
{
    strcpy(server->name, node->entry.name);
    *out = (Dir){.qid = identity(node), .name = server->name,
        .uid = "rpc", .gid = "rpc", .muid = "rpc",
        .mode = node->entry.mode | (node->entry.directory ? DMDIR : 0),
        .length = node->entry.directory ? 0 : node->length};
}


/* EN: Resolve an immediate component of one traversable directory.
 * RU: Разрешить один компонент имени в каталоге с правом прохода. */
static char *walk_node(Qid *qid, char *name)
{
    StyxRpcNode parent, child;
    if (!lookup(active, qid->path, &parent) || !parent.entry.directory)
    {
        return Enonexist;
    }
    if (!(parent.entry.mode & 1))
    {
        return Eperm;
    }
    if (!strcmp(name, ".."))
    {
        if (!lookup(active, parent.parent, &child) || !child.entry.directory)
        {
            return Enonexist;
        }
        *qid = identity(&child);
        return nil;
    }
    for (size_t index = 0; index < STYXRPC_TREE_CHILDREN; ++index)
    {
        StyxRpcEntry entry;
        StyxRpcStatus status = styxrpc_dir_entry(&active->tree->files, active->context, qid->path, index, &entry);
        if (status != STYXRPC_OK)
        {
            return status == STYXRPC_MORE ? Enonexist : Ebadarg;
        }
        if (!strcmp(name, entry.name))
        {
            if (!entry.id || !lookup(active, entry.id, &child) ||
                child.parent != qid->path || strcmp(child.entry.name, name))
            {
                return Enonexist;
            }
            *qid = identity(&child);
            return nil;
        }
    }
    return Enonexist;
}


/* EN: Release only the cookie owned by this exact fid.
 * RU: Освободить состояние, принадлежащее только этому fid. */
static void release_fid(void *state)
{
    TreeFid *fid = state;
    styxrpc_file_close(&fid->file);
    free(fid);
}


/* EN: Enforce peer mode and attach a fresh file cookie or directory cursor.
 * RU: Проверить права клиента и закрепить новое состояние файла или каталог. */
static char *open_node(Qid *qid, int mode)
{
    StyxRpcNode node;
    if (!lookup(active, qid->path, &node))
    {
        return Enonexist;
    }
    unsigned access = mode == OREAD ? STYXRPC_READ : mode == OWRITE ? STYXRPC_WRITE :
        mode == ORDWR ? STYXRPC_READ | STYXRPC_WRITE : 0;
    unsigned required = ((access & STYXRPC_READ) ? 4U : 0U) | ((access & STYXRPC_WRITE) ? 2U : 0U);
    if (!access || (node.entry.mode & required) != required ||
        (node.entry.directory && access != STYXRPC_READ))
    {
        return Eperm;
    }
    TreeFid *fid = calloc(1, sizeof(*fid));
    if (!fid)
    {
        return Enomem;
    }
    if ((!node.entry.directory && styxrpc_file_open(&fid->file, &active->tree->files, active->context,
        qid->path, access) != STYXRPC_OK) || styxsetauxowned(&active->wire, fid, release_fid) < 0)
    {
        release_fid(fid);
        return Eperm;
    }
    *qid = identity(&node);
    return nil;
}


/* EN: Generate complete directory records from the opened fid's byte cursor.
 * RU: Сформировать целые записи каталога с байтового курсора открытого fid. */
static char *read_directory(Qid qid, TreeFid *fid, char *bytes, ulong *count, uint64_t offset)
{
    StyxRpcNode directory;
    if (!lookup(active, qid.path, &directory) || !directory.entry.directory)
    {
        return Enonexist;
    }
    if (!offset)
    {
        fid->offset = 0;
        fid->index = 0;
        fid->revision = directory.revision;
    }
    if (offset != fid->offset || fid->revision != directory.revision)
    {
        return Ebadarg;
    }
    size_t written = 0, index = fid->index;
    while (*count > written && index < STYXRPC_TREE_CHILDREN)
    {
        StyxRpcEntry entry;
        StyxRpcStatus status = styxrpc_dir_entry(&active->tree->files, active->context, qid.path, index, &entry);
        if (status == STYXRPC_MORE)
        {
            break;
        }
        StyxRpcNode child;
        if (status != STYXRPC_OK || !entry.id || !lookup(active, entry.id, &child) || child.parent != qid.path)
        {
            return Ebadarg;
        }
        Dir info;
        describe(active, &child, &info);
        uint extent = sizeD2M(&info);
        if (extent > *count - written)
        {
            if (!written)
            {
                return "directory entry exceeds read capacity";
            }
            break;
        }
        if (convD2M(&info, (uchar *)bytes + written, extent) != extent)
        {
            return Ebadarg;
        }
        written += extent;
        ++index;
    }
    fid->index = index;
    fid->offset += written;
    *count = written;
    return nil;
}


/* EN: Dispatch a positional read on this exact opened fid or its directory cursor.
 * RU: Передать позиционное чтение открытого fid или его курсора каталога. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    TreeFid *fid = styxgetaux(&active->wire);
    if (!fid || offset < 0)
    {
        return Ebadarg;
    }
    if (qid.type & QTDIR)
    {
        return read_directory(qid, fid, bytes, count, (uint64_t)offset);
    }
    size_t size;
    if (styxrpc_file_read(&fid->file, (uint64_t)offset, bytes, *count, &size) != STYXRPC_OK)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN: Forward one positional write through the exact opened fid.
 * RU: Передать одну позиционную запись через точный открытый fid. */
static char *write_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    TreeFid *fid = styxgetaux(&active->wire);
    size_t size;
    if (!fid || offset < 0 || (qid.type & QTDIR) ||
        styxrpc_file_write(&fid->file, (uint64_t)offset, bytes, *count, &size) != STYXRPC_OK)
    {
        return Ebadarg;
    }
    *count = size;
    return nil;
}


/* EN: Resolve stat by ID, independent of the lookup path.
 * RU: Выдать stat по ID независимо от пути обхода. */
static char *stat_node(Qid qid, Dir *dir)
{
    StyxRpcNode node;
    if (!lookup(active, qid.path, &node))
    {
        return Enonexist;
    }
    describe(active, &node, dir);
    return nil;
}


/* EN/RU: See serve.h. / См. serve.h. */
StyxRpcServer *styxrpc_server_create(RemotePhy *phy, const StyxRpcTree *tree, void *context)
{
    static Styxops ops = {.walk = walk_node, .open = open_node, .read = read_node,
        .write = write_node, .stat = stat_node};
    if (!phy || !tree || !tree->stat || !tree->files.enumerate)
    {
        phyfree(phy);
        return NULL;
    }
    StyxRpcServer *server = calloc(1, sizeof(*server));
    if (!server)
    {
        phyfree(phy);
        return NULL;
    }
    server->tree = tree;
    server->context = context;
    StyxRpcNode root;
    if (!lookup(server, 0, &root))
    {
        phyfree(phy);
        free(server);
        return NULL;
    }
    if (styxphyinit(&server->wire, &ops, phy, root.entry.mode, 0, 8))
    {
        free(server);
        return NULL;
    }
    return server;
}


/* EN/RU: See program.h. / См. program.h. */
StyxRpcServer *styxrpc_program_server_create(RemotePhy *phy, StyxRpcPrograms *programs)
{
    if (!programs || !programs->nodes || programs->used < 2)
    {
        phyfree(phy);
        return NULL;
    }
    StyxRpcServer *server = styxrpc_server_create(phy, &styxrpc_program_tree, programs);
    if (server && pthread_mutex_init(&server->program_lock, NULL))
    {
        styxrpc_server_destroy(server);
        return NULL;
    }
    if (server)
    {
        server->programs = programs;
    }
    return server;
}


/* EN/RU: See program.h. / См. program.h. */
uint32_t styxrpc_program_server_grant(StyxRpcServer *server, const StyxRpcResource *resource)
{
    if (!server || !server->programs)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    uint32_t pid = styxrpc_program_unibus_grant(server->programs, resource);
    pthread_mutex_unlock(&server->program_lock);
    return pid;
}


/* EN/RU: See program.h. / См. program.h. */
int styxrpc_program_server_revoke(StyxRpcServer *server, uint32_t pid)
{
    if (!server || !server->programs)
    {
        return 0;
    }
    pthread_mutex_lock(&server->program_lock);
    int revoked = styxrpc_program_unibus_revoke(server->programs, pid);
    pthread_mutex_unlock(&server->program_lock);
    return revoked;
}


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_step(StyxRpcServer *server)
{
    if (!server)
    {
        return 0;
    }
    StyxRpcServer *previous = active;
    active = server;
    int ready = !styxwait(&server->wire);
    if (ready)
    {
        if (server->programs)
        {
            pthread_mutex_lock(&server->program_lock);
        }
        ready = !styxprocess(&server->wire);
        if (server->programs)
        {
            pthread_mutex_unlock(&server->program_lock);
        }
    }
    active = previous;
    return ready;
}


/* EN/RU: See serve.h. / См. serve.h. */
void styxrpc_server_destroy(StyxRpcServer *server)
{
    if (server)
    {
        StyxRpcServer *previous = active;
        active = server;
        styxend(&server->wire);
        active = previous;
        if (server->programs)
        {
            pthread_mutex_destroy(&server->program_lock);
        }
        free(server);
    }
}


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_state(StyxRpcServer *server, StyxRpcServerState *state)
{
    if (!server || !state)
    {
        return 0;
    }
    size_t count = 0;
    for (const Client *client = server->wire.clients; client; client = client->next)
    {
        ++count;
    }
    *state = (StyxRpcServerState){.clients = count, .reply_bytes = styxphypending(&server->wire),
        .free_slots = styxphyspace(&server->wire)};
    return 1;
}
