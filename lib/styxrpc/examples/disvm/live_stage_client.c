/* Copyright (c) 2026 bhgv
 * EN: Call real DisVM exports and inspect live VFS variables with an independent C SDK.
 * RU: Вызвать настоящие экспорты DisVM и проверить живые переменные VFS через отдельный SDK C.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_client.h"
#include "styxrpc/graph.h"
#include "styxrpc/hybrid.h"
#include "styxrpc/live_client.h"
#include "styxrpc/module_client.h"
#include "styxrpc/program.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CALLER_PID 37U /* Published reverse context of this C program. */
#define CALL_WAIT 2000U /* Milliseconds to wait for a confirmed RPC reply. */
#define FRAME 2048U /* Enough room for the deliberately small example records. */

static const char module[] = "mounted/stage-library.dis"; /* Exported in the Dis server command. */
static const struct timespec pause_time = {0, 5000000L}; /* Delay between startup polls. */


/* EN: Serve the reverse context until the connected Dis process closes its lane.
 * RU: Обслуживать встречный контекст до закрытия канала процессом Dis. */
static void *dispatch(void *state)
{
    StyxRpcServer *server = state;
    while (styxrpc_server_step(server))
    {
    }
    styxrpc_server_destroy(server);
    return NULL;
}


/* EN: Resolve one published export and copy its compiled schema into the target.
 * RU: Найти опубликованный экспорт и скопировать его компилированную схему в цель. */
static int target(StyxClient *client, const StyxRpcModuleExports *exports, const char *name,
    StyxRpcManagedTarget *out)
{
    for (size_t i = 0; i < exports->count; ++i)
    {
        const StyxRpcModuleImport *method = exports->methods + i;
        if (!strcmp(method->name, name))
        {
            *out = (StyxRpcManagedTarget){.session = client, .module = module, .name = name,
                .signature = method->signature, .schema = method->schema, .pid = CALLER_PID, .wait_ms = CALL_WAIT};
            return 1;
        }
    }
    return 0;
}


/* EN: Encode one declared tuple head as text and its signed big leaf.
 * RU: Закодировать голову-кортеж как текст и знаковое поле big. */
