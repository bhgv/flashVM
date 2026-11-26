/* Copyright (c) 2026 bhgv
 * EN: Create a file through the exact attached VFS directory, once.
 * RU: Создать файл одним запросом через точный каталог подключённой VFS.
 */
#include "vfs_client_internal.h"
#include "client.h"
#include <string.h>

#define CREATE_PERMISSIONS 0666U /* Regular file, with the provider's ordinary mode policy. */


/* EN/RU: See vfs_client.h. / См. vfs_client.h. */
StyxRpcIoOutcome styxrpc_vfs_create(struct StyxClient *session, const char *path, StyxRpcVfsFile **file)
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
    if (styxclientcreate(directory, name, CREATE_PERMISSIONS, STYXRPC_FILE_WRITE, &result))
    {
        *file = directory;
        return STYXRPC_IO_OK;
    }
    styxclientclunk(directory);
    return result.uncertain ? STYXRPC_IO_UNCERTAIN : STYXRPC_IO_FAILED;
}
