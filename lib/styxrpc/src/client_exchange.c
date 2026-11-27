/* Copyright (c) 2026 bhgv
 * EN: Common verified function open and framed transport for hosted RPC clients.
 * RU: Общая проверка открытия функции и транспорт кадров для клиентов RPC.
 */
#define _POSIX_C_SOURCE 200809L
#include "client_exchange.h"
#include "styxrpc/module_client.h"
#include "vfs_client_internal.h"
#include "call_record.h"
#include "call_scope.h"
#include "load_profile.h"
#include "scalar_verify.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PROFILE_CAPACITY (STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX) /* Whole interface bound. */


/* EN: Read a complete bounded protocol file below borrowed directory using
 * wait for walk, open, data and cleanup. Return zero on failure or overflow.
 * RU: Прочитать ограниченный служебный файл заимствованного directory.
 * Проверять wait при обходе, открытии, чтении и закрытии. Ноль при отказе. */
static size_t read_control(StyxFid *directory, const char *name, void *bytes,
    size_t capacity, StyxRpcVfsWait *wait)
{
    StyxFid *fid = NULL;
    size_t size = 0, complete = 0;
    if (styxrpc_vfs_control_walk_from(directory, name, wait, &fid) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_open(fid, STYXRPC_FILE_READ, wait) == STYXRPC_IO_OK)
    {
        while (size < capacity)
        {
            StyxTransfer part;
            styxclientreaduntil(fid, size, (unsigned char *)bytes + size, capacity - size,
                wait->stop ? styxrpc_vfs_control_poll : NULL, wait, &part);
            if (!part.success || !part.confirmed || part.uncertain || part.again || part.count > capacity - size)
            {
                break;
            }
            if (!part.count)
            {
                complete = size;
                break;
            }
            size += part.count;
        }
    }
    styxrpc_vfs_control_close(fid, wait);
    return complete;
}


/* EN: Match one ordinary client offer on the same held module used for the call.
 * RU: Сопоставить одно предложение клиента на том же удерживаемом модуле. */
static int verify(StyxFid *module, const char *name, uint32_t signature,
    const StyxRpcSchema *schema, int *scoped, StyxRpcVfsWait *wait)
{
    unsigned char bytes[PROFILE_CAPACITY + 1];
    size_t size = read_control(module, ".interface", bytes, sizeof(bytes), wait);
    StyxCallShape shape;
    int framing;
    return styxscalar_profile_match_protocol(bytes, size, name, signature, scoped, &shape, &framing) &&
        (schema ? !strcmp(schema->arguments, shape.arguments) && !strcmp(schema->result, shape.result) &&
            (!*scoped || framing == STYX_CALL_RECORD_FRAMING) : !*scoped);
}


/* EN: Write consecutive confirmed suffixes of one request on the borrowed fid.
 * RU: Записать последовательные подтверждённые остатки одного запроса на fid. */
