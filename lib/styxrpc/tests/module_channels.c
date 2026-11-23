/* Copyright (c) 2026 bhgv
 * EN: Check token lifetime, fid isolation and rollback for C module channels.
 * RU: Проверка срока токенов, изоляции fid и отката выдач каналов модуля C.
 */
#include "styxrpc/module_channels.h"
#include "styxrpc/channel_handlers.h"
#include <stdio.h>
#include <string.h>

typedef struct Counter
{
    unsigned writes; /* Calls to the authenticated resource. */
    unsigned closes; /* Exact successful opens retired. */
    unsigned calls;  /* Accepted factory calls. */
    unsigned releases; /* Resources retired by the factory adapter. */
    unsigned char value; /* Byte stored through the lease. */
} Counter;


/* EN: Emit deterministic bytes for tests and optionally simulate entropy failure.
 * RU: Выдать предсказуемые байты теста и при необходимости вернуть ошибку генератора. */
static int entropy(void *context, void *bytes, size_t length)
{
    const int *enabled = context;
    memset(bytes, 0x42, length);
    return *enabled;
}


/* EN: Return the stored byte only at the reply origin.
 * RU: Вернуть сохранённый байт только в начале ответа. */
static int read_byte(void *state, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const Counter *counter = state;
    if (offset || capacity < 1)
    {
        return 0;
    }
    *(unsigned char *)bytes = counter->value;
    *size = 1;
    return 1;
}


/* EN: Require that the module bus translated the first data write to offset zero.
 * RU: Проверить, что шина модуля перенесла первую запись данных к смещению ноль. */
static int write_byte(void *state, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    Counter *counter = state;
    if (offset || count != 1)
    {
        return 0;
    }
    counter->value = *(const unsigned char *)bytes;
    counter->writes++;
    *size = 1;
    return 1;
}


/* EN: Count each successfully bound handle released by clunk or revocation.
 * RU: Подсчитать открытые дескрипторы, освобождённые clunk или отзывом. */
static void close_byte(void *state)
{
    ((Counter *)state)->closes++;
}


/* EN: Authenticate one already opened bus fid using a complete token.
 * RU: Проверить открытый fid шины по полному токену. */
static int authorize(const StyxRpcResource *bus, void *cookie, const unsigned char *token)
{
    size_t extent;
    return bus->write(cookie, 0, token, STYXRPC_CHANNEL_TOKEN_SIZE, &extent) &&
        extent == STYXRPC_CHANNEL_TOKEN_SIZE;
}


/* EN: Exercise concurrent leases, reuse and a sticky failed authentication attempt.
 * RU: Проверить одновременные выдачи, повторное использование и отказ на одном fid. */
