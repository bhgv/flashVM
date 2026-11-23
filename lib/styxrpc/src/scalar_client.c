/* Copyright (c) 2026 bhgv
 * EN: Verify a mounted interface and perform one scalar call without replay.
 * RU: Проверка смонтированного интерфейса и скалярный вызов без повтора.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/scalar_client.h"
#include "load_profile.h"
#include "scalar_verify.h"
#include "flashvm/srecord.h"
#include "client.h"
#include <string.h>
#include <time.h>

#define PROFILE_CAPACITY (STYX_MOUNTED_EXPORT_LIMIT * STYX_SHAPED_OFFER_MAX) /* Whole interface bound. */


/* EN: Read a complete, bounded interface from the same held module used for the call.
 * RU: Прочитать полный ограниченный интерфейс из того же удерживаемого модуля. */
static int verify(StyxFid *module, const char *name, uint32_t signature)
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
                int scoped;
                matched = styxscalar_profile_match(bytes, size, name, signature, &scoped) && !scoped;
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
static StyxRpcOutcome receive_reply(StyxFid *fid, uint32_t signature, unsigned wait_ms, int64_t *result)
{
    const struct timespec pause = {0, 1000000}; /* Yield one millisecond between empty reads. */
    uint64_t began = monotonic_ms();
    unsigned char reply[STYXRPC_FRAME_SIZE];
    size_t used = 0;
    uint64_t now = began;
    while (now && now - began < wait_ms && used < sizeof(reply))
    {
        StyxTransfer part;
        styxclientread(fid, used, reply + used, sizeof(reply) - used, &part);
        if (!part.success || !part.confirmed || part.uncertain || part.count > sizeof(reply) - used)
        {
            break;
        }
        if (part.count)
        {
            used += part.count;
            FVM_SRecord record;
            size_t extent;
            FVM_SResult parsed = fvm_srecord_read(reply, used, sizeof(reply), &record, &extent);
            if (parsed == FVM_S_OK && extent == used)
            {
                StyxRpcStatus status = styxrpc_reply(signature, reply, used, result);
                return status == STYXRPC_OK ? STYXRPC_CALL_OK :
                    status == STYXRPC_REMOTE_ERROR ? STYXRPC_CALL_REMOTE_ERROR : STYXRPC_CALL_UNCERTAIN;
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


/* EN/RU: See scalar_client.h. / См. scalar_client.h. */
StyxRpcOutcome styxrpc_scalar_invoke(StyxClient *session, const char *module, const char *name,
    uint32_t signature, int64_t argument, unsigned wait_ms, int64_t *result)
{
    unsigned char request[STYXRPC_FRAME_SIZE];
    size_t size;
    if (!session || !module || !*module || !styxscalar_name_length(name) || strchr(name, '/') ||
        name[0] == '.' || !wait_ms || wait_ms > STYXRPC_SCALAR_WAIT_MAX_MS || !result ||
        styxrpc_request(signature, argument, request, sizeof(request), &size) != STYXRPC_OK)
    {
        return STYXRPC_CALL_NOT_SENT;
    }
    StyxFid *directory = styxclientwalk(session, module);
    StyxMeta metadata;
    StyxFid *fid = directory && styxclientstat(directory, &metadata) && metadata.directory &&
        (metadata.mode & 0444U) && (metadata.mode & 0111U) && verify(directory, name, signature) ?
        styxclientwalkfrom(directory, name) : NULL;
    styxclientclunk(directory);
    StyxRpcOutcome outcome = STYXRPC_CALL_NOT_SENT;
    if (fid && styxclientstat(fid, &metadata) && !metadata.directory &&
        (metadata.mode & 0444U) && (metadata.mode & 0222U) && (metadata.mode & 0111U) && styxclientopen(fid, 2))
    {
        outcome = send_request(fid, request, size) ?
            receive_reply(fid, signature, wait_ms, result) : STYXRPC_CALL_UNCERTAIN;
    }
    styxclientclunk(fid);
    return outcome;
}
