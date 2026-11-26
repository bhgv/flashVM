/* Copyright (c) 2026 bhgv
 * EN: Check the paired module reference wire shape and both exact-fid resolvers.
 * RU: Проверка сетевого формата ссылки на модуль и обоих способов открытия точного fid.
 */
#include "styxrpc/module_ref.h"
#include "styxrpc/module_slot.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* Ordered local socket transport. */
#define COUNTER_PATH "mounted/ref.dis/.var/counter" /* Name reused after publication changes. */

typedef struct Fixture
{
    StyxRpcModuleSlot *slot;    /* Borrowed by the server until it joins. */
    StyxRpcModule *replacement; /* Image installed by the server owner's restart callback. */
} Fixture;


/* EN: Decode a fixed high-bit Qid, then reject wrong scope and malformed frames.
 * RU: Декодировать Qid со старшим битом, затем отклонить чужую область и повреждённую запись. */
static int wire_contract(void)
{
    const StyxRpcModuleRef ref = {UINT64_C(0x8123456789abcdef), STYXRPC_MODULE_REF_REVERSE, 1, COUNTER_PATH};
    const unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE] = {0xb4};
    const unsigned char other[STYXRPC_MODULE_REF_SCOPE_SIZE] = {0x4b};
    const unsigned char prefix[] = {'M', 'V', 1, 3, 0x81, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    unsigned char bytes[STYXRPC_MODULE_REF_WIRE_MAX];
    StyxRpcModuleRef copy = {0}, refused = {0};
    size_t size = styxrpc_module_ref_encode(&ref, scope, bytes, sizeof(bytes));
    if (size != STYXRPC_MODULE_REF_HEADER + strlen(COUNTER_PATH) ||
        memcmp(bytes, prefix, sizeof(prefix)) || !styxrpc_module_ref_decode(bytes, size, scope, &copy) ||
        copy.node != ref.node || copy.owner != ref.owner || copy.writable != 1 || strcmp(copy.path, ref.path) ||
        styxrpc_module_ref_decode(bytes, size, other, &refused))
    {
        return 0;
    }
    bytes[size - 1] = 0;
    return !styxrpc_module_ref_decode(bytes, size, scope, &refused) && !refused.node &&
        !styxrpc_module_ref_path("mounted/ref.dis/.var/prefix/counter") &&
        !styxrpc_module_ref_path("mounted/../.var/counter");
}


/* EN: Read the fixture's live Int32 from state into out on the current owner.
 * RU: Прочитать живой Int32 из state в out в потоке текущего владельца. */
static int get(void *state, int32_t *out)
{
    *out = *(int32_t *)state;
    return 1;
}


/* EN: Set the fixture's Int32 in state on the current owner.
 * RU: Записать Int32 в state в потоке текущего владельца. */
static int set(void *state, int32_t value)
{
    *(int32_t *)state = value;
    return 1;
}


/* EN: Keep a minimal scalar export beside the tested variable.
 * RU: Оставить минимальную скалярную функцию рядом с тестируемой переменной. */
static int echo(void *state, int64_t argument, int64_t *result)
{
    (void)state;
    *result = argument;
    return 1;
}


/* EN: Build a module borrowing value. The replacement can expose it read-only.
 * RU: Собрать модуль с заимствованным value. Замена может открывать его только для чтения. */
static StyxRpcModule *image(int32_t *value, int writable)
{
    const StyxRpcModuleEntry entry = {STYXRPC_MODULE_INT, {.integer = {"echo", STYXRPC_INT_SIGNATURE, echo, NULL}}};
    const StyxRpcModuleVariable variable = {"counter", {value, get, writable ? set : NULL}};
    const StyxRpcModuleSpec spec = {"ref.dis", &entry, 1, &variable, 1, NULL, NULL};
    StyxRpcModule *module = NULL;
    return styxrpc_module_create(&spec, &module) == STYXRPC_OK ? module : NULL;
}


/* EN: Refuse uploads because the fixture already holds a replacement module.
 * RU: Отклонить загрузку, поскольку тест уже держит модуль для замены. */
static int upload(void *state, const void *bytes, size_t size)
{
    (void)state;
    (void)bytes;
    (void)size;
    return 0;
}


/* EN: Replace the slot in the Styx .state request on its owning thread.
 * RU: Заменить слот в Styx-запросе к .state в потоке владельца. */
static int command(void *state, StyxRpcControlCommand request)
{
    const Fixture *test = state;
    return request == STYXRPC_CONTROL_RESTART &&
        styxrpc_module_slot_set(test->slot, test->replacement) == STYXRPC_OK;
}


/* EN: Return an empty status for this fixture's control file.
 * RU: Вернуть пустой статус файла управления теста. */
static int status(void *state, char *text, size_t capacity, size_t *used)
{
    (void)state;
    (void)text;
    (void)capacity;
    *used = 0;
    return 1;
}


/* EN: Take and retire server state; the caller owns the slot before and after this worker.
 * RU: Принять и освободить сервер; до и после работы потока слотом владеет вызывающий. */
