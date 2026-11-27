/* Copyright (c) 2026 bhgv
 * EN: Check the paired module reference wire shape and both exact-fid resolvers.
 * RU: Проверка сетевого формата ссылки на модуль и обоих способов открытия точного fid.
 */
#include "styxrpc/module_ref.h"
#include "styxrpc/module_client.h"
#include "styxrpc/module_ref_port.h"
#include "styxrpc/module_slot.h"
#include "styxrpc/named_channel.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define STREAM (PHY_STREAM | PHY_ORDERED | PHY_RELIABLE) /* Ordered local socket transport. */
#define MODULE_PATH "mounted/ref.dis" /* Name reused for two compatible images. */
#define COUNTER_PATH "mounted/ref.dis/.var/counter" /* Name reused after publication changes. */
#define SCOPE_PID 123U /* Stable process in the paired control tree. */
#define SCOPE_IDENTITY ".prog/123/.pid" /* Direct RPC-only path. */
#define COUNTER_QID UINT64_C(0x101) /* Local fixture identity, independent of remote Qids. */

static unsigned echo_calls; /* Calls published by the server for cancellation checks. */

typedef struct Fixture
{
    StyxRpcModuleSlot *slot;    /* Borrowed by the server until it joins. */
    StyxRpcModule *replacement; /* Image installed by the server owner's restart callback. */
} Fixture;


/* EN: Read the exact four stored bytes at zero into bytes and report their size.
 * RU: Прочитать четыре байта с нуля в bytes и сообщить их длину. */
static int local_get(void *state, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    if (offset || capacity != 4)
    {
        return 0;
    }
    memcpy(bytes, state, capacity);
    *size = capacity;
    return 1;
}


/* EN: Confirm replacement of the exact four stored bytes at zero.
 * RU: Подтвердить замену четырёх сохранённых байтов с нуля. */
static int local_set(void *state, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    if (offset || count != 4)
    {
        return 0;
    }
    memcpy(state, bytes, count);
    *size = count;
    return 1;
}


/* EN: Bind out to the fixture's live buffer when path, role and exact Qid match.
 * RU: Привязать out к живому буферу теста при совпадении пути, роли и точного Qid. */
static StyxRpcIoOutcome local_open(void *state, const StyxRpcModuleRef *ref, StyxRpcFile *out)
{
    static const StyxRpcVfs ops = {.read = local_get, .write = local_set};
    if (ref->node != COUNTER_QID || ref->owner != STYXRPC_MODULE_REF_REVERSE ||
        strcmp(ref->path, COUNTER_PATH))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    *out = (StyxRpcFile){.ops = &ops, .cookie = state,
        .access = STYXRPC_READ | (ref->writable ? STYXRPC_WRITE : 0)};
    return STYXRPC_IO_OK;
}


/* EN: Issue the current fixture Qid and open the requested exact publication.
 * RU: Выдать текущий Qid теста и открыть указанную точную публикацию. */
static StyxRpcIoOutcome local_capture(void *state, StyxRpcModuleRef *ref, StyxRpcFile *out)
{
    ref->node = COUNTER_QID;
    return local_open(state, ref, out);
}


/* EN: Check first-write owner capture, its forwarded descriptor and write-through after adopt.
 * RU: Проверить захват владельца первой записью, пересылку дескриптора и запись после приёма. */
