/* Copyright (c) 2026 bhgv
 * EN: Common public path admission for the hosted client.
 * RU: Общий допуск публичных путей хостового клиента.
 */
#ifndef STYXRPC_VFS_CLIENT_INTERNAL_H
#define STYXRPC_VFS_CLIENT_INTERNAL_H
#include "styxrpc/vfs_client.h"
#include "client.h"
/* EN: Check a nonempty relative path outside reserved service components.
 * RU: Проверить непустой относительный путь вне служебных компонентов. */
int styxrpc_vfs_visible_path(const char *path);
/* EN: Open a protocol-selected service path from the attached root.
 * RU: Открыть выбранный протоколом служебный путь от корня подключения. */
StyxRpcIoOutcome styxrpc_vfs_exchange_open(struct StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file);
/* EN/RU: Open a visible directory. / Открыть видимый каталог. */
StyxFid *styxrpc_vfs_open_directory_fid(StyxClient *, const char *);

/* EN/RU: Identify hidden metadata. / Определить скрытые метаданные. */
int styxrpc_vfs_service_meta(const StyxMeta *);

/* EN/RU: Map a native transfer outcome. / Сопоставить результат передачи. */
StyxRpcIoOutcome styxrpc_vfs_transfer_outcome(const StyxTransfer *, size_t);


#endif
