/* Copyright (c) 2026 bhgv
 * EN: Stage checked channel calls on Styx and run them on the application owner.
 * RU: Приём проверенных вызовов каналов по Styx с исполнением у владельца приложения.
 */
#include "styxrpc/channel_handlers.h"
#include "styxrpc/graph.h"
#include "styxrpc/managed_module.h"
#include "call_record.h"
#include "load_profile.h"
#include "module_nodes.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PENDING 32U /* Complete queued jobs per hosted module. */
#define FRAME STYXRPC_MANAGED_LIMIT /* Capacity of each disjoint call buffer. */

typedef struct HandlerFile
{
    struct StyxRpcChannelHandlers *module;  /* Borrowed through close and completion. */
    const StyxRpcChannelFunction *function; /* NULL for metadata. */
    struct HandlerFile *next;               /* Queue link protected by module.lock. */
    uint64_t id;                           /* Exact opened node. */
    size_t staged;                         /* Request bytes accepted by the dispatcher. */
    int submitted;                         /* Sealed by the dispatcher after the last write. */
    int rejected;                          /* Sticky dispatcher-side framing failure. */
    int pending;                           /* Queued or executing, under module.lock. */
    int detached;                          /* FID closed while pending, under module.lock. */
    size_t ready;                          /* Immutable reply length, published under module.lock. */
    StyxRpcChannelCall call;                /* Verified request whose payload borrows storage. */
    unsigned char storage[];               /* Request, reply, callback output and framed output. */
} HandlerFile;

struct StyxRpcChannelHandlers
{
    pthread_mutex_t lock;                  /* Protects jobs and reply publication. */
    pthread_cond_t wake;                    /* Signals queue or stop to the single owner. */
    HandlerFile *first;                    /* Oldest pending FID. */
    HandlerFile *last;                     /* Newest pending FID. */
    HandlerFile *active;                   /* FID executing on the sole owner, if any. */
    size_t queued;                         /* Number of complete, not yet started jobs. */
    int stopped;                           /* Admission closed. */
    const StyxRpcChannelFunction *functions; /* Immutable borrowed descriptors. */
    size_t count;                          /* Number of exported functions. */
    StyxRpcEntry entries[FIRST_EXPORT + STYXRPC_MAX_EXPORTS + 2]; /* Module nodes and markers. */
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES]; /* Serialized compiled offers. */
    size_t profile_size;                   /* Bytes in the immutable interface. */
};


/* EN: Append the compiled offer for function to module when its schema is supported.
 * RU: Добавить компилированное предложение function в module при поддерживаемой схеме. */