static int owned_reference(StyxClient *client)
{
    unsigned char value[] = {0, 0, 0, 17};
    const unsigned char changed[] = {0xff, 0xff, 0xff, 0xfe};
    const unsigned char scope[STYXRPC_MODULE_REF_SCOPE_SIZE] = {0xb4};
    const unsigned char path[] = COUNTER_PATH;
    unsigned char command[sizeof(path) + 1] = {STYXRPC_REF_PORT_CAPTURE_LOCAL, 1};
    memcpy(command + 2, path, sizeof(path) - 1);
    StyxRpcModuleRefPort *issuer = styxrpc_module_ref_port_open_owned(client, STYXRPC_MODULE_REF_FORWARD,
        scope, local_open, local_capture, value);
    StyxRpcModuleRefPort *receiver = styxrpc_module_ref_port_open_paired(client, STYXRPC_MODULE_REF_FORWARD,
        scope, local_open, value);
    unsigned char frame[2 + STYXRPC_MODULE_REF_WIRE_MAX] = {STYXRPC_REF_PORT_ADOPT};
    size_t size = 0;
    StyxRpcModuleRef decoded;
    int good = issuer && receiver &&
        styxrpc_module_ref_port_write(receiver, 0, command, sizeof(command)) == STYXRPC_IO_NOT_OPEN &&
        styxrpc_module_ref_port_write(issuer, 0, command, sizeof(command)) == STYXRPC_IO_OK &&
        styxrpc_module_ref_port_read(issuer, STYXRPC_REF_PORT_FRAME, frame + 1,
            sizeof(frame) - 1, &size) == STYXRPC_IO_OK &&
        size > 1 && frame[1] == STYXRPC_NAMED_MODULE_REF &&
        styxrpc_module_ref_decode(frame + 2, size - 1, scope, &decoded) &&
        decoded.node == COUNTER_QID && decoded.owner == STYXRPC_MODULE_REF_REVERSE && decoded.writable &&
        styxrpc_module_ref_port_write(receiver, 0, frame, size + 1) == STYXRPC_IO_OK &&
        styxrpc_module_ref_port_write(receiver, 0, changed, sizeof(changed)) == STYXRPC_IO_OK &&
        !memcmp(value, changed, sizeof(value));
    styxrpc_module_ref_port_close(receiver);
    styxrpc_module_ref_port_close(issuer);
    return good;
}


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
    __atomic_add_fetch(&echo_calls, 1, __ATOMIC_RELEASE);
    *result = argument;
    return 1;
}


/* EN: Cancel early with NULL state, or after the server receives the tested call.
 * RU: Отменить до начала при NULL либо после получения проверяемого вызова сервером. */
static int stop_at(void *state)
{
    const unsigned *sent = state;
    return !sent || __atomic_load_n(&echo_calls, __ATOMIC_ACQUIRE) >= *sent;
}


/* EN: Distinguish cancellation before and after writing; retain the original image.
 * RU: Различить отмену до и после записи; сохранить исходный образ. */
