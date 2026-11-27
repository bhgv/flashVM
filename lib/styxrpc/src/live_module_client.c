/* Copyright (c) 2026 bhgv
 * EN: Resolve a remote Dis load request to its exact same-session module fid.
 * RU: Получить точный fid загруженного модуля Dis в той же сессии.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/live_module.h"
#include "styxrpc/module_client.h"
#include "vfs_client_internal.h"
#include <string.h>
#include <time.h>

#define LIVE_MODULE_WAIT_MS 4000L /* Maximum polling interval for a pending load. */
#define LIVE_MODULE_POLL_NS 10000000L /* Pacing between unfinished status reads. */


/* EN: Read a fixed-width generation into id, polling request until the deadline or stop.
 * RU: Прочитать поколение в id, опрашивая request до срока или сигнала stop. */
static StyxRpcIoOutcome receive_generation(StyxFid *request, char *id, StyxRpcModuleStop stop, void *state)
{
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start))
    {
        return STYXRPC_IO_FAILED;
    }
    size_t count = 0;
    do
    {
        size_t received;
        StyxRpcIoOutcome outcome = styxrpc_vfs_read_wait(request, count, id + count,
            STYXRPC_LIVE_MODULE_ID_BYTES - count, &received, stop, state);
        if (outcome != STYXRPC_IO_OK)
        {
            return outcome == STYXRPC_IO_CANCELLED ? outcome : STYXRPC_IO_FAILED;
        }
        count += received;
        if (count == STYXRPC_LIVE_MODULE_ID_BYTES)
        {
            return strspn(id, "0123456789abcdef") == count && strspn(id, "0") != count ?
                STYXRPC_IO_OK : STYXRPC_IO_FAILED;
        }
        const struct timespec pause = {0, LIVE_MODULE_POLL_NS};
        nanosleep(&pause, NULL);
    } while (!clock_gettime(CLOCK_MONOTONIC, &now) &&
        (now.tv_sec - start.tv_sec) * 1000L + (now.tv_nsec - start.tv_nsec) / 1000000L < LIVE_MODULE_WAIT_MS);
    return STYXRPC_IO_FAILED;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxRpcIoOutcome styxrpc_module_live_open_until(StyxClient *session, const char *path,
    StyxFid **directory, StyxFid **lease, StyxRpcModuleStop stop, void *state)
{
    if (!directory || !lease || directory == lease || !session || !styxrpc_vfs_visible_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    size_t length = strlen(path);
    if (length <= 4 || strcmp(path + length - 4, ".dis"))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    if (stop && stop(state))
    {
        return STYXRPC_IO_CANCELLED;
    }
    StyxRpcIoOutcome outcome = STYXRPC_IO_NOT_OPEN;
    StyxFid *request = styxclientwalk(session, STYXRPC_LIVE_MODULE_PATH);
    StyxFid *module = NULL;
    StyxMeta info;
    if (request && styxclientstat(request, &info) && !info.directory &&
        !strcmp(info.name, STYXRPC_LIVE_MODULE_REQUEST) && styxclientopen(request, STYXRPC_FILE_RDWR))
    {
        StyxTransfer sent;
        styxclientwriteuntil(request, 0, path, length, stop, state, &sent);
        outcome = stop && sent.cancelled && !sent.success ? STYXRPC_IO_CANCELLED :
            styxrpc_vfs_transfer_outcome(&sent, length);
        char address[] = STYXRPC_LIVE_MODULE_ROOT "/00000000";
        if (outcome == STYXRPC_IO_OK)
        {
            outcome = sent.count == length ? receive_generation(request,
                address + sizeof(STYXRPC_LIVE_MODULE_ROOT), stop, state) : STYXRPC_IO_FAILED;
        }
        if (outcome == STYXRPC_IO_OK)
        {
            module = styxclientwalk(session, address);
            if (!module || !styxclientstat(module, &info) || !info.directory ||
                !(info.mode & 0444U) || !(info.mode & 0111U))
            {
                outcome = STYXRPC_IO_FAILED;
            }
        }
    }
    if (outcome == STYXRPC_IO_OK && stop && stop(state))
    {
        outcome = STYXRPC_IO_CANCELLED;
    }
    if (outcome != STYXRPC_IO_OK)
    {
        styxclientclunk(module);
        styxclientclunk(request);
        return outcome;
    }
    *directory = module;
    *lease = request;
    return STYXRPC_IO_OK;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxFid *styxrpc_module_live_open(StyxClient *session, const char *path, StyxFid **lease)
{
    StyxFid *directory = NULL;
    return styxrpc_module_live_open_until(session, path, &directory, lease, NULL, NULL) == STYXRPC_IO_OK ?
        directory : NULL;
}
