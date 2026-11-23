/* Copyright (c) 2026 bhgv
 * EN: Existing one-shot launch profile, definite outcomes, and no replay.
 * RU: Существующий профиль запуска, подтверждённый исход и отсутствие повторов.
 */
#include "styxrpc/launch.h"
#include "client.h"
#include "flashvm/srecord.h"
#include <string.h>

#define LAUNCH_TAG 17UL     /* Existing DisVM launch request tag. */
#define LAUNCH_LIMIT 65536U /* Maximum startup argument extent. */


/* EN: Borrow client to read a bounded prefix from a fresh read-only fid.
 * RU: Использовать client для ограниченного чтения на новом fid. */
static int read_snapshot(StyxClient *client, const char *path, char *bytes, size_t capacity, size_t *size)
{
    StyxFid *fid = client ? styxclientwalk(client, path) : NULL;
    int opened = fid && styxclientopen(fid, 0);
    StyxTransfer got = {0};
    if (opened)
    {
        styxclientread(fid, 0, bytes, capacity, &got);
    }
    styxclientclunk(fid);
    *size = got.count;
    return got.success && got.confirmed && !got.uncertain;
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_start(StyxClient *client, const void *arguments, size_t size,
    void *scratch, size_t capacity)
{
    static const char profile[] = "disvm.launch/1 raw\n";
    char offered[sizeof(profile)];
    size_t length, frame;
    if (!client || (size && !arguments) || size > LAUNCH_LIMIT || !scratch ||
        fvm_srecord_write(LAUNCH_TAG, arguments, size, scratch, capacity, &frame) != FVM_S_OK ||
        !read_snapshot(client, ".interface", offered, sizeof(offered), &length) ||
        length != sizeof(profile) - 1 || memcmp(offered, profile, length))
    {
        return STYXRPC_LAUNCH_NOT_SENT;
    }
    StyxFid *fid = styxclientwalk(client, ".unibus");
    StyxTransfer sent = {0};
    int opened = fid && styxclientopen(fid, 1);
    if (opened)
    {
        styxclientwrite(fid, 0, scratch, frame, &sent);
    }
    styxclientclunk(fid);
    if (sent.success && sent.confirmed && !sent.uncertain && sent.count == frame)
    {
        return STYXRPC_LAUNCH_PENDING;
    }
    return sent.uncertain || sent.count || (opened && !sent.confirmed) ?
        STYXRPC_LAUNCH_UNKNOWN : STYXRPC_LAUNCH_NOT_SENT;
}


/* EN/RU: See launch.h. / См. launch.h. */
StyxRpcLaunchStatus styxrpc_launch_status(StyxClient *client)
{
    char reply[7];
    size_t length;
    if (!read_snapshot(client, ".exit", reply, sizeof(reply), &length))
    {
        return STYXRPC_LAUNCH_UNKNOWN;
    }
    if (!length)
    {
        return STYXRPC_LAUNCH_PENDING;
    }
    if (length == 3 && !memcmp(reply, "ok\n", 3))
    {
        return STYXRPC_LAUNCH_OK;
    }
    return length == 6 && !memcmp(reply, "error\n", 6) ?
        STYXRPC_LAUNCH_FAILED : STYXRPC_LAUNCH_UNKNOWN;
}
