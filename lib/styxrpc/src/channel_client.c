/* Copyright (c) 2026 bhgv
 * EN: Verify the scoped admission and hold the module and function for live channels.
 * RU: Проверка контекстного допуска и удержание модуля и функции для живых каналов.
 */
#include "styxrpc/channel_client.h"
#include "styxrpc/graph.h"
#include "channel_arguments_internal.h"
#include "channel_route_internal.h"
#include "client_exchange.h"
#include "call_record.h"
#include "load_profile.h"
#include "scalar_record.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>

struct StyxRpcChannelLease
{
    StyxFid *module;             /* Unopened pinned module, used to walk the bus. */
    StyxFid *function;           /* Opened call whose clunk revokes its remote grant. */
    StyxRpcChannelRoutes routes; /* Copied and validated capabilities. */
    size_t references;           /* References serialized by the application owner. */
};


/* EN: Validate the reply against the checked request and collect its distinct grants.
 * RU: Проверить ответ по принятому запросу и собрать отдельные полномочия результата. */
static StyxRpcStatus read_reply(const StyxRpcSchema *schema, const StyxRpcChannelCall *call,
    const void *bytes, size_t size, StyxRpcLeasedReply *reply)
{
    StyxScalarKind kind;
    int64_t code;
    if (styxscalarread(STYX_FUNCTION_REPLY_TAG, bytes, size, &kind, &code) && kind == STYX_SCALAR_ERROR)
    {
        return STYXRPC_REMOTE_ERROR;
    }
    StyxCallShape shape;
    memcpy(shape.arguments, schema->arguments, sizeof(shape.arguments));
    memcpy(shape.result, schema->result, sizeof(shape.result));
    StyxReplyField fields[2];
    size_t used;
    if (!styxcall_reply_read(STYX_MANAGED_REPLY_TAG, bytes, size, fields) ||
        !styxmanaged_graph_reply(&shape, &call->arguments, fields) ||
        fvm_srecord_read(fields[0].bytes, fields[0].size, STYXRPC_MANAGED_LIMIT, &reply->result, &used) != FVM_S_OK)
    {
        return STYXRPC_INVALID;
    }
    if (strchr(shape.result, 'c') && styxrpc_channel_export_routes(shape.result, fields[0].bytes,
        fields[0].size, &reply->routes) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    return STYXRPC_OK;
}


/* EN: Send only after the matching module and optional reverse publication are available.
 * RU: Отправить после проверки модуля и готовности дополнительной встречной публикации. */
static StyxRpcOutcome invoke(StyxFid *directory, StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result,
    StyxRpcCallStop stop, void *state)
{
    if (!target || !target->pid || !target->wait_ms || target->wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS ||
        (!arguments && size) || size > STYXRPC_MANAGED_LIMIT || !reply || !capacity ||
        capacity > STYXRPC_MANAGED_LIMIT || !result ||
        !memchr(target->schema.arguments, 0, sizeof(target->schema.arguments)) ||
        !memchr(target->schema.result, 0, sizeof(target->schema.result)) ||
        strpbrk(target->schema.arguments, "rh") || strpbrk(target->schema.result, "rh"))
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    if (stop && stop(state))
    {
        return STYXRPC_CALL_CANCELLED;
    }
    unsigned char pid[STYXRPC_FRAME_SIZE];
    unsigned char *request = malloc(STYXRPC_MANAGED_LIMIT);
    StyxRpcChannelLease *lease = calloc(1, sizeof(*lease));
    StyxRpcChannelCall call;
    size_t prefix, request_size, received;
    StyxRpcLeasedReply decoded = {0};
    StyxRpcOutcome outcome = STYXRPC_CALL_NOT_SENT;
    if (request && lease &&
        styxrpc_value_integer(STYXRPC_VALUE_BIG, target->pid, pid, sizeof(pid), &prefix) == STYXRPC_OK &&
        fvm_srecord_write_pair(STYXRPC_GRAPH_REQUEST_TAG, pid, prefix, arguments, size,
            request, STYXRPC_MANAGED_LIMIT, &request_size) == FVM_S_OK &&
        styxrpc_channel_call(&target->schema, request, request_size, &call) == STYXRPC_OK &&
        (lease->function = styxrpc_client_channel_function(directory, target, &lease->module)) &&
        (!scope || styxrpc_argument_scope_match(scope, &call)))
    {
        outcome = styxrpc_client_exchange(lease->function, request, request_size,
            target->wait_ms, reply, capacity, &received, stop, state);
        if (outcome == STYXRPC_CALL_OK)
        {
            StyxRpcStatus status = read_reply(&target->schema, &call, reply, received, &decoded);
            outcome = status == STYXRPC_OK ? STYXRPC_CALL_OK :
                status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
        }
        if (outcome == STYXRPC_CALL_OK)
        {
            if (decoded.routes.count)
            {
                lease->references = 1;
                lease->routes = decoded.routes;
                decoded.lease = lease;
                lease = NULL;
            }
            *result = decoded;
        }
    }
    if (lease)
    {
        styxclientclunk(lease->function);
        styxclientclunk(lease->module);
        free(lease);
    }
    free(request);
    return outcome;
}


/* EN/RU: See styxrpc/channel_client.h. / См. styxrpc/channel_client.h. */
StyxRpcOutcome styxrpc_channel_invoke_at(StyxFid *directory, const StyxRpcManagedTarget *target, const void *arguments,
    size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result)
{
    return invoke(directory, NULL, target, arguments, size, reply, capacity, result, NULL, NULL);
}


/* EN/RU: See styxrpc/channel_client.h. / См. styxrpc/channel_client.h. */
StyxRpcOutcome styxrpc_channel_invoke(const StyxRpcManagedTarget *target, const void *arguments,
    size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result)
{
    return styxrpc_channel_invoke_at(NULL, target, arguments, size, reply, capacity, result);
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcOutcome styxrpc_argument_scope_invoke_at(StyxFid *directory, StyxRpcArgumentScope *scope,
    const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result)
{
    return scope ? styxrpc_argument_scope_invoke_at_until(directory, scope, target, arguments, size,
        reply, capacity, result, NULL, NULL) : STYXRPC_CALL_NOT_SENT;
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcOutcome styxrpc_argument_scope_invoke_at_until(StyxFid *directory, StyxRpcArgumentScope *scope,
    const StyxRpcManagedTarget *target, const void *arguments, size_t size, void *reply, size_t capacity,
    StyxRpcLeasedReply *result, StyxRpcCallStop stop, void *state)
{
    StyxRpcOutcome outcome = invoke(directory, scope, target, arguments, size, reply, capacity, result, stop, state);
    styxrpc_argument_scope_close(scope);
    return outcome;
}


/* EN/RU: See styxrpc/channel_arguments.h. / См. styxrpc/channel_arguments.h. */
StyxRpcOutcome styxrpc_argument_scope_invoke(StyxRpcArgumentScope *scope, const StyxRpcManagedTarget *target,
    const void *arguments, size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result)
{
    return styxrpc_argument_scope_invoke_at(NULL, scope, target, arguments, size, reply, capacity, result);
}


/* EN/RU: See styxrpc/channel_client.h. / См. styxrpc/channel_client.h. */
int styxrpc_channel_lease_retain(StyxRpcChannelLease *lease)
{
    if (!lease || lease->references == SIZE_MAX)
    {
        return 0;
    }
    ++lease->references;
    return 1;
}


/* EN/RU: See styxrpc/channel_client.h. / См. styxrpc/channel_client.h. */
void styxrpc_channel_lease_release(StyxRpcChannelLease *lease, int deferred)
{
    if (!lease || --lease->references)
    {
        return;
    }
    if (deferred)
    {
        styxclientclunklater(lease->function);
        styxclientclunklater(lease->module);
    }
    else
    {
        styxclientclunk(lease->function);
        styxclientclunk(lease->module);
    }
    free(lease);
}


/* EN/RU: See styxrpc/channel_client.h. / См. styxrpc/channel_client.h. */
StyxRpcStatus styxrpc_channel_lease_exchange(StyxRpcChannelLease *lease, size_t route_index,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result)
{
    if (!lease || route_index >= lease->routes.count)
    {
        return STYXRPC_INVALID;
    }
    const unsigned char *token = lease->routes.entries[route_index].token;
    StyxRpcChannelGrant grants[STYXRPC_CHANNEL_SLOTS];
    size_t count = 0;
    for (size_t i = 0; i < lease->routes.count; ++i)
    {
        const StyxRpcChannelRoute *route = lease->routes.entries + i;
        if (!memcmp(token, route->token, STYXRPC_CHANNEL_TOKEN_SIZE))
        {
            grants[count++] = route->channel;
        }
    }
    return styxrpc_channel_exchange_bound(lease->module, token, grants, count, selection, timeout_ms,
        scratch, scratch_size, request, request_capacity, reply, reply_capacity, result);
}


/* EN/RU: See channel_client.h. / См. channel_client.h. */
const StyxRpcChannelRoute *styxrpc_channel_lease_route(const StyxRpcChannelLease *lease, size_t index)
{
    return lease && index < lease->routes.count ? lease->routes.entries + index : NULL;
}


/* EN: Merge a checked module route, retaining stable indices and rejecting type conflicts.
 * RU: Добавить проверенный маршрут модуля со стабильным индексом и проверкой типа. */
static int admit_route(StyxRpcChannelRoutes *table, const StyxRpcChannelRoute *route, size_t *index)
{
    for (size_t i = 0; i < table->count; ++i)
    {
        const StyxRpcChannelRoute *present = &table->entries[i];
        if (present->channel.key == route->channel.key &&
            !memcmp(present->token, route->token, STYXRPC_CHANNEL_TOKEN_SIZE))
        {
            if (present->unbuffered != route->unbuffered || strcmp(present->channel.shape, route->channel.shape))
            {
                return 0;
            }
            *index = i;
            return 1;
        }
    }
    if (table->count == STYXRPC_CHANNEL_SLOTS)
    {
        return 0;
    }
    *index = table->count++;
    table->entries[*index] = *route;
    return 1;
}


/* EN/RU: See channel_client.h. / См. channel_client.h. */
StyxRpcStatus styxrpc_channel_lease_admit_module(StyxRpcChannelLease *lease,
    const StyxRpcChannelRoute *route, size_t *index)
{
    return lease && index && route && route->kind == STYXRPC_ROUTE_MODULE &&
        styxrpc_route_valid(route) && admit_route(&lease->routes, route, index) ?
        STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See channel_client.h. / См. channel_client.h. */
StyxRpcStatus styxrpc_channel_lease_import_element(StyxRpcChannelLease *lease, size_t route_index,
    const void *bytes, size_t size)
{
    const StyxRpcChannelRoute *parent = styxrpc_channel_lease_route(lease, route_index);
    StyxRpcChannelRoutes received;
    if (!parent || !strchr(parent->channel.shape, 'c') ||
        styxrpc_channel_element_routes(parent->channel.shape, bytes, size, &received) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcChannelRoutes staged = lease->routes;
    for (size_t i = 0; i < received.count; ++i)
    {
        StyxRpcChannelRoute route = received.entries[i];
        size_t index;
        if (route.kind != STYXRPC_ROUTE_CURRENT)
        {
            return STYXRPC_INVALID;
        }
        route.kind = STYXRPC_ROUTE_MODULE;
        memcpy(route.token, parent->token, sizeof(route.token));
        if (!admit_route(&staged, &route, &index))
        {
            return STYXRPC_INVALID;
        }
    }
    lease->routes = staged;
    return STYXRPC_OK;
}


/* EN: Check whether this token was already authenticated for the same message.
 * RU: Проверить, был ли этот токен уже подтверждён для данного сообщения. */
static int seen_token(const StyxRpcChannelRoutes *received, size_t index)
{
    for (size_t i = 0; i < index; ++i)
    {
        if (!memcmp(received->entries[i].token, received->entries[index].token, STYXRPC_CHANNEL_TOKEN_SIZE))
        {
            return 1;
        }
    }
    return 0;
}


/* EN/RU: See channel_client.h. / См. channel_client.h. */
StyxRpcStatus styxrpc_channel_lease_import_message(StyxRpcChannelLease *lease,
    const char *shape, const void *bytes, size_t size)
{
    StyxRpcChannelRoutes received;
    const char *end = shape && *shape == '(' ? styxshape_end(shape + 1) : NULL;
    if (!lease || !end || *end != ')' || end[1] || !strchr(shape, 'c') || strpbrk(shape, "rh") ||
        styxrpc_channel_element_routes(shape, bytes, size, &received) != STYXRPC_OK)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcChannelRoutes staged = lease->routes;
    for (size_t i = 0; i < received.count; ++i)
    {
        const StyxRpcChannelRoute *route = received.entries + i;
        size_t index;
        if (route->kind != STYXRPC_ROUTE_MODULE || !admit_route(&staged, route, &index))
        {
            return STYXRPC_INVALID;
        }
    }
    for (size_t i = 0; i < received.count; ++i)
    {
        if (!seen_token(&received, i) && !styxrpc_channel_authenticate_bound(lease->module, received.entries[i].token))
        {
            return STYXRPC_INVALID;
        }
    }
    lease->routes = staged;
    return STYXRPC_OK;
}
