/* Copyright (c) 2026 bhgv
 * EN: Submit one ordinary creation or removal on a validated exact fid.
 * RU: Выполнить одно создание или удаление по проверенному точному fid.
 */
#include "vfs_client_internal.h"
#include <string.h>

#define CREATE_PERMISSIONS 0666U /* Regular file, with the provider's ordinary mode policy. */
#define DIRECTORY_PERMISSIONS 0777U /* Directory permission ceiling before provider policy. */


/* EN: Walk and stat the borrowed path under wait. Publish the owned visible
 * fid only on success; directory requires its target to be a directory.
 * RU: Пройти заимствованный путь и проверить метаданные с wait. Выдать fid
 * видимого узла при успехе. directory требует типа каталога. */
static StyxRpcIoOutcome mutation_entry(StyxClient *session, const char *path, int directory,
    StyxRpcVfsWait *wait, StyxFid **out)
{
    StyxFid *fid = NULL;
    StyxMeta info;
    StyxRpcIoOutcome status = styxrpc_vfs_control_walk(session, path, wait, &fid);
    if (status == STYXRPC_IO_OK)
    {
        status = styxrpc_vfs_control_stat(fid, &info, wait, STYXRPC_IO_NOT_OPEN);
        if (status == STYXRPC_IO_OK && ((directory && !info.directory) || styxrpc_vfs_service_meta(&info)))
        {
            status = STYXRPC_IO_NOT_OPEN;
        }
    }
    if (status == STYXRPC_IO_OK)
    {
        *out = fid;
    }
    else
    {
        styxrpc_vfs_close(fid, wait->stop != NULL);
    }
    return status;
}


/* EN: Validate the parent of the borrowed path, then make exactly one child.
 * mkdir selects the directory profile. On acknowledgement file receives the
 * opened parent fid, now naming the child. Otherwise retire the temporary fid.
 * RU: Проверить родителя заимствованного пути и создать ровно одного потомка.
 * mkdir выбирает профиль каталога. При подтверждении file получает прежний
 * fid родителя, открытый на потомке. Иначе временный fid закрывается. */
static StyxRpcIoOutcome create(StyxClient *session, const char *path, int mkdir,
    StyxRpcVfsFile **file, StyxRpcVfsStop stop, void *state)
{
    if (!session || !file || !styxrpc_vfs_visible_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    char parent[STYXRPC_VFS_PATH_MAX + 1] = ".";
    const char *slash = strrchr(path, '/');
    if (slash)
    {
        size_t length = (size_t)(slash - path);
        memcpy(parent, path, length);
        parent[length] = 0;
    }
    const char *name = slash ? slash + 1 : path;
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *directory = NULL;
    StyxRpcIoOutcome status = mutation_entry(session, parent, 1, &wait, &directory);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    StyxCancel *token;
    status = styxrpc_vfs_control_begin(&wait, styxclientcancellable(session), &token);
    if (status == STYXRPC_IO_OK)
    {
        StyxTransfer result;
        if (mkdir)
        {
            styxclientmkdircancel(directory, name, DIRECTORY_PERMISSIONS, token, &result);
        }
        else
        {
            styxclientcreatecancel(directory, name, CREATE_PERMISSIONS, STYXRPC_FILE_WRITE, token, &result);
        }
        status = styxrpc_vfs_control_end(&wait, token, &result, 1, STYXRPC_IO_FAILED);
    }
    if (status == STYXRPC_IO_OK)
    {
        *file = directory;
    }
    else
    {
        styxrpc_vfs_close(directory, stop != NULL);
    }
    return status;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_create_until(StyxClient *session, const char *path, StyxRpcVfsFile **file,
    StyxRpcVfsStop stop, void *state)
{
    return create(session, path, 0, file, stop, state);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_create(StyxClient *session, const char *path, StyxRpcVfsFile **file)
{
    return styxrpc_vfs_create_until(session, path, file, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_mkdir_until(StyxClient *session, const char *path,
    StyxRpcVfsStop stop, void *state)
{
    StyxFid *made = NULL;
    StyxRpcIoOutcome result = create(session, path, 1, &made, stop, state);
    styxrpc_vfs_close(made, stop != NULL);
    return result;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_mkdir(StyxClient *session, const char *path)
{
    return styxrpc_vfs_mkdir_until(session, path, NULL, NULL);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_remove_until(StyxClient *session, const char *path,
    StyxRpcVfsStop stop, void *state)
{
    if (!session || !styxrpc_vfs_visible_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxRpcVfsWait wait = {stop, state, 0};
    StyxFid *fid = NULL;
    StyxRpcIoOutcome status = mutation_entry(session, path, 0, &wait, &fid);
    if (status != STYXRPC_IO_OK)
    {
        return status;
    }
    StyxCancel *token;
    status = styxrpc_vfs_control_begin(&wait, styxclientcancellable(session), &token);
    if (status != STYXRPC_IO_OK)
    {
        styxrpc_vfs_close(fid, stop != NULL);
        return status;
    }
    StyxTransfer result;
    styxclientremovecancel(fid, token, &result);
    return styxrpc_vfs_control_end(&wait, token, &result, 1, STYXRPC_IO_FAILED);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_remove(StyxClient *session, const char *path)
{
    return styxrpc_vfs_remove_until(session, path, NULL, NULL);
}
