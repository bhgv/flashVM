/* Copyright (c) 2026 bhgv
 * EN: Dynamic name resolution and directory/file dispatch over original Styx.
 * RU: Динамический поиск имён и обслуживание каталогов и файлов через Styx.
 */
#include <lib9.h>
#include <styx.h>
#include <styxserver.h>
#include "server.h"
#include "control_internal.h"
#include "message_internal.h"
#include "module_slot_internal.h"
#include "nodes_internal.h"
#include "variable_ref_internal.h"
#include "styxrpc/exchange.h"
#include "styxrpc/program.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

typedef struct MessageRoot
{
    MessageService *service;                /* Owned registration while published. */
    char name[STYXRPC_TREE_NAME + 1];       /* Exact name under the connection root. */
    uint64_t claim;                        /* Optional reservation in an active module slot. */
    uint32_t generation;                   /* Increased before each reuse of the wire Qid. */
} MessageRoot;


struct StyxRpcServer
{
    Styxserver wire;                /* Native single-owner transport dispatcher. */
    const StyxRpcTree *tree;        /* Borrowed callback table. */
    void *context;                  /* Borrowed application state. */
    StyxRpcAttachNotice attach;      /* Optional owner callback for attachment metadata. */
    void *attach_state;             /* Borrowed by attach until cleared. */
    char name[STYXRPC_TREE_NAME + 1]; /* Own stat name through its response encoding. */
    MessageRoot messages[STYXRPC_MESSAGE_ROOTS_MAX]; /* Live exchange roots and retired generations. */
    int started;                    /* Legacy message enable is only allowed before dispatch. */
    int program_only;                /* Serve only the native reverse program tree during a synchronous call. */
    struct TreeFid *deferred;        /* Unrelated clunks whose callbacks must wait for ordinary dispatch. */
    int control_only;               /* Serve .state while a host activation is stopped. */
    StyxRpcPrograms *programs;       /* Exclusive borrowed registry for a program server. */
    pthread_mutex_t program_lock;   /* Serializes requests and changes to that registry. */
};

typedef struct TreeFid
{
    struct TreeFid *next; /* Deferred clunk linkage, used at most once. */
    int program;      /* Open belongs to the native reverse program tree. */
    StyxRpcFile file; /* Owned state of an opened byte file, empty on directories. */
    uint64_t offset;  /* Expected next directory byte position. */
    size_t index;     /* Next directory entry index. */
    uint32_t revision; /* Revision captured when at offset zero. */
} TreeFid;

static __thread StyxRpcServer *active; /* Current host dispatcher on this native thread. */


#include "serve_program_wait.inc"


/* EN: Map id to an exchange slot, or return an out-of-range index.
 * RU: Найти индекс корня обмена по id либо вернуть индекс вне диапазона. */
static uint64_t message_slot(uint64_t id)
{
    return STYXRPC_MESSAGE_NODE - id;
}


/* EN: Obtain current metadata for an exact ID, within the exported contract.
 * RU: Получить текущие метаданные точного ID в рамках контракта экспорта. */
