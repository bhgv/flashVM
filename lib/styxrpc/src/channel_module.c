/* Copyright (c) 2026 bhgv
 * EN: Bind a compiled scoped function to an application-owned UniBus publication.
 * RU: Привязка контекстной функции к публикации UniBus приложения.
 */
#include "channel_module_internal.h"
#include "module_nodes.h"
#include "call_record.h"
#include "load_profile.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <string.h>

#define MODULE_CHANNEL_FRAME 64U /* One exported leaf and its wrapper. */


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_input_schema(const StyxRpcSchema *schema, int channels)
{
    return memchr(schema->arguments, 0, sizeof(schema->arguments)) &&
        !strpbrk(schema->arguments, channels ? "rh" : "rhc");
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_output_schema(const StyxRpcSchema *schema)
{
    const char *end;
    return memchr(schema->result, 0, sizeof(schema->result)) &&
        schema->result[0] == '(' && schema->result[1] == 'c' &&
        (end = styxshape_end(schema->result + 1)) && !strcmp(end, ")") &&
        !strpbrk(schema->result + 2, "rh");
}


/* EN: Admit one value-only input and one channel result with value-only elements.
 * RU: Допустить вход по значению и один канал с элементами по значению. */
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


/* EN: Encode exactly one live channel or nil, followed by the empty positional changes.
 * RU: Закодировать один живой канал или nil, затем пустые позиционные изменения. */
static int write_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const StyxRpcChannelOutput *channel, const unsigned char *token,
    void *reply, size_t capacity, size_t *ready)
{
    unsigned char leaf[MODULE_CHANNEL_FRAME], result[MODULE_CHANNEL_FRAME], updates[MODULE_CHANNEL_FRAME];
    size_t length, result_size, update_size;
    int valid = channel->key ? styxrpc_channel_export_value(token, channel->key, channel->unbuffered,
        leaf, sizeof(leaf), &length) == STYXRPC_OK :
        fvm_srecord_write(STYXRPC_VALUE_NIL, NULL, 0, leaf, sizeof(leaf), &length) == FVM_S_OK;
    /* EN: A nil channel is a child of the typed result, distinct from a void reply.
     * RU: nil канала — дочернее значение результата, отличное от пустого ответа. */
    return valid && fvm_srecord_write(STYXRPC_GRAPH_RESULT_TAG, leaf, length, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(schema, call, result, result_size, updates, update_size,
            reply, capacity, ready) == STYXRPC_OK;
}


/* EN/RU: See channel_module_internal.h. / См. channel_module_internal.h. */
int styxrpc_channel_output_publish(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    StyxRpcModuleChannels *registry, const StyxRpcChannelOutput *output,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE], void *reply, size_t capacity, size_t *ready)
{
    int published = output->key && styxrpc_module_channels_publish(registry, &output->resource, token);
    if ((!output->key || published) && write_reply(schema, call, output, token, reply, capacity, ready))
    {
        return 1;
    }
    if (published)
    {
        styxrpc_module_channels_revoke(registry, token);
    }
    return 0;
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
    if (output.release)
    {
        output.release(output.resource.context);
    }
    return styxscalarwrite(STYX_FUNCTION_REPLY_TAG, STYX_SCALAR_ERROR, 0, reply, capacity, ready);
}
