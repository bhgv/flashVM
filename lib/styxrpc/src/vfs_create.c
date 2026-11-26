/* Copyright (c) 2026 bhgv
 * EN: Create a file through the exact attached VFS directory, once.
 * RU: Создать файл одним запросом через точный каталог подключённой VFS.
 */
#include "vfs_client_internal.h"
#include "client.h"
#include <string.h>

#define CREATE_PERMISSIONS 0666U /* Regular file, with the provider's ordinary mode policy. */
#define DIRECTORY_PERMISSIONS 0777U /* Directory permission ceiling before provider policy. */


/* EN: Walk and validate the parent of a visible path, then create one child.
 * RU: Проверить родительский каталог видимого пути, затем создать один узел. */
static StyxRpcIoOutcome create(struct StyxClient *session, const char *path, int mkdir, StyxRpcVfsFile **file)
{
    if (!session || !file || !styxrpc_vfs_visible_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    char parent[STYXRPC_VFS_PATH_MAX + 1];
    const char *name = strrchr(path, '/');
    if (name)
    {
        size_t length = (size_t)(name - path);
        memcpy(parent, path, length);
        parent[length] = 0;
        ++name;
    }
    else
    {
        strcpy(parent, ".");
        name = path;
    }
    StyxFid *directory = styxclientwalk(session, parent);
    if (!directory)
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxMeta info;
    if (!styxclientstat(directory, &info) || !info.directory || styxrpc_vfs_service_meta(&info))
    {
        styxclientclunk(directory);
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxTransfer result;
    int made = mkdir ? styxclientmkdir(directory, name, DIRECTORY_PERMISSIONS, &result) :
        styxclientcreate(directory, name, CREATE_PERMISSIONS, STYXRPC_FILE_WRITE, &result);
    if (made)
    {
        *file = directory;
        return STYXRPC_IO_OK;
    }
    styxclientclunk(directory);
    return result.uncertain ? STYXRPC_IO_UNCERTAIN : STYXRPC_IO_FAILED;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_create(struct StyxClient *session, const char *path, StyxRpcVfsFile **file)
{
    return create(session, path, 0, file);
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_mkdir(struct StyxClient *session, const char *path)
{
    StyxFid *made = NULL;
    StyxRpcIoOutcome result = create(session, path, 1, &made);
    styxclientclunk(made);
    return result;
}


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_remove(struct StyxClient *session, const char *path)
{
    if (!session || !styxrpc_vfs_visible_path(path))
    {
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxFid *fid = styxclientwalk(session, path);
    StyxMeta info;
    if (!fid || !styxclientstat(fid, &info) || styxrpc_vfs_service_meta(&info))
    {
        styxclientclunk(fid);
        return STYXRPC_IO_NOT_OPEN;
    }
    StyxTransfer result;
    if (styxclientremove(fid, &result))
    {
        return STYXRPC_IO_OK;
    }
    return result.uncertain ? STYXRPC_IO_UNCERTAIN : STYXRPC_IO_FAILED;
}