static void *serve(void *server)
{
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN: Issue the exact service command on the privileged control fid.
 * RU: Отправить точную служебную команду через fid управления. */
static int restart(StyxClient *client)
{
    StyxFid *file = styxclientwalk(client, ".state");
    StyxTransfer transfer = {0};
    if (file && styxclientopen(file, STYXRPC_FILE_WRITE))
    {
        styxclientwrite(file, 0, "restart", 7, &transfer);
    }
    styxclientclunk(file);
    return transfer.success && transfer.confirmed && !transfer.uncertain && transfer.count == 7;
}


/* EN: Hold remote and local aliases across a real publication replacement.
 * The caller resumes the slot only after the dispatch worker joins.
 * RU: Удержать удалённую и локальную ссылки при реальной замене публикации.
 * Вызывающий возобновляет работу со слотом после завершения диспетчера. */
static int replacement(StyxRpcModuleSlot *slot, StyxRpcFile *local)
{
    int fds[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds))
    {
        return 0;
    }
    StyxRpcServer *server = styxrpc_server_create(phyposixfd(fds[0], STREAM), &styxrpc_module_slot_tree, slot);
    pthread_t worker;
    if (!server || pthread_create(&worker, NULL, serve, server))
    {
        close(fds[1]);
        styxrpc_server_destroy(server);
        return 0;
    }
    StyxClient *client = styxclientmountphy(phyposixfd(fds[1], STREAM), 4096, "test", "", 1500, 4);
    StyxRpcModuleRef old = {0, STYXRPC_MODULE_REF_FORWARD, 1, COUNTER_PATH};
    StyxRpcModuleRef fresh = {0, STYXRPC_MODULE_REF_FORWARD, 0, COUNTER_PATH};
    StyxRpcVfsFile *view = NULL, *alias = NULL, *denied = NULL, *current = NULL;
    int32_t value = 0;
    int good = client &&
        styxrpc_vfs_int_open_identified(client, old.path, 1, &view, &old.node) == STYXRPC_IO_OK &&
        styxrpc_module_ref_open_remote(client, &old, &alias) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_read(alias, &value) == STYXRPC_IO_OK && value == -73 &&
        styxrpc_vfs_int_write(alias, -4) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_read(view, &value) == STYXRPC_IO_OK && value == -4 &&
        restart(client) &&
        styxrpc_vfs_int_read(alias, &value) != STYXRPC_IO_OK;
    old.writable = 0;
    good = good && styxrpc_module_ref_open_remote(client, &old, &denied) == STYXRPC_IO_NOT_OPEN && !denied &&
        styxrpc_vfs_int_open_identified(client, fresh.path, 0, &current, &fresh.node) == STYXRPC_IO_OK &&
        old.node != fresh.node && styxrpc_vfs_int_read(current, &value) == STYXRPC_IO_OK && value == 92;
    fresh.writable = 1;
    good = good && styxrpc_module_ref_open_remote(client, &fresh, &denied) == STYXRPC_IO_NOT_OPEN && !denied;
    styxrpc_vfs_close(view, 0);
    styxrpc_vfs_close(alias, 0);
    styxrpc_vfs_close(denied, 0);
    styxrpc_vfs_close(current, 0);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    StyxRpcFile stale = {0};
    good = good && styxrpc_module_ref_read_local(local, &value) != STYXRPC_OK &&
        styxrpc_module_ref_write_local(local, 0) != STYXRPC_OK &&
        styxrpc_module_ref_open_local(slot, &old, &stale) == STYXRPC_INVALID;
    styxrpc_file_close(&stale);
    return good;
}


/* EN: Copy the exact named child so the fixture does not depend on directory order.
 * RU: Скопировать дочерний узел по имени, чтобы тест не зависел от порядка каталога. */
static int find(StyxRpcModuleSlot *slot, uint64_t parent, const char *name, StyxRpcEntry *entry)
{
    for (size_t i = 0; styxrpc_dir_entry(&styxrpc_module_slot_tree.files, slot, parent, i, entry) == STYXRPC_OK; ++i)
    {
        if (!strcmp(entry->name, name))
        {
            return 1;
        }
    }
    return 0;
}


/* EN: Check wire data and owner/client operations, then close fids before their slot.
 * RU: Проверить формат и операции владельца и клиента, закрыть fid перед уничтожением слота. */
int main(void)
{
    int32_t original = 41, next = 92;
    StyxRpcModule *first = image(&original, 1);
    Fixture test = {NULL, image(&next, 0)};
    const StyxRpcControlSpec spec = {&test, 1, upload, command, status};
    StyxRpcControl *control = NULL;
    if (styxrpc_control_create(&spec, &control) == STYXRPC_OK)
    {
        test.slot = styxrpc_module_slot_create(control);
    }
    StyxRpcModuleRef ref = {0, STYXRPC_MODULE_REF_FORWARD, 1, COUNTER_PATH};
    StyxRpcFile local = {0};
    StyxRpcEntry mounted, module, vars, counter;
    int good = wire_contract() && first && test.replacement &&
        styxrpc_module_slot_set(test.slot, first) == STYXRPC_OK &&
        find(test.slot, 0, "mounted", &mounted) && find(test.slot, mounted.id, "ref.dis", &module) &&
        find(test.slot, module.id, ".var", &vars) && find(test.slot, vars.id, "counter", &counter);
    if (good)
    {
        ref.node = counter.id;
        good = styxrpc_module_ref_open_local(test.slot, &ref, &local) == STYXRPC_OK &&
            styxrpc_module_ref_write_local(&local, -73) == STYXRPC_OK && original == -73 &&
            replacement(test.slot, &local) && original == -4 && next == 92;
    }
    styxrpc_file_close(&local);
    styxrpc_module_slot_destroy(test.slot);
    styxrpc_control_destroy(control);
    styxrpc_module_destroy(first);
    styxrpc_module_destroy(test.replacement);
    if (!good)
    {
        fputs("scoped module ref wire or publication lifetime check failed\n", stderr);
    }
    return good ? 0 : 1;
}