static int lookup(StyxRpcServer *server, uint64_t id, StyxRpcNode *node)
{
    uint64_t slot = message_slot(id);
    if (slot < STYXRPC_MESSAGE_ROOTS_MAX)
    {
        MessageRoot *root = &server->messages[slot];
        if (!root->service)
        {
            return 0;
        }
        *node = (StyxRpcNode){.entry = {id, root->name, 0666, 0}, .revision = root->generation};
        return 1;
    }
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
    if (active->program_only &&
        (qid->path ? !native_program(qid->path) : strcmp(name, ".prog")))
    {
        return Eperm;
    }
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
    if (!qid->path)
    {
        uint64_t service = !strcmp(name, ".prog") ? STYXRPC_SLOT_PROGRAM_ROOT :
            !strcmp(name, STYXRPC_NODE_EXEC_SERVICE) ? STYXRPC_NODE_EXEC_QID : 0;
        if (active->tree == &styxrpc_module_slot_tree && service && lookup(active, service, &child))
        {
            *qid = identity(&child);
            return nil;
        }
        for (size_t slot = 0; slot < STYXRPC_MESSAGE_ROOTS_MAX; ++slot)
        {
            const MessageRoot *root = &active->messages[slot];
            if (root->service && !strcmp(name, root->name))
            {
                *qid = (Qid){.path = STYXRPC_MESSAGE_NODE - slot, .vers = root->generation};
                return nil;
            }
        }
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
    if (active->program_only && !fid->program)
    {
        fid->next = active->deferred;
        active->deferred = fid;
    }
    else
    {
        close_tree_fid(fid);
    }
}


/* EN: Enforce peer mode and attach a fresh file cookie or directory cursor.
 * RU: Проверить права клиента и закрепить новое состояние файла или каталог. */
static char *open_node(Qid *qid, int mode)
{
    StyxRpcNode node;
    if ((active->control_only && qid->path != CONTROL_STATE) ||
        (active->program_only && !native_program(qid->path)))
    {
        return Eperm;
    }
    uint64_t slot = message_slot(qid->path);
    int messages = slot < STYXRPC_MESSAGE_ROOTS_MAX;
    if (!lookup(active, qid->path, &node) || (messages && qid->vers != node.revision))
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
    fid->program = native_program(qid->path);
    const StyxRpcVfs *files = messages ? &styxrpc_message_files : &active->tree->files;
    void *context = messages ? active->messages[slot].service : active->context;
    if ((!node.entry.directory && styxrpc_file_open(&fid->file, files, context,
        qid->path, access) != STYXRPC_OK) || styxsetauxowned(&active->wire, fid, release_fid) < 0)
    {
        release_fid(fid);
        return Eperm;
    }
    *qid = identity(&node);
    return nil;
}


#include "serve_listing.inc"


/* EN: Dispatch a positional read on this exact opened fid or its directory cursor.
 * RU: Передать позиционное чтение открытого fid или его курсора каталога. */
static char *read_node(Qid qid, char *bytes, ulong *count, vlong offset)
{
    TreeFid *fid = styxgetaux(&active->wire);
    if (!fid || offset < 0 || (active->control_only && qid.path != CONTROL_STATE) ||
        (active->program_only && !fid->program))
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
        (active->control_only && qid.path != CONTROL_STATE) ||
        (active->program_only && !fid->program) ||
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
    if (active->program_only && qid.path && !native_program(qid.path))
    {
        return Eperm;
    }
    if (!lookup(active, qid.path, &node) || (STYXRPC_EXCHANGE_QID(qid.path) && qid.vers != node.revision))
    {
        return Enonexist;
    }
    describe(active, &node, dir);
    return nil;
}


#include "serve_attach.inc"


/* EN/RU: See serve.h. / См. serve.h. */
StyxRpcServer *styxrpc_server_create(RemotePhy *phy, const StyxRpcTree *tree, void *context)
{
    static Styxops ops = {.walk = walk_node, .open = open_node, .read = read_node,
        .write = write_node, .stat = stat_node, .attach = attach_peer};
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


#include "serve_grants.inc"


#include "serve_messages.inc"


/* EN/RU: See serve.h. / См. serve.h. */
int styxrpc_server_step_for(StyxRpcServer *server, int timeout_ms)
{
    if (!server || timeout_ms < -1)
    {
        return 0;
    }
    server->started = 1;
    StyxRpcServer *previous = active;
    active = server;
    if (!server->program_only)
    {
        drain_clunks(server);
    }
    int ready = !styxphywait(&server->wire, timeout_ms);
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
int styxrpc_server_step(StyxRpcServer *server)
{
    return styxrpc_server_step_for(server, -1);
}

#include "serve_control.inc"

/* EN/RU: See serve.h. / См. serve.h. */
void styxrpc_server_destroy(StyxRpcServer *server)
{
    if (server)
    {
        StyxRpcServer *previous = active;
        active = server;
        styxend(&server->wire);
        drain_clunks(server);
        active = previous;
        if (server->programs)
        {
            pthread_mutex_destroy(&server->program_lock);
        }
        for (size_t i = 0; i < STYXRPC_MESSAGE_ROOTS_MAX; ++i)
        {
            retire_root(server, &server->messages[i]);
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