static int exercise(void)
{
    int enabled = 0;
    Counter a = {0}, b = {0};
    StyxRpcModuleChannels channels;
    StyxRpcModuleChannel slots[2];
    const StyxRpcResource left = {&a, NULL, read_byte, write_byte, close_byte};
    const StyxRpcResource right = {&b, NULL, read_byte, write_byte, close_byte};
    unsigned char first[STYXRPC_CHANNEL_TOKEN_SIZE] = {0};
    unsigned char second[STYXRPC_CHANNEL_TOKEN_SIZE], fresh[STYXRPC_CHANNEL_TOKEN_SIZE];
    const unsigned char zero[STYXRPC_CHANNEL_TOKEN_SIZE] = {0};
    if (!styxrpc_module_channels_init(&channels, slots, 2, entropy, &enabled) ||
        styxrpc_module_channels_publish(&channels, &left, first) || memcmp(first, zero, sizeof(first)))
    {
        return 0;
    }
    enabled = 1;
    if (!styxrpc_module_channels_publish(&channels, &left, first) ||
        !styxrpc_module_channels_publish(&channels, &right, second) ||
        !memcmp(first, second, sizeof(first)) || styxrpc_module_channels_publish(&channels, &right, fresh))
    {
        return 0;
    }

    StyxRpcResource bus = styxrpc_module_channels_resource(&channels);
    void *x = NULL, *y = NULL, *invalid = NULL;
    size_t extent;
    unsigned char value = 0;
    unsigned access = STYXRPC_READ | STYXRPC_WRITE;
    int good = bus.open(bus.context, access, &x) && bus.open(bus.context, access, &y) &&
        bus.open(bus.context, access, &invalid);
    if (good)
    {
        good = !bus.read(x, 0, &value, 1, &extent) && !authorize(&bus, invalid, zero) &&
            !authorize(&bus, invalid, first) && authorize(&bus, x, first) && authorize(&bus, y, second) &&
            bus.write(x, STYXRPC_CHANNEL_TOKEN_SIZE, "a", 1, &extent) && extent == 1 &&
            bus.write(y, STYXRPC_CHANNEL_TOKEN_SIZE, "b", 1, &extent) &&
            a.writes == 1 && b.writes == 1 &&
            styxrpc_module_channels_revoke(&channels, first) && a.closes == 1 && b.closes == 0 &&
            styxrpc_module_channels_publish(&channels, &left, fresh) && memcmp(first, fresh, sizeof(first)) &&
            !styxrpc_module_channels_revoke(&channels, first) && !authorize(&bus, x, fresh) &&
            !bus.read(x, 0, &value, 1, &extent) && !bus.write(x, STYXRPC_CHANNEL_TOKEN_SIZE, "q", 1, &extent) &&
            bus.read(y, 0, &value, 1, &extent) && extent == 1 && value == 'b';
    }
    if (x)
    {
        bus.close(x);
    }
    if (y)
    {
        bus.close(y);
    }
    if (invalid)
    {
        bus.close(invalid);
    }
    return good && a.closes == 1 && b.closes == 1;
}


/* EN: Count retirement after the module has closed authenticated operations.
 * RU: Подсчитать освобождение после закрытия проверенных операций модулем. */
static void release_channel(void *state)
{
    ((Counter *)state)->releases++;
}


/* EN: Provide an owned test resource recording all writes, closes and releases.
 * RU: Выдать собственный тестовый ресурс со счётчиками записи, закрытия и освобождения. */
static void output_channel(Counter *counter, StyxRpcChannelOutput *output)
{
    counter->calls++;
    *output = (StyxRpcChannelOutput){
        .resource = {counter, NULL, read_byte, write_byte, close_byte},
        .key = 1, .release = release_channel
    };
}


/* EN: Issue a resource for one verified zero-argument scoped call.
 * RU: Выдать ресурс одного проверенного вызова без аргументов. */
static StyxRpcStatus provide(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    Counter *counter = context;
    if (call->pid != 42 || call->arguments.size || call->count)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    output_channel(counter, output);
    return STYXRPC_OK;
}


/* EN: Find one public named child in the module's enumeration.
 * RU: Найти дочерний узел по имени в публичном списке модуля. */
static const StyxRpcEntry *find(const StyxRpcVfs *files, void *context, uint64_t parent, const char *name)
{
    for (size_t i = 0; ; i++)
    {
        const StyxRpcEntry *entry = files->enumerate(context, parent, i);
        if (!entry || !strcmp(entry->name, name))
        {
            return entry;
        }
    }
}


/* EN: Stage an actual scoped request, authenticate its bus and revoke it through function clunk.
 * RU: Принять настоящий контекстный запрос, проверить шину и отозвать её через clunk функции. */
