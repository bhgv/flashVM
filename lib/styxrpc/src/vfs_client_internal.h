/* Copyright (c) 2026 bhgv
 * EN: Common public path admission for the hosted client.
 * RU: Общий допуск публичных путей хостового клиента.
 */
#ifndef STYXRPC_VFS_CLIENT_INTERNAL_H
#define STYXRPC_VFS_CLIENT_INTERNAL_H
#include "styxrpc/vfs_client.h"
#include "client.h"


/* EN: Hold the borrowed caller hook for one synchronous multi-stage operation.
 * RU: Удерживать заимствованный колбэк одного синхронного многошагового вызова. */
typedef struct StyxRpcVfsWait
{
    StyxRpcVfsStop stop; /* Optional native callback sampled on the calling thread. */
    void *state;        /* Borrowed callback argument. */
    int stopped;        /* Sticky observation of the caller's stop. */
} StyxRpcVfsWait;


/* EN: Before a wire stage, sample wait and optionally return an owned polling
 * token in token. Serial transport only samples before submission.
 * RU: Перед обменом проверить wait и выдать в token собственный токен опроса,
 * если доступен мультиплексор. Последовательный транспорт проверяется до отправки. */
StyxRpcIoOutcome styxrpc_vfs_control_begin(StyxRpcVfsWait *wait, int multiplexed, StyxCancel **token);


/* EN: Consume optional token and classify the copied result. Use rejected on
 * ordinary failure. For mutating requests preserve uncertainty over cancellation.
 * RU: Поглотить token и разобрать копию result. При отказе выдать rejected.
 * Для изменений сохранять неопределённость даже при отмене. */
StyxRpcIoOutcome styxrpc_vfs_control_end(StyxRpcVfsWait *wait, StyxCancel *token,
    const StyxTransfer *result, int mutating, StyxRpcIoOutcome rejected);


/* EN: Walk a validated wire path, borrowing session and wait. Set out to an
 * owned unopened fid on success, with caller responsible for retirement.
 * RU: Пройти проверенный путь, заимствуя session и wait. При успехе передать
 * в out собственный неоткрытый fid, который закрывает вызывающий. */
StyxRpcIoOutcome styxrpc_vfs_control_walk(StyxClient *session, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out);


/* EN: Read metadata from the borrowed fid under wait. Publish meta on success
 * only. On definite failure the caller chooses rejected.
 * RU: Прочитать метаданные заимствованного fid с wait. Выдать meta только при
 * успехе. Код определённого отказа задаёт вызывающий в rejected. */
StyxRpcIoOutcome styxrpc_vfs_control_stat(StyxFid *fid, StyxMeta *meta,
    StyxRpcVfsWait *wait, StyxRpcIoOutcome rejected);


/* EN: Open the borrowed fid in mode under wait; on failure callers retire it.
 * RU: Открыть заимствованный fid с mode и wait. При отказе его закрывает вызывающий. */
StyxRpcIoOutcome styxrpc_vfs_control_open(StyxFid *fid, StyxRpcFileMode mode, StyxRpcVfsWait *wait);



/* EN: Check a nonempty relative path outside reserved service components.
 * RU: Проверить непустой относительный путь вне служебных компонентов. */
int styxrpc_vfs_visible_path(const char *path);
/* EN: Open a protocol-selected service path from the attached root.
 * RU: Открыть выбранный протоколом служебный путь от корня подключения. */
StyxRpcIoOutcome styxrpc_vfs_exchange_open(struct StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file);
/* EN: Borrow session/path/wait and publish one owned directory fid on success.
 * Empty path selects the attached root.
 * RU: Заимствовать session/path/wait и выдать свой fid каталога при успехе.
 * Пустой путь выбирает корень подключения. */
StyxRpcIoOutcome styxrpc_vfs_open_directory_fid(StyxClient *session, const char *path,
    StyxRpcVfsWait *wait, StyxFid **out);

/* EN/RU: Identify hidden metadata. / Определить скрытые метаданные. */
int styxrpc_vfs_service_meta(const StyxMeta *);

/* EN/RU: Map a native transfer outcome. / Сопоставить результат передачи. */
StyxRpcIoOutcome styxrpc_vfs_transfer_outcome(const StyxTransfer *, size_t);


#endif
