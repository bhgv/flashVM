/* Copyright (c) 2026 bhgv
 * EN: Stage checked channel calls on Styx and run them on the application owner.
 * RU: Приём проверенных вызовов каналов по Styx с исполнением у владельца приложения.
 */
#include "channel_handlers_internal.h"
#include "channel_module_internal.h"
#include "styxrpc/graph.h"
#include "call_record.h"
#include "load_profile.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>

#define MAX_PENDING 32U /* Complete queued jobs per hosted module. */


/* EN: Validate one offer and add its stable function file, rejecting duplicate names.
 * RU: Проверить предложение и добавить устойчивый файл функции, отклоняя повторы. */
static int add_offer(StyxRpcChannelHandlers *module, size_t index, const char *name,
    uint32_t signature, const StyxRpcSchema *schema)
{
    size_t used;
    if (styxrpc_channel_profile(name, signature, schema, module->profile + module->profile_size,
        sizeof(module->profile) - module->profile_size, &used) != STYXRPC_OK ||
        !styxrpc_channel_input_schema(schema, 1))
    {
        return 0;
    }
    for (size_t i = 0; i < index; ++i)
    {
        if (!strcmp(module->entries[FIRST_EXPORT + i].name, name))
        {
            return 0;
        }
    }
    module->profile_size += used;
    styxrpc_module_export_node(module->entries, index, name);
    return 1;
}


/* EN: Populate all compiled offers and metadata before creating synchronization state.
 * RU: Заполнить предложения и метаданные до создания состояния синхронизации. */
static int populate(StyxRpcChannelHandlers *module, const char *name,
    const StyxRpcChannelFunction *functions, size_t count, const StyxRpcChannelExport *results, size_t result_count)
{
    styxrpc_module_nodes(module->entries, name);
    for (size_t i = 0; i < count; ++i)
    {
        const StyxRpcChannelFunction *f = functions + i;
        if (!f->handler || !add_offer(module, i, f->name, f->signature, &f->schema) ||
            !strchr(f->schema.arguments, 'c') || strpbrk(f->schema.result, "rhc"))
        {
            return 0;
        }
    }
    for (size_t i = 0; i < result_count; ++i)
    {
        const StyxRpcChannelExport *f = results + i;
        if (!f->begin || !styxrpc_channel_output_schema(&f->schema) ||
            !add_offer(module, count + i, f->name, f->signature, &f->schema))
        {
            return 0;
        }
    }
    size_t marker = FIRST_EXPORT + count + result_count;
    module->entries[marker] = (StyxRpcEntry){marker, STYX_MANAGED_DELTA_FILE, 0444, 0};
    module->entries[marker + 1] = (StyxRpcEntry){marker + 1, STYX_CHANNEL_PROFILE_FILE, 0444, 0};
    if (result_count)
    {
        module->entries[marker + 2] = (StyxRpcEntry){marker + 2, ".unibus", 0666, 0};
    }
    return 1;
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
StyxRpcChannelHandlers *styxrpc_channel_handlers_create_with_results(const char *name,
    const StyxRpcChannelFunction *functions, size_t count, const StyxRpcChannelExport *results,
    size_t result_count, StyxRpcModuleChannels *registry)
{
    if (!styxrpc_module_name(name) || count > STYXRPC_MAX_EXPORTS || result_count > STYXRPC_MAX_EXPORTS - count ||
        (!count && !result_count) || (count && !functions) ||
        (result_count && (!results || !registry || !registry->slots || !registry->capacity || !registry->entropy)))
    {
        return NULL;
    }
    StyxRpcChannelHandlers *module = calloc(1, sizeof(*module));
    if (!module)
    {
        return NULL;
    }
    if (!populate(module, name, functions, count, results, result_count) ||
        pthread_mutex_init(&module->lock, NULL))
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
    module->results = results;
    module->copied_count = count;
    module->count = count + result_count;
    module->registry = result_count ? registry : NULL;
    return module;
}


/* EN/RU: See styxrpc/channel_handlers.h. / См. styxrpc/channel_handlers.h. */
StyxRpcChannelHandlers *styxrpc_channel_handlers_create(const char *name,
    const StyxRpcChannelFunction *functions, size_t count)
{
    return styxrpc_channel_handlers_create_with_results(name, functions, count, NULL, 0, NULL);
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


/* EN: Run the checked callback once and return the immutable validated reply length.
 * RU: Однократно выполнить проверенный колбэк и вернуть длину проверенного ответа. */
static size_t run_copied_call(HandlerFile *file)
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


/* EN: Run one owner callback. The dispatcher will publish any staged channel.
 * RU: Выполнить колбэк владельца. Подготовленный канал опубликует диспетчер. */
static void run_call(HandlerFile *file)
{
    if (file->function)
    {
        file->ready = run_copied_call(file);
    }
    else if (file->channel->begin(file->channel->context, &file->call, &file->output) == STYXRPC_OK)
    {
        file->staged_output = 1;
    }
    else
    {
        styxrpc_handlers_release_output(file);
        styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0,
            file->storage + FRAME, FRAME, &file->ready);
    }
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

    run_call(file);
    pthread_mutex_lock(&module->lock);
    module->active = NULL;
    file->pending = 0;
    int detached = file->detached;
    pthread_mutex_unlock(&module->lock);
    if (detached)
    {
        styxrpc_handlers_dispose(file);
    }
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
        dequeue(module)->pending = 0;
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


/* EN/RU: See channel_handlers_internal.h. / См. channel_handlers_internal.h. */
int styxrpc_handlers_submit(HandlerFile *file)
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


/* EN/RU: See channel_handlers_internal.h. / См. channel_handlers_internal.h. */
int styxrpc_handlers_detach(HandlerFile *file)
{
    StyxRpcChannelHandlers *module = file->module;
    pthread_mutex_lock(&module->lock);
    int owned = module->active != file;
    if (!owned)
    {
        file->detached = 1;
    }
    else if (file->pending)
    {
        remove_queued(module, file);
    }
    pthread_mutex_unlock(&module->lock);
    return owned;
}


/* EN/RU: See channel_handlers_internal.h. / См. channel_handlers_internal.h. */
void styxrpc_handlers_release_output(HandlerFile *file)
{
    if (file->output.release)
    {
        file->output.release(file->output.resource.context);
        file->output.release = NULL;
    }
}


/* EN/RU: See channel_handlers_internal.h. / См. channel_handlers_internal.h. */
void styxrpc_handlers_dispose(HandlerFile *file)
{
    styxrpc_handlers_release_output(file);
    free(file);
}
