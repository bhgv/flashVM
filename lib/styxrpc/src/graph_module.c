/* Copyright (c) 2026 bhgv
 * EN: Stage one positional graph on each function fid; serve its verdict by offset.
 * RU: Собрать позиционный граф на fid функции и выдавать исход по смещению.
 */
#include "styxrpc/graph_module.h"
#include "channel_module_internal.h"
#include "module_nodes.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>

typedef struct GraphFile
{
    const StyxRpcGraphModule *module;  /* Borrowed backing for metadata reads. */
    const StyxRpcGraphExport *function; /* Borrowed export for a function fid. */
    const StyxRpcChannelExport *channel; /* Borrowed scoped export for a function fid. */
    StyxRpcModuleChannels *registry;  /* Borrowed when a channel function or bus is open. */
    StyxRpcChannelOutput active;      /* Channel owned until the issuing fid closes. */
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Module publication token. */
    StyxRpcResource bus;              /* Registry operations for the .unibus file. */
    void *bus_cookie;                 /* Open registry .unibus handle. */
    int version;                      /* This open returns the channel protocol marker. */
    uint64_t id;                      /* Exact selected node. */
    size_t staged;                    /* Contiguous accepted request bytes. */
    size_t ready;                     /* Confirmed immutable reply extent. */
    int refused;                      /* Sticky admission failure. */
    unsigned char storage[];          /* Request, reply and encoding scratch. */
} GraphFile;


/* EN/RU: See graph_module.h. / См. graph_module.h. */
StyxRpcStatus styxrpc_graph_module_init(StyxRpcGraphModule *module, const char *name,
    const StyxRpcGraphExport *exports, size_t count)
{
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size = 0;
    if (!module || !styxrpc_module_name(name) || !exports || !count || count > STYXRPC_MAX_EXPORTS)
    {
        return STYXRPC_INVALID;
    }
    for (size_t i = 0; i < count; ++i)
    {
        size_t extent;
        if (!exports[i].begin || styxrpc_graph_profile(exports[i].name, exports[i].signature, &exports[i].schema,
            profile + size, sizeof(profile) - size, &extent) != STYXRPC_OK)
        {
            return STYXRPC_INVALID;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(exports[i].name, exports[j].name))
            {
                return STYXRPC_INVALID;
            }
        }
        size += extent;
    }
    module->exports = exports;
    module->count = count;
    module->profile_size = size;
    memcpy(module->profile, profile, size);
    styxrpc_module_nodes(module->entries, name);
    for (size_t i = 0; i < count; ++i)
    {
        styxrpc_module_export_node(module->entries, i, exports[i].name);
    }
    size_t marker = FIRST_EXPORT + count;
    module->entries[marker] = (StyxRpcEntry){marker, ".ref-fields", 0444, 0};
    return STYXRPC_OK;
}


/* EN: Return the requested child, including the graph marker on the module.
 * RU: Вернуть потомка по индексу, включая маркер графа в модуле. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    const StyxRpcGraphModule *module = context;
    return styxrpc_module_child(module->entries, module->count + 1, directory, index);
}


/* EN: Describe one stable node and advertise the marker length.
 * RU: Описать стабильный узел и длину маркера графа. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    const StyxRpcGraphModule *module = context;
    if (!styxrpc_module_stat(module->entries, module->count + 1, module->profile_size, id, node))
    {
        return 0;
    }
    if (id == FIRST_EXPORT + module->count)
    {
        node->length = sizeof(STYXRPC_POSITIONAL_GRAPH) - 1;
    }
    return 1;
}


/* EN: Allocate exact file state. Read-only metadata never allocates call buffers.
 * RU: Выделить состояние файла; метаданные не используют буферы вызова. */