static int head(const char *text, int64_t number, unsigned char *bytes, size_t capacity, size_t *used)
{
    unsigned char fields[96];
    size_t count, extent;
    return fvm_srecord_write(STYXRPC_VALUE_STRING, text, strlen(text), fields, sizeof(fields), &count) == FVM_S_OK &&
        styxrpc_value_integer(STYXRPC_VALUE_BIG, number, fields + count, sizeof(fields) - count, &extent) == STYXRPC_OK &&
        fvm_srecord_write(STYXRPC_VALUE_TUPLE, fields, count + extent, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Send ordered nonempty and nil lists. Verify both returned tuple heads by exact bytes.
 * RU: Передать непустой и nil списки, сверить головы возвращённых кортежей по байтам. */
static int lists(const StyxRpcManagedTarget *call)
{
    unsigned char heads[192], argument[256], reply[FRAME];
    size_t first, second, size;
    StyxRpcManagedReply result;
    if (strcmp(call->schema.arguments, "(l[(sb)])") ||
        !head("☀", INT64_MIN, heads, sizeof(heads), &first) ||
        !head("end", INT64_MAX, heads + first, sizeof(heads) - first, &second) ||
        fvm_srecord_write(STYXRPC_VALUE_ARRAY, heads, first + second, argument, sizeof(argument), &size) != FVM_S_OK ||
        styxrpc_managed_invoke(call, argument, size, reply, sizeof(reply), &result) != STYXRPC_CALL_OK ||
        !head("end", INT64_MAX - 1, heads, sizeof(heads), &first) ||
        !head("☀", INT64_MIN + 1, heads + first, sizeof(heads) - first, &second) ||
        fvm_srecord_write(STYXRPC_VALUE_ARRAY, heads, first + second, argument, sizeof(argument), &size) != FVM_S_OK ||
        result.result.size != size || memcmp(result.result.payload, argument, size))
    {
        return 0;
    }
    return fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, argument, sizeof(argument), &size) == FVM_S_OK &&
        styxrpc_managed_invoke(call, argument, size, reply, sizeof(reply), &result) == STYXRPC_CALL_OK &&
        result.result.size == size && !memcmp(result.result.payload, argument, size);
}


/* EN: Receive the exact signed big seeded by the Dis export on its retained lease.
 * RU: Прочитать точный знаковый big, отправленный экспортом Dis по удержанной аренде. */
static int receive_big(const StyxRpcLeasedReply *result)
{
    if (!result->lease || result->routes.count != 1 || strcmp(result->routes.entries[0].channel.shape, "(b)"))
    {
        return 0;
    }
    const StyxRpcSelection receive = {.count = 1, .guards = {{result->routes.entries[0].channel.key, 1, NULL, 0}}};
    unsigned char request[96], reply[96];
    StyxRpcChannelResult selected;
    FVM_SRecord value;
    size_t used;
    int64_t number;
    return styxrpc_channel_lease_exchange(result->lease, 0, &receive, CALL_WAIT, NULL, 0,
        request, sizeof(request), reply, sizeof(reply), &selected) == STYXRPC_OK &&
        selected.selected == 1 &&
        fvm_srecord_read(selected.value, selected.size, sizeof(reply), &value, &used) == FVM_S_OK &&
        value.tag == STYXRPC_CHANNEL_ELEMENT_TAG && used == selected.size &&
        styxrpc_value_get_integer(STYXRPC_VALUE_BIG, value.payload, value.size, &number) == STYXRPC_OK &&
        number == INT64_MIN;
}


/* EN: Stage an exact hybrid patch from the same reply that yielded a live channel.
 * RU: Подготовить точный патч таблицы из ответа, который выдал живой канал. */
static int table_and_channel(const StyxRpcManagedTarget *call)
{
    unsigned char definition[256] = {0}, arguments[320], reply[FRAME];
    size_t extent, size, used, index;
    FVM_SRecord delta;
    StyxRpcHybrid *table = styxrpc_hybrid_open(NULL, 0);
    StyxRpcLeasedReply result = {0};
    StyxRpcHybrid *changed = NULL;
    StyxRpcHybridRequest read = {.operation = STYXRPC_HYBRID_GET, .key = {.text = 1, .bytes = "wide", .size = 4}};
    const StyxRpcHybridRequest local = {.operation = STYXRPC_HYBRID_SET, .key = {.index = 4}, .value = 17};
    StyxRpcHybridRequest seed = local;
    int valid = table && styxrpc_hybrid_apply(table, &seed) &&
        styxrpc_hybrid_snapshot(table, definition + 1, sizeof(definition) - 1, &extent) &&
        fvm_srecord_write(STYXRPC_VALUE_REF, definition, extent + 1, arguments, sizeof(arguments), &size) == FVM_S_OK &&
        styxrpc_channel_invoke(call, arguments, size, reply, sizeof(reply), &result) == STYXRPC_CALL_OK;
    if (valid)
    {
        const FVM_SRecord baseline = {.tag = STYXRPC_GRAPH_REQUEST_TAG, .payload = arguments, .size = size};
        valid = fvm_srecord_read(result.updates.payload, result.updates.size, FRAME, &delta, &used) == FVM_S_OK &&
            used == result.updates.size && styxrpc_graph_update_index(&call->schema, &baseline, &delta, 1, &index) &&
            !index && (changed = styxrpc_hybrid_stage(table, delta.payload + 1, delta.size - 1)) &&
            styxrpc_hybrid_apply(changed, &read) && read.value == INT64_MAX && receive_big(&result);
    }
    if (valid)
    {
        read.key = local.key;
        valid = styxrpc_hybrid_apply(changed, &read) && read.value == local.value;
    }
    styxrpc_channel_lease_release(result.lease, 0);
    styxrpc_hybrid_close(changed);
    styxrpc_hybrid_close(table);
    return valid;
}


/* EN: Discover checked metadata and use C codecs for the two different call profiles.
 * RU: Обнаружить проверенные метаданные и применить кодеки C двух профилей. */
static int methods(StyxClient *client)
{
    StyxRpcModuleExports exports;
    StyxRpcManagedTarget call;
    if (styxrpc_module_exports(client, module, CALLER_PID, &exports) != STYXRPC_OK)
    {
        fputs("Cannot discover the compiled Dis exports\n", stderr);
        return 0;
    }
    if (!target(client, &exports, "words", &call) || !lists(&call))
    {
        fputs("Typed list call failed\n", stderr);
        return 0;
    }
    if (!target(client, &exports, "update", &call) || !table_and_channel(&call))
    {
        fputs("Hybrid delta or returned channel failed\n", stderr);
        return 0;
    }
    return 1;
}


/* EN: Wait for and capture the published record without accepting the backing directory.
 * RU: Дождаться и захватить опубликованную запись, не принимая базовый каталог. */
static int ready(StyxClient *client, StyxRpcLive **root)
{
    for (unsigned i = 0; i < 600; ++i)
    {
        if (styxrpc_live_open_until(client, "live/.var", 1, root, NULL, NULL) == STYXRPC_IO_OK)
        {
            return 1;
        }
        nanosleep(&pause_time, NULL);
    }
    return 0;
}


/* EN: Check nil, empty and binary-safe UTF-8 through a borrowed writable string field.
 * RU: Проверить nil, пустую строку и UTF-8 с нулём через заимствованное записываемое поле. */
static int replace_text(const StyxRpcLive *name)
{
    StyxRpcLiveValue data;
    const StyxRpcLiveValue empty = {0};
    const StyxRpcLiveValue text = {.size = 5, .text = {'x', 0, 'e', 'n', 'd'}};
    return styxrpc_live_read_until(name, 0, &data, NULL, NULL) == STYXRPC_IO_OK && data.nil &&
        styxrpc_live_write_until(name, 0, &empty, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_read_until(name, 0, &data, NULL, NULL) == STYXRPC_IO_OK && !data.nil && !data.size &&
        styxrpc_live_write_until(name, 0, &text, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_read_until(name, 0, &data, NULL, NULL) == STYXRPC_IO_OK && !data.nil &&
        data.size == text.size && !memcmp(data.text, text.text, text.size);
}


/* EN: Use independent fields of the mounted record, confirming each write before signalling the owner.
 * RU: Работать с независимыми полями записи, подтверждая каждую запись перед сигналом владельцу. */
static int variables(StyxClient *client)
{
    StyxRpcLive *root = NULL, *numbers = NULL, *name = NULL, *wide = NULL, *table = NULL;
    StyxRpcLiveValue data = {.integer = 63};
    uint32_t length;
    int valid = ready(client, &root) &&
        styxrpc_live_field_until(root, "numbers", &numbers, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_field_until(root, "name", &name, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_field_until(root, "wide", &wide, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_field_until(root, "table", &table, NULL, NULL) == STYXRPC_IO_OK;
    styxrpc_live_close(root, 0);
    valid = valid &&
        styxrpc_live_length_until(numbers, &length, NULL, NULL) == STYXRPC_IO_OK && length == 2 &&
        styxrpc_live_write_until(numbers, 1, &data, NULL, NULL) == STYXRPC_IO_OK &&
        styxrpc_live_read_until(numbers, 0, &data, NULL, NULL) == STYXRPC_IO_OK && data.integer == 63 &&
        replace_text(name) &&
        styxrpc_live_length_until(table, &length, NULL, NULL) == STYXRPC_IO_OK && length == 1 &&
        styxrpc_live_read_until(table, -1, &data, NULL, NULL) == STYXRPC_IO_OK && data.integer == 11 &&
        styxrpc_live_read_until(table, 9, &data, NULL, NULL) == STYXRPC_IO_OK && data.integer == 19 &&
        styxrpc_live_read_until(wide, 0, &data, NULL, NULL) == STYXRPC_IO_OK && data.integer == INT64_MIN;
    if (valid)
    {
        data.integer = INT64_MAX;
        valid = styxrpc_live_write_until(wide, 0, &data, NULL, NULL) == STYXRPC_IO_OK &&
            styxrpc_live_write_until(table, 9, &data, NULL, NULL) == STYXRPC_IO_OK &&
            styxrpc_live_read_until(wide, 0, &data, NULL, NULL) == STYXRPC_IO_OK && data.integer == INT64_MAX;
    }
    styxrpc_live_close(table, 0);
    styxrpc_live_close(wide, 0);
    styxrpc_live_close(name, 0);
    styxrpc_live_close(numbers, 0);
    if (valid)
    {
        size_t used;
        valid = styxrpc_vfs_write_at(client, "sdk-done", 0, "1", 1, &used) == STYXRPC_IO_OK && used == 1;
    }
    return valid;
}


/* EN: Use one inherited stream. Close both Styx directions before joining the dispatcher.
 * RU: Использовать унаследованный поток. Закрыть оба направления Styx перед ожиданием диспетчера. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 3 ? strtol(argv[1], &end, 10) : -1;
    if (fd < 3 || fd > INT_MAX || *end || (strcmp(argv[2], "methods") && strcmp(argv[2], "variables")))
    {
        return 2;
    }
    int reverse = !strcmp(argv[2], "variables");
    StyxRpcRegistration storage[16];
    StyxRpcPrograms programs;
    if (!styxrpc_programs_init(&programs, storage, 16) || !styxrpc_program_add(&programs, CALLER_PID))
    {
        return 1;
    }
    PhyBidir *mux = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 1;
    }
    StyxRpcServer *server = styxrpc_program_server_create(phybidirchannel(mux, !reverse), &programs);
    pthread_t owner;
    if (!server || pthread_create(&owner, NULL, dispatch, server))
    {
        styxrpc_server_destroy(server);
        phybidirfree(mux);
        return 1;
    }
    StyxClient *client = styxclientmountphy(phybidirchannel(mux, reverse), 4096, "stage-sdk", "", CALL_WAIT, 8);
    int valid = client && (reverse ? variables(client) : methods(client));
    if (valid)
    {
        puts("PASS independent C SDK and native DisVM");
    }
    else if (reverse)
    {
        fputs("Live collection read or replacement failed\n", stderr);
    }
    styxclientshutdown(client);
    styxclientfree(client);
    phybidirfree(mux);
    pthread_join(owner, NULL);
    return valid ? 0 : 1;
}
