/* Copyright (c) 2026 bhgv
 * EN: Exercise both named-service mounts and exact VFS name retirement.
 * RU: Проверка двух подключений службы имён и точного освобождения имён VFS.
 */
#include "styxrpc/module_slot.h"
#include "styxrpc/named_peer.h"
#include "styxrpc/nodes.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define TEST_MSIZE 4096U /* Requested wire message size in bytes. */
#define TEST_WAIT_MS 2000U /* Per-request timeout in milliseconds. */
#define TEST_TAGS 4U /* Outstanding requests on each test client. */
#define TEST_NODE_READ 4U /* Permission argument for a read-only callback. */
#define TEST_STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* Connected socket pair. */


/* EN: Refuse all movie uploads in this metadata-only slot.
 * RU: Отклонить загрузки фильмов в слоте проверки метаданных. */
static int upload(void *state, const void *bytes, size_t size)
{
    (void)state;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Refuse commands; this check only tests publication metadata.
 * RU: Отклонить команды, поскольку проверяется только публикация. */
static int command(void *state, StyxRpcControlCommand request)
{
    (void)state;
    (void)request;
    return 0;
}


/* EN: Return no status text for this metadata-only slot.
 * RU: Не выдавать текст состояния для слота проверки метаданных. */
static int status(void *state, char *bytes, size_t capacity, size_t *used)
{
    (void)state;
    (void)bytes;
    (void)capacity;
    *used = 0;
    return 1;
}


/* EN: Supply an empty read when an ordinary test member is opened.
 * RU: Вернуть EOF при чтении обычного проверочного узла. */
static int transfer(void *state, unsigned operation, void *bytes, size_t capacity, size_t *used)
{
    (void)state;
    (void)operation;
    (void)bytes;
    (void)capacity;
    *used = 0;
    return 1;
}


/* EN: Require that a path refuses channel enrollment and retire any unexpected fid.
 * RU: Проверить отказ в подключении по пути и закрыть неожиданный fid. */
static int refused(StyxClient *client, const char *path)
{
    StyxRpcVfsFile *file = styxrpc_named_open(client, path);
    if (file)
    {
        styxrpc_vfs_close(file, 0);
        fprintf(stderr, "reserved or occupied path accepted: %s\n", path);
        return 0;
    }
    return 1;
}


/* EN: A live channel reserves its exact name until both mounts close their fids.
 * RU: Живой канал резервирует точное имя до закрытия fid обоих подключений. */
static int lifetime(StyxRpcModuleSlot *slot, StyxClient *local, StyxClient *remote)
{
    const StyxRpcNodeCallbacks node = {.transfer = transfer};
    StyxRpcVfsFile *first = styxrpc_named_open(local, "/work");
    StyxRpcVfsFile *second = styxrpc_named_open(remote, "/work");
    int ok = first && second &&
        styxrpc_module_slot_new_node(slot, "/work", TEST_NODE_READ, &node) == STYXRPC_INVALID &&
        styxrpc_module_slot_new_node(slot, "/work/child", TEST_NODE_READ, &node) == STYXRPC_INVALID;
    styxrpc_vfs_close(first, 0);
    ok = ok && styxrpc_module_slot_new_node(slot, "/work", TEST_NODE_READ, &node) == STYXRPC_INVALID;
    styxrpc_vfs_close(second, 0);
    return ok && styxrpc_module_slot_new_node(slot, "/work", TEST_NODE_READ, &node) == STYXRPC_OK &&
        refused(remote, "/work") && refused(local, "/work") && refused(local, "/.channels");
}


/* EN: Confirm /scope enumerates only its ordinary callback while the channel is live.
 * RU: Проверить, что /scope показывает только обычный колбэк при живом канале. */
static int hidden_from_directory(StyxRpcModuleSlot *slot)
{
    const StyxRpcVfs *tree = &styxrpc_module_slot_tree.files;
    StyxRpcEntry directory, member;
    for (size_t index = 0; styxrpc_dir_entry(tree, slot, 0, index, &directory) == STYXRPC_OK; ++index)
    {
        if (!strcmp(directory.name, "scope"))
        {
            return directory.directory && styxrpc_dir_entry(tree, slot, directory.id, 0, &member) == STYXRPC_OK &&
                !strcmp(member.name, "member") && styxrpc_dir_entry(tree, slot, directory.id, 1, &member) == STYXRPC_MORE;
        }
    }
    return 0;
}


/* EN: Recreating a parent invalidates joins to the old reservation, preserving its fid.
 * RU: Замена каталога отклоняет новые подключения к старому резерву и сохраняет его fid. */
static int replaced_parent(StyxRpcModuleSlot *slot, StyxClient *local, StyxClient *remote)
{
    const StyxRpcNodeCallbacks node = {.transfer = transfer};
    if (!refused(local, "/absent/queue") ||
        styxrpc_module_slot_new_node(slot, "/scope/member", TEST_NODE_READ, &node) != STYXRPC_OK)
    {
        return 0;
    }
    StyxRpcVfsFile *old = styxrpc_named_open(remote, "/scope/queue");
    int ok = old && hidden_from_directory(slot) &&
        styxrpc_module_slot_remove_node(slot, "/scope/member") == STYXRPC_OK &&
        styxrpc_module_slot_new_node(slot, "/scope/member", TEST_NODE_READ, &node) == STYXRPC_OK &&
        refused(local, "/scope/queue");
    styxrpc_vfs_close(old, 0);
    StyxRpcVfsFile *fresh = styxrpc_named_open(local, "/scope/queue");
    ok = ok && fresh != NULL;
    styxrpc_vfs_close(fresh, 0);
    return ok;
}


/* EN: Stop only after the first reply poll has had a chance to observe pending.
 * RU: Отменить ожидание после первого опроса, вернувшего состояние ожидания. */
static int stop_after_poll(void *state)
{
    unsigned int *checks = state;
    return ++*checks == 3U;
}


/* EN: Cancel both operation directions and verify enrollment retirement.
 * RU: Отменить обе операции и проверить снятие участия в канале. */
static int stop_waits(StyxRpcModuleSlot *slot, StyxClient *local, StyxClient *remote)
{
    const StyxRpcNodeCallbacks node = {.transfer = transfer};
    StyxRpcVfsFile *reader = styxrpc_named_open(local, "/stop-read");
    StyxRpcVfsFile *writer = styxrpc_named_open(remote, "/stop-write");
    unsigned int reads = 0, writes = 0;
    const unsigned char value[] = {STYXRPC_NAMED_BOOLEAN, 1};
    unsigned char *frame = NULL;
    size_t size = sizeof(value);
    int ok = reader && writer &&
        styxrpc_named_recv_until(reader, &frame, &size, stop_after_poll, &reads) == STYXRPC_IO_CANCELLED &&
        !frame && size == sizeof(value) &&
        styxrpc_named_send_until(writer, value, sizeof(value), stop_after_poll, &writes) == STYXRPC_IO_CANCELLED;
    free(frame);
    styxrpc_vfs_close(reader, 0);
    styxrpc_vfs_close(writer, 0);
    return ok && styxrpc_module_slot_new_node(slot, "/stop-read", TEST_NODE_READ, &node) == STYXRPC_OK &&
        styxrpc_module_slot_new_node(slot, "/stop-write", TEST_NODE_READ, &node) == STYXRPC_OK;
}


/* EN: Disconnect the shared peer and require a local unanswered read to return.
 * RU: Разорвать связь и проверить возврат локального чтения без пары. */
static int cancellation(StyxClient *local, StyxClient *remote)
{
    StyxRpcVfsFile *waiting = styxrpc_named_open(local, "/cancel");
    if (!waiting)
    {
        return 0;
    }
    styxclientshutdown(remote);
    unsigned char *frame = NULL;
    size_t size;
    int canceled = styxrpc_named_recv(waiting, &frame, &size) != STYXRPC_IO_OK;
    free(frame);
    styxrpc_vfs_close(waiting, 1);
    return canceled;
}


/* EN: Use the library's real remote and local service fids on a connected stream.
 * RU: Использовать настоящие удалённые и локальные fid библиотеки на соединённом потоке. */
static int check(StyxRpcModuleSlot *slot, StyxRpcNamedRegistry *registry)
{
    int fds[2];
    if (!registry || socketpair(AF_UNIX, SOCK_STREAM, 0, fds))
    {
        return 0;
    }
    StyxRpcNamedPeer *peer = styxrpc_named_peer_host(phyposixfd(fds[0], TEST_STREAM), registry,
        TEST_MSIZE, TEST_WAIT_MS, TEST_TAGS);
    StyxClient *remote = styxclientmountphy(phyposixfd(fds[1], TEST_STREAM),
        TEST_MSIZE, "test", "", TEST_WAIT_MS, TEST_TAGS);
    StyxClient *local = styxrpc_named_peer_client(peer);
    int ok = local && remote && lifetime(slot, local, remote) &&
        replaced_parent(slot, local, remote) && stop_waits(slot, local, remote) &&
        cancellation(local, remote);
    styxclientshutdown(remote);
    styxclientfree(remote);
    styxrpc_named_peer_destroy(peer);
    return ok;
}


/* EN: Construct the namespace, run the hosted checks and release in owner order.
 * RU: Создать пространство имён, выполнить проверки и освободить владельцев по порядку. */
int main(void)
{
    const StyxRpcControlSpec spec = {.upload_limit = 1, .upload = upload, .command = command, .status = status};
    StyxRpcControl *control = NULL;
    if (styxrpc_control_create(&spec, &control) != STYXRPC_OK)
    {
        return 1;
    }
    StyxRpcModuleSlot *slot = styxrpc_module_slot_create(control);
    StyxRpcNamedRegistry *registry = styxrpc_module_slot_named_registry(slot);
    int ok = check(slot, registry);
    styxrpc_named_destroy(registry);
    styxrpc_module_slot_destroy(slot);
    styxrpc_control_destroy(control);
    if (!ok)
    {
        fputs("paired named service lifetime check failed\n", stderr);
    }
    return ok ? 0 : 1;
}
