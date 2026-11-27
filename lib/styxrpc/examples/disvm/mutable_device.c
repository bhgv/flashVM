/* Copyright (c) 2026 bhgv
 * EN: In-memory C provider for ordinary 9P creation and exact-fid removal.
 * RU: Провайдер C в памяти для создания через 9P и удаления по точному fid.
 */
#include "styxrpc/serve.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NODE_LIMIT 16U /* Retired IDs stay reserved until exit. */

typedef struct Node
{
    StyxRpcEntry entry; /* Stable template copied by stat. */
    uint64_t parent;    /* Directory holding this link. */
    char name[32];      /* Copied component, retained even after unlink. */
    char bytes[32];     /* Bytes held by open cookies independently of the link. */
    size_t length;      /* Extent of bytes. */
    unsigned revision;  /* Directory membership generation. */
    unsigned opens;     /* Exact live fid owners. */
    int linked;        /* Name and ID are visible via walk/stat. */
} Node;

typedef struct Device
{
    Node nodes[NODE_LIMIT]; /* Stable storage for live and removed nodes. */
    unsigned used;          /* Number of issued IDs, including the root. */
} Device;


/* EN: Add a live child or root in owner-only storage; return its stable node.
 * RU: Добавить потомка или корень в память владельца и вернуть устойчивый узел. */
static Node *insert(Device *device, uint64_t parent, const char *name, int directory, unsigned mode)
{
    if (device->used == NODE_LIMIT || strlen(name) >= sizeof(device->nodes[0].name))
    {
        return NULL;
    }
    unsigned id = device->used++;
    Node *node = &device->nodes[id];
    strcpy(node->name, name);
    node->entry = (StyxRpcEntry){id, node->name, mode, directory};
    node->parent = parent;
    node->linked = 1;
    if (id)
    {
        ++device->nodes[parent].revision;
    }
    return node;
}


/* EN: Return a borrowed stat snapshot for a live ID; zero for removed IDs.
 * RU: Вернуть снимок статуса живого ID. Для удалённых ID вернуть ноль. */
static int stat_node(void *context, uint64_t id, StyxRpcNode *out)
{
    Device *device = context;
    if (id >= device->used || !device->nodes[id].linked)
    {
        return 0;
    }
    const Node *node = &device->nodes[id];
    *out = (StyxRpcNode){.entry = node->entry, .parent = node->parent,
        .length = node->length, .revision = node->revision};
    return 1;
}


/* EN: Return a borrowed child by slot in stable ascending ID order, or NULL.
 * RU: Вернуть потомка по позиции в порядке возрастания ID либо NULL. */
static const StyxRpcEntry *enumerate(void *context, uint64_t parent, size_t index)
{
    Device *device = context;
    for (unsigned id = 1; id < device->used; ++id)
    {
        Node *node = &device->nodes[id];
        if (node->linked && node->parent == parent && index-- == 0)
        {
            return &node->entry;
        }
    }
    return NULL;
}


/* EN: Pin an ordinary file as a retained cookie for the requested access.
 * The dispatcher checks access; this provider stores one kind of native cookie.
 * RU: Удержать обычный файл как состояние fid. Доступ проверяет диспетчер;
 * провайдер хранит состояние одного типа для всех режимов. */
static int open_node(void *context, uint64_t id, unsigned access, void **cookie)
{
    Device *device = context;
    if (!access || id >= device->used || !device->nodes[id].linked || device->nodes[id].entry.directory)
    {
        return 0;
    }
    Node *node = &device->nodes[id];
    ++node->opens;
    *cookie = node;
    return 1;
}


/* EN: Copy up to capacity bytes at offset from a retained cookie, even if unlinked.
 * RU: Скопировать до capacity байт со смещения offset даже после удаления имени. */
static int read_node(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const Node *node = cookie;
    *size = offset < node->length ? node->length - (size_t)offset : 0;
    if (*size > capacity)
    {
        *size = capacity;
    }
    if (*size)
    {
        memcpy(bytes, node->bytes + (size_t)offset, *size);
    }
    return 1;
}


/* EN: Store count bytes at offset in the fixed file budget or reject the write.
 * RU: Записать count байт со смещения offset в пределах файла либо отказать. */
static int write_node(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Node *node = cookie;
    if (offset > sizeof(node->bytes) || count > sizeof(node->bytes) - offset)
    {
        return 0;
    }
    memcpy(node->bytes + (size_t)offset, bytes, count);
    if (offset + count > node->length)
    {
        node->length = (size_t)offset + count;
    }
    *size = count;
    return 1;
}


/* EN: Consume one cookie and keep the ID slot reserved.
 * RU: Закрыть одно состояние fid и сохранить занятым номер ID. */
static void close_node(void *cookie)
{
    Node *node = cookie;
    --node->opens;
}


/* EN: Publish a new child and return its ID, revision and optional writer cookie.
 * RU: Опубликовать потомка и вернуть ID, ревизию и состояние записи, если нужно. */
static int create_node(void *context, uint64_t parent, const char *name, int directory,
    uint64_t *id, uint32_t *revision, void **cookie)
{
    const StyxRpcEntry *existing;
    for (size_t index = 0; (existing = enumerate(context, parent, index)); ++index)
    {
        if (!strcmp(existing->name, name))
        {
            return 0;
        }
    }
    Node *node = insert(context, parent, name, directory, directory ? 0777U : 0666U);
    if (!node)
    {
        return 0;
    }
    *id = node->entry.id;
    *revision = node->revision;
    if (cookie)
    {
        ++node->opens;
        *cookie = node;
    }
    return 1;
}


/* EN: Unlink the selected empty node, preserving outstanding cookies.
 * RU: Удалить выбранный пустой узел, сохранив действующие состояния fid. */
static int remove_node(void *context, uint64_t id)
{
    Device *device = context;
    if (enumerate(context, id, 0))
    {
        return 0;
    }
    device->nodes[id].linked = 0;
    ++device->nodes[device->nodes[id].parent].revision;
    return 1;
}


/* EN: Serve one inherited socket on this thread and check all fid owners retire.
 * RU: Обслужить унаследованный сокет в этом потоке и проверить закрытие всех fid. */
int main(int argc, char **argv)
{
    static const StyxRpcTree tree = {.files = {enumerate, open_node, read_node, write_node, close_node},
        .stat = stat_node}; /* Fixed ordinary file dispatch policy. */
    static const StyxRpcMutations mutations = {create_node, remove_node}; /* Exact-ID mutation policy. */
    if (argc != 2)
    {
        return 1;
    }
    Device device = {0};
    insert(&device, 0, ".", 1, 0777);
    insert(&device, 0, "readonly", 1, 0555);
    Node *service = insert(&device, 0, ".prog", 1, 0777);
    insert(&device, service->entry.id, "sub", 1, 0777);
    StyxRpcServer *server = styxrpc_server_create(phyposixfd(atoi(argv[1]), PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
        &tree, &device);
    if (!styxrpc_server_enable_mutations(server, &mutations) ||
        styxrpc_server_enable_mutations(server, &mutations))
    {
        styxrpc_server_destroy(server);
        return 1;
    }
    while (styxrpc_server_step(server))
    {
    }
    int late_enable = styxrpc_server_enable_mutations(server, &mutations);
    styxrpc_server_destroy(server);
    for (unsigned id = 0; id < device.used; ++id)
    {
        if (device.nodes[id].opens)
        {
            fprintf(stderr, "fid leaked for node %u\n", id);
            return 1;
        }
    }
    return late_enable;
}
