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


/* EN: Read size bytes at offset on file, sampling optional stop(state).
 * Return the outcome and the confirmed prefix in used, including on cancellation.
 * RU: Прочитать size байтов с offset на file с необязательным stop(state).
 * Вернуть исход и подтверждённую длину в used, в том числе при отмене. */
StyxRpcIoOutcome styxrpc_vfs_read_wait(StyxRpcVfsFile *file, uint64_t offset, void *bytes,
    size_t size, size_t *used, StyxClientStop stop, void *state);


/* EN: Write size bytes at offset on file with the same stop contract and confirmed prefix.
 * The cancelled fragment may already have taken effect.
 * RU: Записать size байтов с offset на file с той же отменой и подтверждённой длиной.
 * Отменённый фрагмент мог уже изменить данные. */
StyxRpcIoOutcome styxrpc_vfs_write_wait(StyxRpcVfsFile *file, uint64_t offset, const void *bytes,
    size_t size, size_t *used, StyxClientStop stop, void *state);
#endif
