/* Copyright (c) 2026 bhgv
 * EN: Bind a compiled scoped function to an application-owned UniBus publication.
 * RU: Привязка контекстной функции к публикации UniBus приложения.
 */
#include "channel_module_internal.h"
#include "channel_remap.h"
#include "module_nodes.h"
#include "call_record.h"
#include "load_profile.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>

#define MODULE_CHANNEL_FRAME 64U /* One exported leaf and its wrapper. */


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_input_schema(const StyxRpcSchema *schema, int channels)
{
    return memchr(schema->arguments, 0, sizeof(schema->arguments)) &&
        (channels || !strchr(schema->arguments, 'c'));
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_output_schema(const StyxRpcSchema *schema)
{
    return memchr(schema->result, 0, sizeof(schema->result)) &&
        strchr(schema->result, 'c') != NULL;
}


/* EN: Admit a scoped export with a lease in its result and no borrowed channels.
 * RU: Допустить контекстный экспорт с арендой в результате без заимствованных каналов. */
static int offer(const StyxRpcChannelExport *exported, unsigned char *bytes, size_t capacity, size_t *used)
{
    const StyxRpcSchema *schema = &exported->schema;
    return exported->begin && styxrpc_channel_input_schema(schema, 0) &&
        styxrpc_channel_output_schema(schema) &&
        styxrpc_channel_profile(exported->name, exported->signature, schema, bytes, capacity, used) == STYXRPC_OK;
}


/* EN/RU: See styxrpc/channel_module.h. / См. styxrpc/channel_module.h. */
StyxRpcStatus styxrpc_channel_module_init(StyxRpcChannelModule *module, const char *name,
    const StyxRpcChannelExport *exports, size_t count, StyxRpcModuleChannels *registry)
{
    unsigned char profile[STYXRPC_MANAGED_PROFILE_BYTES];
    size_t size = 0;
    if (!module || !styxrpc_module_name(name) || !exports || !count || count > STYXRPC_MAX_EXPORTS ||
        !registry || !registry->slots || !registry->capacity || !registry->entropy)
    {
        return STYXRPC_INVALID;
    }
    for (size_t i = 0; i < count; ++i)
    {
        size_t extent;
        if (!offer(exports + i, profile + size, sizeof(profile) - size, &extent))
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
    module->graph.exports = NULL;
    module->exports = exports;
    module->registry = registry;
    module->graph.count = count;
    module->graph.profile_size = size;
    memcpy(module->graph.profile, profile, size);
    styxrpc_module_nodes(module->graph.entries, name);
    for (size_t i = 0; i < count; ++i)
    {
        styxrpc_module_export_node(module->graph.entries, i, exports[i].name);
    }
    size_t marker = FIRST_EXPORT + count;
    module->graph.entries[marker] = (StyxRpcEntry){marker, STYX_MANAGED_DELTA_FILE, 0444, 0};
    module->version = (StyxRpcEntry){marker + 1, STYX_CHANNEL_PROFILE_FILE, 0444, 0};
    module->bus = (StyxRpcEntry){marker + 2, ".unibus", 0666, 0};
    return STYXRPC_OK;
}


typedef struct ResultKeys
{
    uint32_t first; /* Expected first live key of the staged result. */
    int seen;       /* A live result key has already been stamped. */
} ResultKeys;


/* EN: Require the resource key to be the first channel visible in the result.
 * RU: Потребовать, чтобы первый видимый канал результата имел ключ ресурса. */
static int result_key(const void *state, uint32_t *key, int *unbuffered)
{
    ResultKeys *keys = (ResultKeys *)state;
    (void)unbuffered;
    if (!keys->seen && *key != keys->first)
    {
        return 0;
    }
    keys->seen = 1;
    return 1;
}


/* EN: Stamp every staged current key with the exact token, or write the legacy leaf.
 * RU: Проставить точный токен у подготовленных ключей либо записать прежний лист. */
static int result_child(const StyxRpcChannelOutput *channel, const unsigned char *token,
    void *bytes, size_t capacity, size_t *used)
{
    if (channel->result)
    {
        ResultKeys keys = {channel->key, 0};
        const ChannelKeyTransform mapping = {.apply = result_key, .context = &keys,
            .token = token, .result = 1};
        return styxrpc_channel_reframe(&mapping, channel->result, channel->result_size, bytes, capacity, used) &&
            keys.seen == (channel->key != 0);
    }
    return channel->key ? styxrpc_channel_export_value(token, channel->key, channel->unbuffered,
        bytes, capacity, used) == STYXRPC_OK :
        fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, bytes, capacity, used) == FVM_S_OK;
}


/* EN: Pack the checked result and positional updates before exposing any reply bytes.
 * RU: Упаковать проверенный результат и позиционные изменения до выдачи ответа. */
static int write_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const StyxRpcChannelOutput *channel, const unsigned char *token,
    void *reply, size_t capacity, size_t *ready)
{
    unsigned char simple[2U * MODULE_CHANNEL_FRAME], empty[MODULE_CHANNEL_FRAME];
    size_t length, result_size, update_size;
    if (!capacity || capacity > STYXRPC_MANAGED_LIMIT || channel->update_size > capacity ||
        (channel->update_size && !channel->updates) || channel->result_size > capacity ||
        (!!channel->result_size != (channel->result != NULL)))
    {
        return 0;
    }
    size_t extent = channel->result ? capacity : MODULE_CHANNEL_FRAME;
    unsigned char *leaf = channel->result ? malloc(2U * extent) : simple;
    unsigned char *updates = channel->update_size ? malloc(capacity) : empty;
    int valid = leaf && updates && result_child(channel, token, leaf, extent, &length) &&
        fvm_srecord_write(STYXRPC_GRAPH_RESULT_TAG, leaf, length, leaf + extent, extent, &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, channel->updates, channel->update_size, updates,
            channel->update_size ? capacity : sizeof(empty), &update_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(schema, call, leaf + extent, result_size, updates, update_size,
            reply, capacity, ready) == STYXRPC_OK;
    if (updates != empty)
    {
        free(updates);
    }
    if (leaf != simple)
    {
        free(leaf);
    }
    return valid;
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_output_publish(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    StyxRpcModuleChannels *registry, StyxRpcChannelOutput *output,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE], void *reply, size_t capacity, size_t *ready)
{
    int published = output->key && styxrpc_module_channels_publish(registry, &output->resource, token);
    int valid = (!output->key || published) && write_reply(schema, call, output, token, reply, capacity, ready);
    free(output->updates);
    output->updates = NULL;
    output->update_size = 0;
    free(output->result);
    output->result = NULL;
    output->result_size = 0;
    if (!valid && published)
    {
        styxrpc_module_channels_revoke(registry, token);
    }
    return valid;
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
void styxrpc_channel_output_dispose(StyxRpcChannelOutput *output)
{
    free(output->updates);
    free(output->result);
    if (output->release)
    {
        output->release(output->resource.context);
    }
    *output = (StyxRpcChannelOutput){0};
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_module_finish(const StyxRpcChannelExport *function, StyxRpcModuleChannels *registry,
    const void *request, size_t size, void *reply, size_t capacity, size_t *ready,
    StyxRpcChannelOutput *active, unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE])
{
    StyxRpcChannelCall call;
    StyxRpcChannelOutput output = {0};
    if (styxrpc_channel_call(&function->schema, request, size, &call) != STYXRPC_OK ||
        call.arguments.tag != STYXRPC_GRAPH_REQUEST_TAG)
    {
        return 0;
    }
    if (function->begin(function->context, &call, &output) == STYXRPC_OK &&
        styxrpc_channel_output_publish(&function->schema, &call, registry, &output, token, reply, capacity, ready))
    {
        *active = output;
        return 1;
    }
    styxrpc_channel_output_dispose(&output);
    return styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, capacity, ready);
}