static int open_common(const StyxRpcGraphModule *module, const StyxRpcChannelModule *scoped,
    uint64_t id, unsigned access, void **cookie)
{
    size_t marker = FIRST_EXPORT + module->count;
    int function = id >= FIRST_EXPORT && id < marker;
    int bus = scoped && id == scoped->bus.id;
    if (id < INTERFACE || id > (scoped ? scoped->bus.id : marker) ||
        access != ((function || bus) ? STYXRPC_READ | STYXRPC_WRITE : STYXRPC_READ))
    {
        return 0;
    }
    size_t extra = function ? 3U * STYXRPC_MANAGED_LIMIT : 0;
    GraphFile *file = calloc(1, sizeof(*file) + extra);
    if (!file)
    {
        return 0;
    }
    file->module = module;
    file->id = id;
    file->function = function && module->exports ? module->exports + (id - FIRST_EXPORT) : NULL;
    file->channel = function && scoped ? scoped->exports + (id - FIRST_EXPORT) : NULL;
    file->registry = scoped ? scoped->registry : NULL;
    file->version = scoped && id == scoped->version.id;
    if (bus)
    {
        file->bus = styxrpc_module_channels_resource(file->registry);
        if (!file->bus.open(file->bus.context, access, &file->bus_cookie))
        {
            free(file);
            return 0;
        }
    }
    *cookie = file;
    return 1;
}


/* EN: Open plain graph metadata or one ordinary graph function.
 * RU: Открыть метаданные обычного графа или одну функцию графа. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    return open_common(context, NULL, id, access, cookie);
}


/* EN: Open a scoped function or one node of its leased channel module.
 * RU: Открыть контекстную функцию или узел её модуля арендованных каналов. */
static int open_channel(void *context, uint64_t id, unsigned access, void **cookie)
{
    const StyxRpcChannelModule *module = context;
    return open_common(&module->graph, module, id, access, cookie);
}


/* EN: Add the channel marker and bus after the common graph nodes.
 * RU: Добавить маркер каналов и шину после общих узлов графа. */
static const StyxRpcEntry *enumerate_channel(void *context, uint64_t directory, size_t index)
{
    StyxRpcChannelModule *module = context;
    const StyxRpcEntry *entry = enumerate(&module->graph, directory, index);
    if (entry || directory != MODULE)
    {
        return entry;
    }
    return index == module->graph.count + 2 ? &module->version :
        index == module->graph.count + 3 ? &module->bus : NULL;
}


/* EN: Describe the common nodes or one of the two public channel-specific files.
 * RU: Описать общий узел либо один из двух публичных файлов каналов. */
static int describe_channel(void *context, uint64_t id, StyxRpcNode *node)
{
    StyxRpcChannelModule *module = context;
    if (describe(&module->graph, id, node))
    {
        return 1;
    }
    if (id != module->version.id && id != module->bus.id)
    {
        return 0;
    }
    *node = (StyxRpcNode){.entry = id == module->version.id ? module->version : module->bus, .parent = MODULE,
        .length = id == module->version.id ? sizeof(STYXRPC_CHANNEL_VERSION) - 1 : 0};
    return 1;
}


/* EN: Run a checked callback once and encode both reply fields before publication.
 * RU: Однократно вызвать проверенный колбэк и подготовить обе части ответа. */
static int finish(GraphFile *file)
{
    unsigned char *reply = file->storage + STYXRPC_MANAGED_LIMIT;
    unsigned char *scratch = reply + STYXRPC_MANAGED_LIMIT;
    const StyxRpcGraphExport *function = file->function;
    FVM_SRecord arguments;
    StyxRpcGraphOutput output = {0};
    size_t result_size, update_size;
    if (styxrpc_graph_arguments(&function->schema, file->storage, file->staged, &arguments) != STYXRPC_OK)
    {
        return 0;
    }
    if (function->begin(function->context, &arguments, &output) == STYXRPC_OK)
    {
        unsigned long tag = !strcmp(function->schema.result, "()") ? STYXRPC_VALUE_NIL : STYXRPC_GRAPH_RESULT_TAG;
        if (fvm_srecord_write(tag, output.result, output.result_size, scratch, STYXRPC_MANAGED_LIMIT,
                &result_size) == FVM_S_OK &&
            fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, output.updates, output.update_size,
                scratch + result_size, STYXRPC_MANAGED_LIMIT - result_size, &update_size) == FVM_S_OK &&
            styxrpc_graph_reply(&function->schema, file->storage, file->staged, scratch, result_size,
                scratch + result_size, update_size, reply, STYXRPC_MANAGED_LIMIT, &file->ready) == STYXRPC_OK)
        {
            return 1;
        }
    }
    return styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0,
        reply, STYXRPC_MANAGED_LIMIT, &file->ready);
}


