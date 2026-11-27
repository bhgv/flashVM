/* Copyright (c) 2026 bhgv
 * EN: Exclusive create and removal through a retained Styx mount.
 * RU: Исключительное создание и удаление через удерживаемое монтирование Styx.
 */
#include "mount_internal.h"
#include "mount_cancel.h"
#include "styxrpc/exchange_qid.h"
#include <stdlib.h>
#include <string.h>


/* EN: Check the borrowed mount's authority and canonical descendant path.
 * Return an FVM status, distinguishing a proven cycle from other refusals.
 * RU: Проверить права монтирования и канонический путь потомка.
 * Вернуть статус FVM, выделяя подтверждённый цикл среди других отказов. */
static FVM_Result admit(struct Mount *mount, const char *path, const char *operation)
{
    if (mount->file_root || !strcmp(path, ".") ||
        (mount->access & (FVM_ACCESS_WRITE | FVM_ACCESS_EXEC)) != (FVM_ACCESS_WRITE | FVM_ACCESS_EXEC))
    {
        return FVM_UNSUPPORTED;
    }
    int probe = styxclientwalkprobe(mount->client, mount->root, path, WALK_OPEN, operation);
    return probe == WALK_OK ? FVM_OK : probe == WALK_CYCLE ? DVM_VFS_CYCLE : FVM_ERROR;
}


/* EN/RU: See mount_internal.h. / См. mount_internal.h. */
FVM_Result styxmountcreatefile(struct Mount *mount, const char *path, DVM_VfsHandle *out)
{
    FVM_Result status = admit(mount, path, "wx");
    if (status != FVM_OK)
    {
        return status;
    }
    char parent[WALK_PATH_LIMIT] = ".";
    const char *slash = strrchr(path, '/');
    if (slash)
    {
        size_t length = (size_t)(slash - path);
        if (!length || length >= sizeof(parent))
        {
            return FVM_ERROR;
        }
        memcpy(parent, path, length);
        parent[length] = 0;
    }
    struct Mounted *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return FVM_OUT_OF_MEMORY;
    }
    opened->mount = mount;
    opened->fid = styxmountwalk(mount, parent);
    StyxMeta meta;
    StyxMountCancel binding;
    if (opened->fid && styxmountstat(mount->client, opened->fid, &meta) && meta.directory &&
        !STYXRPC_EXCHANGE_QID(meta.identity) && styxmountcancel(mount->client, &binding))
    {
        StyxTransfer reply;
        int made = styxclientcreatecancel(opened->fid, slash ? slash + 1 : path, 0666U, 1, binding.token, &reply);
        styxmountcancelend(&binding);
        if (made)
        {
            out->native_flags = styxclientcancellable(mount->client) ?
                DVM_VFS_CANCEL_CLOSE | DVM_VFS_CANCEL_INFO | DVM_VFS_CANCEL_DATA | DVM_VFS_CANCEL_SEEK : 0;
            out->ops = &styxmountedops;
            out->state = opened;
            return FVM_OK;
        }
    }
    styxmountclunk(mount->client, opened->fid);
    free(opened);
    return FVM_ERROR;
}


/* EN/RU: See mount_internal.h. / См. mount_internal.h. */
FVM_Result styxmountremovefile(void *state, const char *path)
{
    struct Mount *mount = state;
    FVM_Result status = admit(mount, path, "remove");
    if (status != FVM_OK)
    {
        return status;
    }
    StyxFid *fid = styxmountwalk(mount, path);
    StyxMeta meta;
    StyxMountCancel binding;
    if (!fid || !styxmountstat(mount->client, fid, &meta) ||
        STYXRPC_EXCHANGE_QID(meta.identity) || !styxmountcancel(mount->client, &binding))
    {
        styxmountclunk(mount->client, fid);
        return FVM_ERROR;
    }
    int removed = styxclientremovecancel(fid, binding.token, NULL);
    styxmountcancelend(&binding);
    return removed ? FVM_OK : FVM_ERROR;
}
