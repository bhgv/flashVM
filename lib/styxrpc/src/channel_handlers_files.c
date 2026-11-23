/* Copyright (c) 2026 bhgv
 * EN: Serve scoped call fids and issue result leases on the Styx dispatcher.
 * RU: Обслуживание контекстных fid и выдача аренд результата на диспетчере Styx.
 */
#include "channel_handlers_internal.h"
#include "channel_module_internal.h"
#include "styxrpc/graph.h"
#include "call_record.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>


/* EN: Look up the child's stable entry by directory ID and index.
 * RU: Найти стабильную запись потомка по ID каталога и индексу. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    const StyxRpcChannelHandlers *module = context;
    return styxrpc_module_child(module->entries, module->count + 2 + (module->registry != NULL), directory, index);
}


/* EN: Return metadata content and extent for the opened non-function ID.
 * RU: Вернуть данные метаданных и длину для открытого ID без функции. */
static const void *metadata(const StyxRpcChannelHandlers *module, uint64_t id, size_t *size)
{
    if (id == INTERFACE)
    {
        *size = module->profile_size;
        return module->profile;
    }
    if (id == FIRST_EXPORT + module->count)
    {
        *size = sizeof(STYXRPC_POSITIONAL_GRAPH) - 1;
        return STYXRPC_POSITIONAL_GRAPH;
    }
    *size = sizeof(STYXRPC_CHANNEL_VERSION) - 1;
    return STYXRPC_CHANNEL_VERSION;
}


/* EN: Describe one stable node, including the exact size of each marker.
 * RU: Описать стабильный узел с точным размером каждого маркера. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcChannelHandlers *module = context;
    if (!styxrpc_module_stat(module->entries, module->count + 2 + (module->registry != NULL), module->profile_size, id, node))
    {
        return 0;
    }
    size_t marker = FIRST_EXPORT + module->count;
    if (id == marker || id == marker + 1)
    {
        size_t length;
        metadata(module, id, &length);
        node->length = length;
    }
    return 1;
}


/* EN: Open a read-only metadata file or allocate disjoint buffers for a function FID.
 * RU: Открыть метаданные на чтение либо выделить раздельные буферы для fid функции. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    StyxRpcChannelHandlers *module = context;
    size_t marker = FIRST_EXPORT + module->count;
    int function = id >= FIRST_EXPORT && id < marker;
    int bus = module->registry && id == marker + 2;
    if (id < INTERFACE || id > marker + 1 + (module->registry != NULL) ||
        access != ((function || bus) ? STYXRPC_READ | STYXRPC_WRITE : STYXRPC_READ))
    {
        return 0;
    }
    int copied = function && id - FIRST_EXPORT < module->copied_count;
    size_t extra = (copied ? 4U : function ? 2U : 0U) * FRAME;
    HandlerFile *file = calloc(1, sizeof(*file) + extra);
    if (!file)
    {
        return 0;
    }
    file->module = module;
    file->id = id;
    if (copied)
    {
        file->function = module->functions + (id - FIRST_EXPORT);
    }
    else if (function)
    {
        file->channel = module->results + (id - FIRST_EXPORT - module->copied_count);
    }
    else if (bus)
    {
        file->bus = styxrpc_module_channels_resource(module->registry);
        if (!file->bus.open(file->bus.context, access, &file->bus_cookie))
        {
            free(file);
            return 0;
        }
    }
    *cookie = file;
    return 1;
}


/* EN: Accept a contiguous frame, then acknowledge the validated job without running it.
 * RU: Принять последовательный кадр, затем подтвердить проверенную работу без её исполнения. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    HandlerFile *file = cookie;
    if (file->bus.write)
    {
        return file->bus.write(file->bus_cookie, offset, bytes, count, size);
    }
    const StyxRpcSchema *schema = file->function ? &file->function->schema :
        file->channel ? &file->channel->schema : NULL;
    if (!schema || file->submitted || file->rejected || offset != file->staged ||
        !count || count > FRAME - file->staged)
    {
        return 0;
    }
    memcpy(file->storage + file->staged, bytes, count);
    file->staged += count;
    FVM_SRecord record;
    size_t consumed;
    FVM_SResult parsed = fvm_srecord_read(file->storage, file->staged, FRAME, &record, &consumed);
    if (parsed == FVM_S_MORE && file->staged < FRAME)
    {
        *size = count;
        return 1;
    }
    if (parsed != FVM_S_OK || consumed != file->staged || record.tag != STYXRPC_GRAPH_REQUEST_TAG ||
        styxrpc_channel_call(schema, file->storage, file->staged, &file->call) != STYXRPC_OK ||
        !styxrpc_handlers_submit(file))
    {
        file->rejected = 1;
        return 0;
    }
    file->submitted = 1;
    *size = count;
    return 1;
}


/* EN: Publish the staged lease on the dispatcher before exposing its reply.
 * RU: Опубликовать подготовленную аренду у диспетчера до выдачи ответа. */
static void publish(HandlerFile *file)
{
    file->staged_output = 0;
    unsigned char *reply = file->storage + FRAME;
    if (styxrpc_channel_output_publish(&file->channel->schema, &file->call, file->module->registry,
        &file->output, file->token, reply, FRAME, &file->ready))
    {
        file->published = file->output.key != 0;
        return;
    }
    styxrpc_handlers_release_output(file);
    styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, FRAME, &file->ready);
}


/* EN: Copy a bounded immutable range into the Styx dispatcher reply.
 * RU: Скопировать ограниченный неизменяемый диапазон в ответ диспетчера Styx. */
static void read_range(const void *source, size_t length, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    size_t remaining = offset < length ? length - (size_t)offset : 0;
    *size = remaining < capacity ? remaining : capacity;
    if (*size)
    {
        memcpy(bytes, (const unsigned char *)source + (size_t)offset, *size);
    }
}


/* EN: Copy metadata or a confirmed reply. An unfinished call reads as empty.
 * RU: Скопировать метаданные или готовый ответ. Ожидающий вызов читается пустым. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    HandlerFile *file = cookie;
    StyxRpcChannelHandlers *module = file->module;
    if (file->bus.read)
    {
        return file->bus.read(file->bus_cookie, offset, bytes, capacity, size);
    }
    if (!file->function && !file->channel)
    {
        size_t length;
        const void *source = metadata(module, file->id, &length);
        read_range(source, length, offset, bytes, capacity, size);
        return 1;
    }
    pthread_mutex_lock(&module->lock);
    int pending = file->pending;
    pthread_mutex_unlock(&module->lock);
    if (pending || (file->staged_output && (offset || !capacity)))
    {
        *size = 0;
        return 1;
    }
    if (file->staged_output)
    {
        publish(file);
    }
    if (file->rejected || (file->submitted && !file->ready))
    {
        return 0;
    }
    read_range(file->storage + FRAME, file->ready, offset, bytes, capacity, size);
    return 1;
}


/* EN: Revoke a published token before dropping its resource or detach an active call.
 * RU: Отозвать опубликованный токен до освобождения ресурса либо отсоединить активный вызов. */
static void close_file(void *cookie)
{
    HandlerFile *file = cookie;
    if (!styxrpc_handlers_detach(file))
    {
        return;
    }
    if (file->bus.close)
    {
        file->bus.close(file->bus_cookie);
    }
    if (file->published)
    {
        styxrpc_module_channels_revoke(file->module->registry, file->token);
    }
    styxrpc_handlers_dispose(file);
}

const StyxRpcTree styxrpc_channel_handlers_tree = {{enumerate, open_file, read_file, write_file, close_file},
    describe}; /* Immutable Styx tree operations for one connection dispatcher. */