static int cancel_calls(StyxFid *image)
{
    unsigned sent = 1;
    int64_t value = -73;
    return styxrpc_scalar_invoke_at_until(image, NULL, NULL, "echo", STYXRPC_INT_SIGNATURE, 81,
            1000, &value, stop_at, NULL) == STYXRPC_CALL_CANCELLED && value == -73 &&
        styxrpc_scalar_invoke_at_until(image, NULL, NULL, "echo", STYXRPC_INT_SIGNATURE, 83,
            1000, &value, stop_at, &sent) == STYXRPC_CALL_CANCELLED_UNCERTAIN && value == -73;
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


/* EN: Read the stable decimal PID on the already opened reverse fid.
 * RU: Прочитать постоянный десятичный PID с уже открытого встречного fid. */
static int read_scope(StyxFid *pid)
{
    char text[16];
    const char expected[] = "123\n";
    StyxTransfer transfer;
    styxclientread(pid, 0, text, sizeof(text), &transfer);
    return transfer.success && transfer.count == sizeof(expected) - 1 &&
        !memcmp(text, expected, sizeof(expected) - 1);
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
    StyxFid *image = styxrpc_module_open(client, MODULE_PATH);
    StyxFid *pid = styxclientwalk(client, SCOPE_IDENTITY);
    StyxRpcFileInfo info;
    StyxRpcModuleExports exports;
    int64_t echoed = 0;
    int32_t value = 0;
    int good = client && pid && styxclientopen(pid, 0) && read_scope(pid) &&
        styxrpc_vfs_stat(client, SCOPE_IDENTITY, &info) == STYXRPC_IO_NOT_OPEN &&
        owned_reference(client) &&
        styxrpc_module_exports_at(image, 0, &exports) == STYXRPC_OK &&
        styxrpc_module_check_at(image, exports.methods, exports.count, 0) == STYXRPC_OK &&
        cancel_calls(image) &&
        styxrpc_scalar_invoke_at(image, NULL, NULL, "echo", STYXRPC_INT_SIGNATURE, 12, 1000, &echoed) ==
            STYXRPC_CALL_OK && echoed == 12 &&
        styxrpc_vfs_int_open_identified(client, old.path, 1, &view, &old.node) == STYXRPC_IO_OK &&
        (denied = view) != NULL &&
        styxrpc_module_ref_open_remote_until(client, &old, &denied, stop_at, NULL) == STYXRPC_IO_CANCELLED &&
        denied == view;
    denied = NULL;
    good = good && styxrpc_module_ref_open_remote_until(client, &old, &alias, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_write_until(view, -123, stop_at, NULL) == STYXRPC_IO_CANCELLED &&
        styxrpc_vfs_int_read(alias, &value) == STYXRPC_IO_OK && value == -73 &&
        styxrpc_vfs_int_write(alias, -4) == STYXRPC_IO_OK &&
        styxrpc_vfs_int_read(view, &value) == STYXRPC_IO_OK && value == -4 &&
        restart(client) && read_scope(pid) &&
        styxrpc_vfs_int_read(alias, &value) != STYXRPC_IO_OK;
    old.writable = 0;
    good = good && styxrpc_module_ref_open_remote_until(client, &old, &denied, NULL, NULL) == STYXRPC_IO_NOT_OPEN &&
        !denied &&
        styxrpc_module_exports_at(image, 0, &exports) == STYXRPC_INVALID &&
        styxrpc_scalar_invoke_at(image, client, MODULE_PATH, "echo", STYXRPC_INT_SIGNATURE, 15, 1000, &echoed) ==
            STYXRPC_CALL_NOT_SENT && echoed == 12 &&
        styxrpc_scalar_invoke(client, MODULE_PATH, "echo", STYXRPC_INT_SIGNATURE, 15, 1000, &echoed) ==
            STYXRPC_CALL_OK && echoed == 15 &&
        styxrpc_vfs_int_open_identified(client, fresh.path, 0, &current, &fresh.node) == STYXRPC_IO_OK &&
        old.node != fresh.node && styxrpc_vfs_int_read(current, &value) == STYXRPC_IO_OK && value == 92;
    fresh.writable = 1;
    good = good && styxrpc_module_ref_open_remote(client, &fresh, &denied) == STYXRPC_IO_NOT_OPEN && !denied;
    styxrpc_vfs_close(view, 0);
    styxrpc_vfs_close(alias, 0);
    styxrpc_vfs_close(denied, 0);
    styxrpc_vfs_close(current, 0);
    styxrpc_vfs_close(image, 0);
    styxrpc_vfs_close(pid, 0);
    styxclientshutdown(client);
    styxclientfree(client);
    pthread_join(worker, NULL);
    StyxRpcFile stale = {0};
    good = good && echo_calls == 3 && styxrpc_module_ref_read_local(local, &value) != STYXRPC_OK &&
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
    StyxRpcRegistration nodes[7];
    StyxRpcPrograms programs;
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
        styxrpc_programs_init(&programs, nodes, sizeof(nodes) / sizeof(*nodes)) &&
        styxrpc_program_add(&programs, SCOPE_PID) &&
        styxrpc_module_slot_programs(test.slot, &programs) &&
        !styxrpc_module_slot_programs(test.slot, &programs) &&
        !find(test.slot, 0, ".prog", &mounted) &&
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