/* EN: Acknowledge a contiguous checked prefix, then deliver at most one request.
 * RU: Подтвердить последовательный префикс и доставить не более одного запроса. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    GraphFile *file = cookie;
    if (file->bus.write)
    {
        return file->bus.write(file->bus_cookie, offset, bytes, count, size);
    }
    if ((!file->function && !file->channel) || file->refused || file->ready || offset != file->staged || !count ||
        count > STYXRPC_MANAGED_LIMIT - file->staged)
    {
        return 0;
    }
    memcpy(file->storage + file->staged, bytes, count);
    file->staged += count;
    FVM_SRecord record;
    size_t used;
    FVM_SResult parsed = fvm_srecord_read(file->storage, file->staged, STYXRPC_MANAGED_LIMIT, &record, &used);
    if (parsed == FVM_S_MORE && file->staged < STYXRPC_MANAGED_LIMIT)
    {
        *size = count;
        return 1;
    }
    if (parsed != FVM_S_OK || used != file->staged ||
        !(file->channel ? styxrpc_channel_module_finish(file->channel, file->registry, file->storage,
            file->staged, file->storage + STYXRPC_MANAGED_LIMIT, STYXRPC_MANAGED_LIMIT, &file->ready,
            &file->active, file->token) : finish(file)))
    {
        file->refused = 1;
        return 0;
    }
    *size = count;
    return 1;
}


/* EN: Read the immutable reply or metadata from the exact opened file.
 * RU: Прочитать неизменяемый ответ или метаданные точного открытого файла. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    const GraphFile *file = cookie;
    if (file->bus.read)
    {
        return file->bus.read(file->bus_cookie, offset, bytes, capacity, size);
    }
    if (file->refused)
    {
        return 0;
    }
    int call = file->function || file->channel;
    const void *source = call ? (const void *)(file->storage + STYXRPC_MANAGED_LIMIT) :
        file->id == INTERFACE ? (const void *)file->module->profile :
        file->version ? STYXRPC_CHANNEL_VERSION : STYXRPC_POSITIONAL_GRAPH;
    size_t length = call ? file->ready : file->id == INTERFACE ? file->module->profile_size :
        file->version ? sizeof(STYXRPC_CHANNEL_VERSION) - 1 : sizeof(STYXRPC_POSITIONAL_GRAPH) - 1;
    size_t remaining = offset < length ? length - (size_t)offset : 0;
    *size = remaining < capacity ? remaining : capacity;
    if (*size)
    {
        memcpy(bytes, (const unsigned char *)source + (size_t)offset, *size);
    }
    return 1;
}

/* EN: Revoke a function's grant before releasing its application-owned context.
 * RU: Отозвать полномочие функции перед освобождением её контекста приложения. */
static void close_file(void *cookie)
{
    GraphFile *file = cookie;
    if (file->bus.close)
    {
        file->bus.close(file->bus_cookie);
    }
    if (file->active.key)
    {
        styxrpc_module_channels_revoke(file->registry, file->token);
    }
    styxrpc_channel_output_dispose(&file->active);
    free(file);
}

const StyxRpcTree styxrpc_graph_module_tree = {{enumerate, open_file, read_file, write_file, close_file}, describe}; /* Immutable dispatch. */
const StyxRpcTree styxrpc_channel_module_tree = {{enumerate_channel, open_channel, read_file, write_file, close_file},
    describe_channel}; /* Scoped module dispatch using the same fid lifecycle. */
