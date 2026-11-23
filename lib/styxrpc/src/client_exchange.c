/* Copyright (c) 2026 bhgv
 * EN: Common verified function open and framed transport for hosted RPC clients.
 * RU: Общая проверка открытия функции и транспорт кадров для клиентов RPC.
 */
#define _POSIX_C_SOURCE 200809L
#include "client_exchange.h"
#include "call_scope.h"
#include "load_profile.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROFILE_CAPACITY (STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX) /* Whole interface bound. */


/* EN: Read a complete, bounded interface from the same held module used for the call.
 * RU: Прочитать полный ограниченный интерфейс из того же удерживаемого модуля. */
static int verify(StyxFid *module, const char *name, uint32_t signature,
    const StyxRpcSchema *schema, int *scoped)
{
    StyxFid *profile = styxclientwalkfrom(module, ".interface");
    unsigned char bytes[PROFILE_CAPACITY + 1];
    size_t size = 0;
    int matched = 0;
    if (profile && styxclientopen(profile, 0))
    {
        while (size < sizeof(bytes))
        {
            StyxTransfer part;
            styxclientread(profile, size, bytes + size, sizeof(bytes) - size, &part);
            if (!part.success || !part.confirmed || part.uncertain || part.again ||
                part.count > sizeof(bytes) - size)
            {
                break;
            }
            if (!part.count)
            {
                StyxCallShape shape;
                int framing;
                matched = styxscalar_profile_match_protocol(bytes, size, name, signature, scoped, &shape, &framing);
                if (matched)
                {
                    matched = schema ? !strcmp(schema->arguments, shape.arguments) &&
                        !strcmp(schema->result, shape.result) &&
                        (!*scoped || framing == STYX_CALL_RECORD_FRAMING) : !*scoped;
                }
                break;
            }
            size += part.count;
        }
    }
    styxclientclunk(profile);
    return matched;
}


/* EN: Write consecutive confirmed suffixes of one request on the borrowed fid.
 * RU: Записать последовательные подтверждённые остатки одного запроса на fid. */
static int send_request(StyxFid *fid, const unsigned char *request, size_t size)
{
    size_t offset = 0;
    while (offset < size)
    {
        StyxTransfer part;
        styxclientwrite(fid, offset, request + offset, size - offset, &part);
        if (!part.success || !part.confirmed || part.uncertain ||
            !part.count || part.count > size - offset)
        {
            return 0;
        }
        offset += part.count;
    }
    return 1;
}


/* EN: Return a monotonic millisecond timestamp, zero on clock failure.
 * RU: Вернуть монотонное время в миллисекундах или ноль при ошибке. */
