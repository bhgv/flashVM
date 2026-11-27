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


/* EN: Read one fixed-width generation from the request fid with bounded polling.
 * RU: Прочитать поколение фиксированной длины через fid запроса с ограниченным ожиданием. */
static int receive_generation(StyxFid *request, char *id)
{
    struct timespec start, now;
    if (clock_gettime(CLOCK_MONOTONIC, &start))
    {
        return 0;
    }
    size_t count = 0;
    do
    {
        size_t received;
        if (styxrpc_vfs_file_read(request, count, id + count,
            STYXRPC_LIVE_MODULE_ID_BYTES - count, &received) != STYXRPC_IO_OK)
        {
            return 0;
        }
        count += received;
        if (count == STYXRPC_LIVE_MODULE_ID_BYTES)
        {
            return strspn(id, "0123456789abcdef") == count && strspn(id, "0") != count;
        }
        const struct timespec pause = {0, LIVE_MODULE_POLL_NS};
        nanosleep(&pause, NULL);
    } while (!clock_gettime(CLOCK_MONOTONIC, &now) &&
        (now.tv_sec - start.tv_sec) * 1000L + (now.tv_nsec - start.tv_nsec) / 1000000L < LIVE_MODULE_WAIT_MS);
    return 0;
}


/* EN/RU: See module_client.h. / См. module_client.h. */
StyxFid *styxrpc_module_live_open(StyxClient *session, const char *path, StyxFid **lease)
{
    if (!lease || !session || !styxrpc_vfs_visible_path(path))
    {
        return NULL;
    }
    size_t length = strlen(path);
    if (length <= 4 || strcmp(path + length - 4, ".dis"))
    {
        return NULL;
    }
    StyxFid *request = styxclientwalk(session, STYXRPC_LIVE_MODULE_PATH);
    StyxFid *module = NULL;
    StyxMeta info;
    if (request && styxclientstat(request, &info) && !info.directory &&
        !strcmp(info.name, STYXRPC_LIVE_MODULE_REQUEST) && styxclientopen(request, STYXRPC_FILE_RDWR))
    {
        StyxTransfer sent;
        styxclientwrite(request, 0, path, length, &sent);
        char address[] = STYXRPC_LIVE_MODULE_ROOT "/00000000";
        char *id = address + sizeof(STYXRPC_LIVE_MODULE_ROOT);
        if (styxrpc_vfs_transfer_outcome(&sent, length) == STYXRPC_IO_OK && sent.count == length &&
            receive_generation(request, id))
        {
            module = styxclientwalk(session, address);
            if (module && (!styxclientstat(module, &info) || !info.directory ||
                !(info.mode & 0444U) || !(info.mode & 0111U)))
            {
                styxclientclunk(module);
                module = NULL;
            }
        }
    }
    if (module)
    {
        *lease = request;
    }
    else
    {
        styxclientclunk(request);
    }
    return module;
}