static int add_offer(StyxRpcChannelHandlers *module, const StyxRpcChannelFunction *function)
{
    const StyxRpcSchema *schema = &function->schema;
    size_t used;
    if (!function->handler || !memchr(schema->arguments, 0, sizeof(schema->arguments)) ||
        !memchr(schema->result, 0, sizeof(schema->result)) || !strchr(schema->arguments, 'c') ||
        strpbrk(schema->arguments, "rh") || strpbrk(schema->result, "rhc") ||
        styxrpc_channel_profile(function->name, function->signature, schema, module->profile + module->profile_size,
            sizeof(module->profile) - module->profile_size, &used) != STYXRPC_OK)
    {
        return 0;
    }
    const char *type = strchr(schema->arguments, 'c');
    while (type)
    {
        const char *end = styxshape_end(type);
        if (!end || memchr(type + 2, 'c', (size_t)(end - type - 3)))
        {
            return 0;
        }
        type = strchr(end, 'c');
    }
    module->profile_size += used;
    return 1;
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
StyxRpcChannelHandlers *styxrpc_channel_handlers_create(const char *name,
    const StyxRpcChannelFunction *functions, size_t count)
{
    if (!styxrpc_module_name(name) || !functions || !count || count > STYXRPC_MAX_EXPORTS)
    {
        return NULL;
    }
    StyxRpcChannelHandlers *module = calloc(1, sizeof(*module));
    if (!module)
    {
        return NULL;
    }
    styxrpc_module_nodes(module->entries, name);
    for (size_t i = 0; i < count; ++i)
    {
        if (!add_offer(module, functions + i))
        {
            free(module);
            return NULL;
        }
        for (size_t j = 0; j < i; ++j)
        {
            if (!strcmp(functions[i].name, functions[j].name))
            {
                free(module);
                return NULL;
            }
        }
        styxrpc_module_export_node(module->entries, i, functions[i].name);
    }
    size_t marker = FIRST_EXPORT + count;
    module->entries[marker] = (StyxRpcEntry){marker, STYX_MANAGED_DELTA_FILE, 0444, 0};
    module->entries[marker + 1] = (StyxRpcEntry){marker + 1, STYX_CHANNEL_PROFILE_FILE, 0444, 0};
    if (pthread_mutex_init(&module->lock, NULL))
    {
        free(module);
        return NULL;
    }
    if (pthread_cond_init(&module->wake, NULL))
    {
        pthread_mutex_destroy(&module->lock);
        free(module);
        return NULL;
    }
    module->functions = functions;
    module->count = count;
    return module;
}


/* EN: Transfer the oldest file to the owner or stop path with module.lock held.
 * RU: Передать самый старый файл владельцу или остановке под module.lock. */
static HandlerFile *dequeue(StyxRpcChannelHandlers *module)
{
    HandlerFile *file = module->first;
    module->first = file->next;
    if (!module->first)
    {
        module->last = NULL;
    }
    --module->queued;
    return file;
}


/* EN: Complete a pending file with module.lock held. The dispatcher may have detached it.
 * RU: Завершить ожидание файла под module.lock. Диспетчер мог закрыть файл. */
static void complete(HandlerFile *file, size_t ready)
{
    file->pending = 0;
    file->ready = ready;
    if (file->detached)
    {
        free(file);
    }
}


/* EN: Run the checked callback once and return the immutable validated reply length.
 * RU: Однократно выполнить проверенный колбэк и вернуть длину проверенного ответа. */
static size_t run_call(HandlerFile *file)
{
    unsigned char *reply = file->storage + FRAME;
    unsigned char *payload = reply + FRAME;
    unsigned char *result = payload + FRAME;
    unsigned char updates[32];
    const StyxRpcChannelFunction *function = file->function;
    size_t payload_size = 0, result_size, updates_size, ready = 0;
    int is_void = !strcmp(function->schema.result, "()");
    if (function->handler(function->context, &file->call, payload, FRAME, &payload_size) == STYXRPC_OK &&
        payload_size <= FRAME && (!is_void || !payload_size) &&
        fvm_srecord_write(is_void ? STYXRPC_VALUE_NIL : STYXRPC_GRAPH_RESULT_TAG, payload, payload_size,
            result, FRAME, &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates,
            sizeof(updates), &updates_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(&function->schema, &file->call, result, result_size,
            updates, updates_size, reply, FRAME, &ready) == STYXRPC_OK)
    {
        return ready;
    }
    styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, FRAME, &ready);
    return ready;
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
int styxrpc_channel_handlers_step(StyxRpcChannelHandlers *module)
{
    if (!module)
    {
        return 0;
    }
    pthread_mutex_lock(&module->lock);
    while (!module->first && !module->stopped)
    {
        pthread_cond_wait(&module->wake, &module->lock);
    }
    if (module->stopped)
    {
        pthread_mutex_unlock(&module->lock);
        return 0;
    }
    HandlerFile *file = dequeue(module);
    module->active = file;
    pthread_mutex_unlock(&module->lock);

    size_t ready = run_call(file);
    pthread_mutex_lock(&module->lock);
    module->active = NULL;
    complete(file, ready);
    pthread_mutex_unlock(&module->lock);
    return 1;
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
void styxrpc_channel_handlers_stop(StyxRpcChannelHandlers *module)
{
    if (!module)
    {
        return;
    }
    pthread_mutex_lock(&module->lock);
    module->stopped = 1;
    while (module->first)
    {
        complete(dequeue(module), 0);
    }
    pthread_cond_signal(&module->wake);
    pthread_mutex_unlock(&module->lock);
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
void styxrpc_channel_handlers_destroy(StyxRpcChannelHandlers *module)
{
    if (module)
    {
        styxrpc_channel_handlers_stop(module);
        pthread_cond_destroy(&module->wake);
        pthread_mutex_destroy(&module->lock);
        free(module);
    }
}


/* EN: Look up the child's stable entry by directory ID and index.
 * RU: Найти стабильную запись потомка по ID каталога и индексу. */
static const StyxRpcEntry *enumerate(void *context, uint64_t directory, size_t index)
{
    const StyxRpcChannelHandlers *module = context;
    return styxrpc_module_child(module->entries, module->count + 2, directory, index);
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
    if (!styxrpc_module_stat(module->entries, module->count + 2, module->profile_size, id, node))
    {
        return 0;
    }
    if (id >= FIRST_EXPORT + module->count)
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
    if (id < INTERFACE || id > marker + 1 ||
        access != (function ? STYXRPC_READ | STYXRPC_WRITE : STYXRPC_READ))
    {
        return 0;
    }
    HandlerFile *file = calloc(1, sizeof(*file) + (function ? 4U * FRAME : 0));
    if (!file)
    {
        return 0;
    }
    file->module = module;
    file->id = id;
    file->function = function ? module->functions + (id - FIRST_EXPORT) : NULL;
    *cookie = file;
    return 1;
}


/* EN: Queue file once unless module is stopped or the pending bound was reached.
 * RU: Поставить file в очередь один раз до остановки module либо переполнения. */
static int submit(HandlerFile *file)
{
    StyxRpcChannelHandlers *module = file->module;
    pthread_mutex_lock(&module->lock);
    int accepted = !module->stopped && module->queued < MAX_PENDING;
    if (accepted)
    {
        if (module->last)
        {
            module->last->next = file;
        }
        else
        {
            module->first = file;
        }
        module->last = file;
        ++module->queued;
        file->pending = 1;
        pthread_cond_signal(&module->wake);
    }
    pthread_mutex_unlock(&module->lock);
    return accepted;
}


/* EN: Accept a contiguous frame, then acknowledge the validated job without running it.
 * RU: Принять последовательный кадр, затем подтвердить проверенную работу без её исполнения. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    HandlerFile *file = cookie;
    if (!file->function || file->submitted || file->rejected || offset != file->staged ||
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
        styxrpc_channel_call(&file->function->schema, file->storage, file->staged, &file->call) != STYXRPC_OK ||
        !submit(file))
    {
        file->rejected = 1;
        return 0;
    }
    file->submitted = 1;
    *size = count;
    return 1;
}


/* EN: Copy an immutable reply range or metadata; an unfinished call reads as empty.
 * RU: Скопировать диапазон готового ответа или метаданных; ожидающий вызов читается пустым. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    HandlerFile *file = cookie;
    StyxRpcChannelHandlers *module = file->module;
    size_t length;
    pthread_mutex_lock(&module->lock);
    if (file->rejected || (file->submitted && !file->pending && !file->ready))
    {
        pthread_mutex_unlock(&module->lock);
        return 0;
    }
    const void *source = file->function ? (const void *)(file->storage + FRAME) :
        metadata(module, file->id, &length);
    if (file->function)
    {
        length = file->ready;
    }
    size_t remaining = offset < length ? length - (size_t)offset : 0;
    *size = remaining < capacity ? remaining : capacity;
    if (*size)
    {
        memcpy(bytes, (const unsigned char *)source + (size_t)offset, *size);
    }
    pthread_mutex_unlock(&module->lock);
    return 1;
}


/* EN: Remove the queued file immediately. Caller holds module.lock.
 * RU: Сразу исключить ожидающий файл из очереди. Вызывающий удерживает module.lock. */
static void remove_queued(StyxRpcChannelHandlers *module, HandlerFile *file)
{
    HandlerFile *previous = NULL;
    for (HandlerFile *entry = module->first; entry != file; entry = entry->next)
    {
        previous = entry;
    }
    if (previous)
    {
        previous->next = file->next;
    }
    else
    {
        module->first = file->next;
    }
    if (module->last == file)
    {
        module->last = previous;
    }
    --module->queued;
}


/* EN: Retire this FID without waiting on a callback that may issue reverse Styx requests.
 * RU: Закрыть этот fid без ожидания колбэка, который может выполнять встречные запросы Styx. */
static void close_file(void *cookie)
{
    HandlerFile *file = cookie;
    StyxRpcChannelHandlers *module = file->module;
    pthread_mutex_lock(&module->lock);
    if (module->active == file)
    {
        file->detached = 1;
    }
    else
    {
        if (file->pending)
        {
            remove_queued(module, file);
        }
        free(file);
    }
    pthread_mutex_unlock(&module->lock);
}

const StyxRpcTree styxrpc_channel_handlers_tree = {{enumerate, open_file, read_file, write_file, close_file},
    describe}; /* Immutable Styx tree operations for one connection dispatcher. */