static uint64_t monotonic_ms(void)
{
    struct timespec now;
    return clock_gettime(CLOCK_MONOTONIC, &now) == 0 ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Poll for the first response bytes, then consume exactly one framed reply.
 * RU: Дождаться первых байтов ответа и прочитать ровно одну запись. */
static int receive_reply(StyxFid *fid, unsigned wait_ms, unsigned char *reply,
    size_t capacity, size_t *received)
{
    const struct timespec pause = {0, 1000000}; /* Yield one millisecond between empty reads. */
    uint64_t began = monotonic_ms();
    size_t used = 0;
    uint64_t now = began;
    while (now && now - began < wait_ms && used < capacity)
    {
        StyxTransfer part;
        styxclientread(fid, used, reply + used, capacity - used, &part);
        if (!part.success || !part.confirmed || part.uncertain || part.count > capacity - used)
        {
            break;
        }
        if (part.count)
        {
            used += part.count;
            FVM_SRecord record;
            size_t extent;
            FVM_SResult parsed = fvm_srecord_read(reply, used, capacity, &record, &extent);
            if (parsed == FVM_S_OK && extent == used)
            {
                *received = used;
                return 1;
            }
            if (parsed != FVM_S_MORE)
            {
                break;
            }
        }
        else if (used)
        {
            break;
        }
        else
        {
            nanosleep(&pause, NULL);
        }
        now = monotonic_ms();
    }
    return 0;
}


/* EN: Confirm the requested graph dialect on the same captured module directory.
 * RU: Подтвердить формат графа на том же удерживаемом каталоге модуля. */
static int verify_graph(StyxFid *module, const char *marker)
{
    if (!marker)
    {
        return 1;
    }
    StyxFid *field = styxclientwalkfrom(module, STYX_MANAGED_DELTA_FILE);
    char bytes[sizeof(STYX_MANAGED_POSITION_GRAPH_MARKER)];
    size_t size = 0;
    int matched = 0;
    if (field && styxclientopen(field, 0))
    {
        while (size < sizeof(bytes))
        {
            StyxTransfer part;
            styxclientread(field, size, bytes + size, sizeof(bytes) - size, &part);
            if (!part.success || !part.confirmed || part.uncertain || part.again ||
                part.count > sizeof(bytes) - size)
            {
                break;
            }
            if (!part.count)
            {
                matched = size == strlen(marker) && !memcmp(bytes, marker, size);
                break;
            }
            size += part.count;
        }
    }
    styxclientclunk(field);
    return matched;
}


/* EN: Hold the module through profile validation and the exact function walk.
 * RU: Удерживать модуль до проверки профиля и поиска точной функции. */
StyxFid *styxrpc_client_function(StyxClient *session, const char *module, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, const char *graph_marker, int *scoped)
{
    if (!session || !module || !*module || !styxscalar_name_length(name) ||
        strchr(name, '/') || name[0] == '.')
    {
        return NULL;
    }
    StyxFid *directory = styxclientwalk(session, module);
    StyxMeta metadata;
    StyxFid *fid = directory && styxclientstat(directory, &metadata) && metadata.directory &&
        (metadata.mode & 0444U) && (metadata.mode & 0111U) && verify(directory, name, signature, schema, scoped) &&
        verify_graph(directory, graph_marker) ?
        styxclientwalkfrom(directory, name) : NULL;
    styxclientclunk(directory);
    if (fid && (!styxclientstat(fid, &metadata) || metadata.directory || !(metadata.mode & 0444U) ||
        !(metadata.mode & 0222U) || !(metadata.mode & 0111U) || !styxclientopen(fid, 2)))
    {
        styxclientclunk(fid);
        return NULL;
    }
    return fid;
}


/* EN: Exchange one complete frame without replay on the borrowed open fid.
 * RU: Обменять один полный кадр через заимствованный fid без повтора. */
int styxrpc_client_exchange(StyxFid *fid, const void *request, size_t size, unsigned wait_ms,
    void *reply, size_t capacity, size_t *received)
{
    return send_request(fid, request, size) && receive_reply(fid, wait_ms, reply, capacity, received);
}


/* EN: Add the published caller PID as the first field of one complete request.
 * RU: Добавить опубликованный PID первым полем полного запроса. */
static unsigned char *scope_request(uint32_t pid, const void *request, size_t size, size_t *used)
{
    FVM_SRecord inner;
    unsigned char prefix[STYXRPC_FRAME_SIZE];
    size_t extent, prefix_size;
    if (!pid || fvm_srecord_read(request, size, STYXRPC_MANAGED_LIMIT, &inner, &extent) != FVM_S_OK ||
        extent != size || styxrpc_value_integer(STYXRPC_VALUE_BIG, pid, prefix, sizeof(prefix), &prefix_size) != STYXRPC_OK)
    {
        return NULL;
    }
    unsigned char *scoped = malloc(STYXRPC_MANAGED_LIMIT);
    if (scoped && fvm_srecord_write_pair(inner.tag, prefix, prefix_size, inner.payload, inner.size,
        scoped, STYXRPC_MANAGED_LIMIT, used) != FVM_S_OK)
    {
        free(scoped);
        scoped = NULL;
    }
    return scoped;
}


/* EN/RU: See client_exchange.h. / См. client_exchange.h. */
StyxRpcOutcome styxrpc_client_managed_exchange(const StyxRpcManagedTarget *target,
    const char *graph_marker, const void *request, size_t request_size, void *reply,
    size_t capacity, size_t *received)
{
    int scoped;
    StyxFid *fid = styxrpc_client_function(target->session, target->module, target->name,
        target->signature, &target->schema, graph_marker, &scoped);
    unsigned char *wrapped = NULL;
    size_t wire_size = request_size;
    StyxRpcOutcome outcome = STYXRPC_CALL_NOT_SENT;
    if (fid && (!scoped || (wrapped = scope_request(target->pid, request, request_size, &wire_size))))
    {
        outcome = styxrpc_client_exchange(fid, wrapped ? wrapped : request, wire_size,
            target->wait_ms, reply, capacity, received) ? STYXRPC_CALL_OK : STYXRPC_CALL_UNCERTAIN;
    }
    styxclientclunk(fid);
    free(wrapped);
    return outcome;
}
