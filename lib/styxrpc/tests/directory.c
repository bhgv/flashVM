/* Copyright (c) 2026 bhgv
 * EN: Exercise lookup and directory revisions on live callback metadata over Styx.
 * RU: Проверка поиска и ревизий живых каталогов через Styx.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/serve.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


enum
{
    Root,    /* Attach root. */
    Control, /* Write to replace the other name. */
    Before,  /* Original file, retained by open fids through deletion. */
    After    /* Newly enumerated file with a distinct ID. */
};

typedef struct Directory
{
    StyxRpcServer *server; /* Owned by dispatcher thread after startup. */
    uint32_t revision;   /* Starts at zero; write changes membership to one. */
} Directory;

static const StyxRpcEntry nodes[] = { /* All possible names at root. */
    {Root, "/", 0555, 1}, {Control, "change", 0222, 0},
    {Before, "before", 0444, 0}, {After, "after", 0444, 0}
};
static char before = 'B'; /* Stub content with lifetime independent of publication. */
static char after = 'A';  /* Stub content of new file. */


/* EN: Return the control file and the current readable sibling, one at a time.
 * RU: Вернуть управляющий файл и текущего читаемого соседа по одному. */
static const StyxRpcEntry *child(void *context, uint64_t id, size_t index)
{
    Directory *tree = context;
    if (id != Root || index > 1)
    {
        return NULL;
    }
    return &nodes[index ? (tree->revision ? After : Before) : Control];
}


/* EN: Return metadata only for current nodes and signal root membership changes.
 * RU: Вернуть метаданные текущих узлов и указать изменение состава корня. */
static int lookup(void *context, uint64_t id, StyxRpcNode *node)
{
    Directory *tree = context;
    if (id > After || (id == Before && tree->revision) || (id == After && !tree->revision))
    {
        return 0;
    }
    *node = (StyxRpcNode){nodes[id], Root, id >= Before ? 1 : 0,
        id == Root ? tree->revision : 0};
    return 1;
}


/* EN: Pin the old or new byte, or identify the control cookie.
 * RU: Закрепить старый либо новый байт или состояние управляющего файла. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    (void)access;
    *cookie = id == Before ? &before : id == After ? &after : context;
    return 1;
}


/* EN: Read the previously opened byte at its only offset.
 * RU: Прочитать ранее открытый байт по единственному смещению. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    *size = capacity && !offset;
    if (*size)
    {
        memcpy(bytes, cookie, 1);
    }
    return 1;
}


/* EN: Replace a published name only on an exact control write.
 * RU: Заменить опубликованное имя при точной записи в управляющий файл. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t size, size_t *written)
{
    if (offset || size != 1 || *(const char *)bytes != '1')
    {
        return 0;
    }
    Directory *tree = cookie;
    StyxRpcServerState state;
    if (!styxrpc_server_state(tree->server, &state) || state.clients != 1)
    {
        return 0;
    }
    tree->revision = 1;
    *written = 1;
    return 1;
}

static const StyxRpcTree callbacks = {{child, open_file, read_file, write_file, NULL}, lookup};


/* EN: Move the server to one dispatcher thread until the client ends the link.
 * RU: Передать сервер потоку диспетчера до закрытия связи клиентом. */
static void *dispatch(void *context)
{
    Directory *tree = context;
    while (styxrpc_server_step(tree->server))
    {
    }
    styxrpc_server_destroy(tree->server);
    return NULL;
}


/* EN: Inspect one session before and after an exact mutation.
 * RU: Проверить сессию до и после одного изменения дерева. */
static int inspect(StyxClient *client)
{
    StyxFid *dir = styxclientwalk(client, ".");
    StyxFid *old = styxclientwalk(client, "before");
    StyxFid *control = styxclientwalk(client, "change");
    StyxTransfer sent, initial, stale, fresh, retained, latest;
    unsigned char listing[64];
    char old_byte = 0, new_byte = 0;
    int good = dir && old && control && styxclientopen(dir, 0) &&
        styxclientopen(old, 0) && styxclientopen(control, 1);
    if (good)
    {
        styxclientread(dir, 0, listing, sizeof(listing), &initial);
        styxclientwrite(control, 0, "1", 1, &sent);
        styxclientread(dir, initial.count, listing, sizeof(listing), &stale);
        styxclientread(old, 0, &old_byte, 1, &retained);
        StyxFid *absent = styxclientwalk(client, "before");
        StyxFid *new_dir = styxclientwalk(client, ".");
        StyxFid *new_file = styxclientwalk(client, "after");
        good = initial.success && initial.count && sent.success && sent.count == 1 &&
            !stale.success && retained.success && retained.count == 1 && old_byte == 'B' &&
            !absent && new_dir && new_file && styxclientopen(new_dir, 0) && styxclientopen(new_file, 0);
        if (good)
        {
            styxclientread(new_dir, 0, listing, sizeof(listing), &fresh);
            styxclientread(new_file, 0, &new_byte, 1, &latest);
            good = fresh.success && fresh.count && latest.success && latest.count == 1 && new_byte == 'A';
        }
        styxclientclunk(new_file);
        styxclientclunk(new_dir);
        styxclientclunk(absent);
    }
    styxclientclunk(control);
    styxclientclunk(old);
    styxclientclunk(dir);
    return good;
}


/* EN: Create the socket pair, start the callback owner, and verify directory semantics.
 * RU: Создать пару сокетов, запустить владельца колбэков и проверить каталог. */
int main(void)
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 1;
    }
    Directory tree = {.server = styxrpc_server_create(
        phyposixfd(sockets[1], PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), &callbacks, &tree)};
    pthread_t owner;
    if (!tree.server || pthread_create(&owner, NULL, dispatch, &tree))
    {
        styxrpc_server_destroy(tree.server);
        close(sockets[0]);
        return 1;
    }
    StyxClient *client = styxclientmountfd(sockets[0], 4096, "test", "", 1000, 8);
    int good = client && inspect(client);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(owner, NULL);
    if (!good)
    {
        fputs("live Styx directory or opened node mismatch\n", stderr);
    }
    return good ? 0 : 1;
}