static int scoped_factory(void)
{
    int enabled = 1;
    Counter state = {0};
    StyxRpcModuleChannels registry;
    StyxRpcModuleChannel slot;
    StyxRpcChannelModule module;
    const StyxRpcChannelExport exported = {"port", UINT32_C(0x421b2140), {"()", "(c[i])"}, provide, &state};
    const StyxRpcVfs *files = &styxrpc_channel_module_tree.files;
    const unsigned access = STYXRPC_READ | STYXRPC_WRITE;
    unsigned char pid[24], request[32], response[128], byte;
    size_t prefix, length, used;
    if (!styxrpc_module_channels_init(&registry, &slot, 1, entropy, &enabled) ||
        styxrpc_channel_module_init(&module, "port.dis", &exported, 1, &registry) != STYXRPC_OK ||
        styxrpc_value_integer(STYXRPC_VALUE_BIG, 42, pid, sizeof(pid), &prefix) != STYXRPC_OK ||
        fvm_srecord_write(STYXRPC_GRAPH_REQUEST_TAG, pid, prefix, request, sizeof(request), &length) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcEntry *mounted = find(files, &module, 0, "mounted");
    const StyxRpcEntry *owner = mounted ? find(files, &module, mounted->id, "port.dis") : NULL;
    const StyxRpcEntry *function = owner ? find(files, &module, owner->id, "port") : NULL;
    const StyxRpcEntry *bus = owner ? find(files, &module, owner->id, ".unibus") : NULL;
    void *call = NULL, *operation = NULL;
    int valid = function && bus && files->open(&module, function->id, access, &call) &&
        files->write(call, 0, request, 1, &used) && used == 1 && !state.calls &&
        files->write(call, 1, request + 1, length - 1, &used) && used == length - 1 && state.calls == 1 &&
        files->read(call, 0, response, sizeof(response), &used) && used &&
        !files->write(call, length, request, length, &used) && state.calls == 1 &&
        files->open(&module, bus->id, access, &operation) &&
        files->write(operation, 0, slot.token, sizeof(slot.token), &used) && used == sizeof(slot.token) &&
        files->write(operation, STYXRPC_CHANNEL_TOKEN_SIZE, "a", 1, &used) && used == 1 &&
        files->read(operation, 0, &byte, 1, &used) && used == 1 && byte == 'a';
    if (call)
    {
        files->close(call);
    }
    if (operation)
    {
        valid = valid && !files->read(operation, 0, &byte, 1, &used) && state.closes == 1 && state.releases == 1;
        files->close(operation);
    }
    return valid && state.closes == 1 && state.releases == 1;
}


/* EN: Stage the same typed resource after checking one admitted int-channel argument.
 * RU: Подготовить тот же ресурс после проверки одного аргумента-канала int. */
static StyxRpcStatus provide_from_argument(void *context, const StyxRpcChannelCall *call, StyxRpcChannelOutput *output)
{
    if (call->pid != 42 || call->count != 1 || call->grants[0].key != 7 || strcmp(call->grants[0].shape, "(i)"))
    {
        return STYXRPC_REMOTE_ERROR;
    }
    output_channel(context, output);
    return STYXRPC_OK;
}


/* EN: Open a new leased-result fid and execute its complete request on the owner.
 * RU: Открыть новый fid арендованного результата и выполнить запрос у владельца. */
static int stage_result(StyxRpcChannelHandlers *module, uint64_t id, void **cookie)
{
    const StyxRpcVfs *files = &styxrpc_channel_handlers_tree.files;
    const StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_CURRENT, .channel = {7, "(i)"}};
    unsigned char pid[32], leaf[32], frame[96];
    size_t pid_size, leaf_size, length, used;
    return styxrpc_value_integer(STYXRPC_VALUE_BIG, 42, pid, sizeof(pid), &pid_size) == STYXRPC_OK &&
        styxrpc_channel_route_value(&route, leaf, sizeof(leaf), &leaf_size) == STYXRPC_OK &&
        fvm_srecord_write_pair(STYXRPC_GRAPH_REQUEST_TAG, pid, pid_size, leaf, leaf_size,
            frame, sizeof(frame), &length) == FVM_S_OK &&
        files->open(module, id, STYXRPC_READ | STYXRPC_WRITE, cookie) &&
        files->write(*cookie, 0, frame, length, &used) && length == used &&
        files->read(*cookie, 0, frame, sizeof(frame), &used) && !used &&
        styxrpc_channel_handlers_step(module);
}


/* EN: Compare an issued response's channel route with the registry's exact token.
 * RU: Сверить маршрут канала ответа с точным токеном в реестре. */