static StyxRpcOutcome send_request(StyxFid *fid, const unsigned char *request, size_t size,
    StyxRpcCallStop stop, void *state)
{
    size_t offset = 0;
    while (offset < size)
    {
        StyxTransfer part;
        styxclientwriteuntil(fid, offset, request + offset, size - offset, stop, state, &part);
        if (stop && part.cancelled && !part.success)
        {
            return offset || part.uncertain ? STYXRPC_CALL_CANCELLED_UNCERTAIN : STYXRPC_CALL_CANCELLED;
        }
        if (!part.success || !part.confirmed || part.uncertain ||
            !part.count || part.count > size - offset)
        {
            return STYXRPC_CALL_UNCERTAIN;
        }
        offset += part.count;
    }
    return STYXRPC_CALL_OK;
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
static StyxRpcOutcome receive_reply(StyxFid *fid, unsigned wait_ms, unsigned char *reply,
    size_t capacity, size_t *received, StyxRpcCallStop stop, void *state)
{
    const struct timespec pause = {0, 1000000}; /* Yield one millisecond between empty reads. */
    uint64_t began = monotonic_ms();
    size_t used = 0;
    uint64_t now = began;
    while (now && now - began < wait_ms && used < capacity)
    {
        StyxTransfer part;
        styxclientreaduntil(fid, used, reply + used, capacity - used, stop, state, &part);
        if (stop && part.cancelled && !part.success)
        {
            return STYXRPC_CALL_CANCELLED_UNCERTAIN;
        }
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
                return STYXRPC_CALL_OK;
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
    return STYXRPC_CALL_UNCERTAIN;
}


/* EN: Confirm the exact marker on the same captured module directory.
 * RU: Подтвердить точный маркер на том же удерживаемом каталоге модуля. */
static int verify_marker(StyxFid *module, const char *name, const char *marker, StyxRpcVfsWait *wait)
{
    if (!marker)
    {
        return 1;
    }
    char bytes[sizeof(STYX_MANAGED_POSITION_GRAPH_MARKER) + sizeof(STYX_CHANNEL_PROFILE)];
    const size_t expected = strlen(marker);
    return expected < sizeof(bytes) && read_control(module, name, bytes, expected + 1, wait) == expected &&
        !memcmp(bytes, marker, expected);
}


/* EN: Match a requested codec and verify a callable file without opening a call fid.
 * RU: Проверить запрошенный кодек и файл вызова без открытия fid функции. */
static int check_import(StyxFid *directory, const void *bytes, size_t size, const StyxRpcModuleImport *required,
    uint32_t pid, int *caller, StyxRpcVfsWait *wait)
{
    const char *name = required->name;
    int scoped, framing;
    StyxCallShape shape;
    if (!styxscalar_name_length(name) || name[0] == '.' || strchr(name, '/') ||
        !styxscalar_profile_match_protocol(bytes, size, name, required->signature, &scoped, &shape, &framing))
    {
        return 0;
    }
    if (required->kind == STYXRPC_MODULE_INT)
    {
        if (required->signature != STYXRPC_INT_SIGNATURE || scoped || framing)
        {
            return 0;
        }
    }
    else
    {
        const StyxRpcSchema *schema = &required->schema;
        if (required->kind < STYXRPC_MODULE_MANAGED || required->kind > STYXRPC_MODULE_CHANNEL ||
            !memchr(schema->arguments, 0, sizeof(schema->arguments)) ||
            !memchr(schema->result, 0, sizeof(schema->result)) ||
            strcmp(schema->arguments, shape.arguments) || strcmp(schema->result, shape.result) ||
            framing != STYX_CALL_RECORD_FRAMING || (scoped && !pid) ||
            (required->kind != STYXRPC_MODULE_MANAGED &&
                !verify_marker(directory, STYX_MANAGED_DELTA_FILE, STYX_MANAGED_POSITION_GRAPH_MARKER, wait)) ||
            (required->kind == STYXRPC_MODULE_CHANNEL &&
                (!scoped || !verify_marker(directory, STYX_CHANNEL_PROFILE_FILE, STYX_CHANNEL_PROFILE, wait))))
        {
            return 0;
        }
    }
    StyxMeta metadata;
    StyxFid *function = NULL;
    int valid = styxrpc_vfs_control_walk_from(directory, name, wait, &function) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_stat(function, &metadata, wait, STYXRPC_IO_NOT_OPEN) == STYXRPC_IO_OK &&
        !metadata.directory && (metadata.mode & 0444U) && (metadata.mode & 0222U) && (metadata.mode & 0111U);
    styxrpc_vfs_control_close(function, wait);
    if (valid && caller)
    {
        *caller = scoped;
    }
    return valid;
}


/* EN/RU: See styxrpc/module_client.h. / См. styxrpc/module_client.h. */
StyxRpcIoOutcome styxrpc_module_capture_until(StyxClient *session, const char *module, StyxFid *base,
    StyxFid **out, StyxRpcModuleStop stop, void *state)
{
    if (!out || (!base && (!session || !styxrpc_vfs_visible_path(module))))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *directory = NULL;
    StyxRpcIoOutcome status = base ? styxrpc_vfs_control_walk_from(base, ".", &wait, &directory) :
        styxrpc_vfs_control_walk(session, module, &wait, &directory);
    if (status == STYXRPC_IO_OK && !base)
    {
        StyxMeta meta;
        status = styxrpc_vfs_control_stat(directory, &meta, &wait, STYXRPC_IO_NOT_OPEN);
        if (status == STYXRPC_IO_OK && (!meta.directory || !(meta.mode & 0444U) ||
            !(meta.mode & 0111U) || styxrpc_vfs_service_meta(&meta)))
        {
            status = STYXRPC_IO_NOT_OPEN;
        }
    }
    if (status == STYXRPC_IO_OK)
    {
        *out = directory;
    }
    else
    {
        styxrpc_vfs_control_close(directory, &wait);
    }
    return status;
}


/* EN/RU: See styxrpc/module_client.h. / См. styxrpc/module_client.h. */
StyxFid *styxrpc_module_open(StyxClient *session, const char *module)
{
    StyxFid *directory = NULL;
    return styxrpc_module_capture_until(session, module, NULL, &directory, NULL, NULL) ==
        STYXRPC_IO_OK ? directory : NULL;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_check_at_until(StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid, StyxRpcModuleStop stop, void *state)
{
    if (!directory || !imports || !count || count > STYXRPC_MAX_EXPORTS)
    {
        return STYXRPC_INVALID;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    unsigned char bytes[PROFILE_CAPACITY + 1];
    size_t size = read_control(directory, ".interface", bytes, sizeof(bytes), &wait);
    int valid = size != 0;
    for (size_t i = 0; valid && !wait.stopped && i < count; ++i)
    {
        valid = check_import(directory, bytes, size, imports + i, pid, NULL, &wait);
    }
    return wait.stopped ? STYXRPC_CANCELLED : valid ? STYXRPC_OK : STYXRPC_INVALID;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_check_at(StyxFid *directory,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid)
{
    return styxrpc_module_check_at_until(directory, imports, count, pid, NULL, NULL);
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcStatus styxrpc_module_check(StyxClient *session, const char *module,
    const StyxRpcModuleImport *imports, size_t count, uint32_t pid)
{
    if (!imports || !count || count > STYXRPC_MAX_EXPORTS)
    {
        return STYXRPC_INVALID;
    }
    StyxFid *directory = styxrpc_module_open(session, module);
    StyxRpcStatus status = styxrpc_module_check_at(directory, imports, count, pid);
    styxclientclunk(directory);
    return status;
}


#include "client_exports.inc"


/* EN: Hold the module through profile validation and the exact function walk.
 * RU: Удерживать модуль до проверки профиля и поиска точной функции. */
static StyxFid *open_function(StyxFid *base, StyxClient *session, const char *module, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, const char *graph_marker,
    int *scoped, StyxFid **held, StyxRpcVfsWait *wait)
{
    if ((!base && (!session || !module || !*module)) || !styxscalar_name_length(name) ||
        strchr(name, '/') || name[0] == '.')
    {
        return NULL;
    }
    StyxFid *directory = NULL, *fid = NULL;
    StyxRpcIoOutcome walked = base ? styxrpc_vfs_control_walk_from(base, ".", wait, &directory) :
        styxrpc_vfs_control_walk(session, module, wait, &directory);
    StyxMeta metadata;
    if (walked == STYXRPC_IO_OK &&
        styxrpc_vfs_control_stat(directory, &metadata, wait, STYXRPC_IO_NOT_OPEN) == STYXRPC_IO_OK &&
        metadata.directory && (metadata.mode & 0444U) && (metadata.mode & 0111U) &&
        verify(directory, name, signature, schema, scoped, wait) &&
        verify_marker(directory, STYX_MANAGED_DELTA_FILE, graph_marker, wait) &&
        (!held || (*scoped && verify_marker(directory, STYX_CHANNEL_PROFILE_FILE, STYX_CHANNEL_PROFILE, wait))) &&
        styxrpc_vfs_control_walk_from(directory, name, wait, &fid) == STYXRPC_IO_OK &&
        styxrpc_vfs_control_stat(fid, &metadata, wait, STYXRPC_IO_NOT_OPEN) == STYXRPC_IO_OK &&
        !metadata.directory && (metadata.mode & 0444U) && (metadata.mode & 0222U) && (metadata.mode & 0111U) &&
        styxrpc_vfs_control_open(fid, STYXRPC_FILE_RDWR, wait) == STYXRPC_IO_OK)
    {
        if (held)
        {
            *held = directory;
        }
        else
        {
            styxrpc_vfs_control_close(directory, wait);
        }
        return fid;
    }
    styxrpc_vfs_control_close(fid, wait);
    styxrpc_vfs_control_close(directory, wait);
    return NULL;
}


/* EN/RU: See client_exchange.h. / См. client_exchange.h. */
StyxFid *styxrpc_client_function(StyxFid *base, StyxClient *session, const char *module, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, const char *graph_marker, int *scoped, StyxRpcVfsWait *wait)
{
    return open_function(base, session, module, name, signature, schema, graph_marker, scoped, NULL, wait);
}


/* EN/RU: See client_exchange.h. / См. client_exchange.h. */
StyxFid *styxrpc_client_channel_function(StyxFid *base, const StyxRpcManagedTarget *target,
    StyxFid **module, StyxRpcVfsWait *wait)
{
    int scoped;
    return open_function(base, target->session, target->module, target->name, target->signature,
        &target->schema, STYX_MANAGED_POSITION_GRAPH_MARKER, &scoped, module, wait);
}


/* EN: Exchange one complete frame without replay on the borrowed open fid.
 * RU: Обменять один полный кадр через заимствованный fid без повтора. */
StyxRpcOutcome styxrpc_client_exchange(StyxFid *fid, const void *request, size_t size, unsigned wait_ms,
    void *reply, size_t capacity, size_t *received, StyxRpcCallStop stop, void *state)
{
    StyxRpcOutcome sent = send_request(fid, request, size, stop, state);
    return sent == STYXRPC_CALL_OK ?
        receive_reply(fid, wait_ms, reply, capacity, received, stop, state) : sent;
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
StyxRpcOutcome styxrpc_client_managed_exchange(StyxFid *base, const StyxRpcManagedTarget *target,
    const char *graph_marker, const void *request, size_t request_size, void *reply,
    size_t capacity, size_t *received, StyxRpcCallStop stop, void *state)
{
    int scoped;
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *fid = styxrpc_client_function(base, target->session, target->module, target->name,
        target->signature, &target->schema, graph_marker, &scoped, &wait);
    unsigned char *wrapped = NULL;
    size_t wire_size = request_size;
    StyxRpcOutcome outcome = wait.stopped ? STYXRPC_CALL_CANCELLED : STYXRPC_CALL_NOT_SENT;
    if (fid && (!scoped || (wrapped = scope_request(target->pid, request, request_size, &wire_size))))
    {
        outcome = styxrpc_client_exchange(fid, wrapped ? wrapped : request, wire_size,
            target->wait_ms, reply, capacity, received, stop ? styxrpc_vfs_control_poll : NULL, &wait);
    }
    styxrpc_vfs_control_close(fid, &wait);
    free(wrapped);
    return outcome;
}