static int issued(void *call, const unsigned char *token)
{
    const StyxRpcVfs *files = &styxrpc_channel_handlers_tree.files;
    unsigned char bytes[128];
    FVM_SRecord envelope, result;
    StyxRpcChannelRoutes routes;
    size_t size, used, child;
    return files->read(call, 0, bytes, sizeof(bytes), &size) && size &&
        fvm_srecord_read(bytes, size, sizeof(bytes), &envelope, &used) == FVM_S_OK && used == size &&
        fvm_srecord_read(envelope.payload, envelope.size, envelope.size, &result, &child) == FVM_S_OK &&
        styxrpc_channel_export_routes("(c[i])", envelope.payload, child, &routes) == STYXRPC_OK &&
        routes.count == 1 && routes.entries[0].channel.key == 1 &&
        !memcmp(routes.entries[0].token, token, STYXRPC_CHANNEL_TOKEN_SIZE);
}


/* EN: Check lazy publication, exhausted registry rollback and revocation of active bus fids.
 * RU: Проверить публикацию по чтению, откат при полном реестре и отзыв активных fid шины. */
static int owner_factory(StyxRpcChannelHandlers *module, StyxRpcModuleChannels *registry, Counter *state)
{
    const StyxRpcVfs *files = &styxrpc_channel_handlers_tree.files;
    const StyxRpcEntry *function = find(files, module, 2, "relay");
    const StyxRpcEntry *bus = find(files, module, 2, ".unibus");
    unsigned char response[128], byte = 0;
    void *abandoned = NULL, *first = NULL, *overflow = NULL, *operation = NULL;
    size_t used;
    int valid = function && bus && stage_result(module, function->id, &abandoned) &&
        files->read(abandoned, 1, response, sizeof(response), &used) && !used && !registry->serial;
    if (abandoned)
    {
        files->close(abandoned);
    }
    valid = valid && state->releases == 1 && !registry->serial &&
        stage_result(module, function->id, &first) && !registry->serial &&
        issued(first, registry->slots[0].token) && registry->serial == 1 &&
        files->open(module, bus->id, STYXRPC_READ | STYXRPC_WRITE, &operation) &&
        files->write(operation, 0, registry->slots[0].token, STYXRPC_CHANNEL_TOKEN_SIZE, &used) &&
        files->write(operation, STYXRPC_CHANNEL_TOKEN_SIZE, "v", 1, &used) && used == 1 &&
        stage_result(module, function->id, &overflow) &&
        files->read(overflow, 0, response, sizeof(response), &used) && used &&
        state->releases == 2 && registry->serial == 1 &&
        files->read(operation, 0, &byte, 1, &used) && used == 1 && byte == 'v';
    if (overflow)
    {
        files->close(overflow);
    }
    if (first)
    {
        files->close(first);
    }
    if (operation)
    {
        valid = valid && !files->read(operation, 0, &byte, 1, &used) && state->closes == 1 && state->releases == 3;
        files->close(operation);
    }
    return valid && state->closes == 1 && state->releases == 3 && state->calls == 3;
}


/* EN: Create an owner-queued SDK module with one result slot and test its adapter.
 * RU: Создать модуль SDK с очередью и одним слотом и проверить его адаптер. */
static int asynchronous_factory(void)
{
    int enabled = 1;
    Counter state = {0};
    StyxRpcModuleChannels registry;
    StyxRpcModuleChannel slot;
    const StyxRpcChannelExport exported = {"relay", UINT32_C(0x9c30df25), {"(c[i])", "(c[i])"},
        provide_from_argument, &state};
    if (!styxrpc_module_channels_init(&registry, &slot, 1, entropy, &enabled))
    {
        return 0;
    }
    StyxRpcChannelHandlers *module = styxrpc_channel_handlers_create_with_results("relay.dis",
        NULL, 0, &exported, 1, &registry);
    int valid = module && owner_factory(module, &registry, &state);
    styxrpc_channel_handlers_destroy(module);
    return valid;
}


/* EN: Run the hosted bus regression without needing a VM or an entropy device.
 * RU: Выполнить регрессию шины без VM и системного источника энтропии. */
int main(void)
{
    if (!exercise() || !scoped_factory() || !asynchronous_factory())
    {
        fputs("module channel lease isolation failed\n", stderr);
        return 1;
    }
    return 0;
}
