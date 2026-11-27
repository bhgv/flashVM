/* Copyright (c) 2026 bhgv
 * EN: Bounded positional byte I/O over an already mounted Styx session.
 * RU: Ограниченное позиционное байтовое I/O по смонтированной сессии Styx.
 */
#ifndef STYXRPC_VFS_CLIENT_H
#define STYXRPC_VFS_CLIENT_H
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_VFS_PATH_MAX 1024U    /* Maximum relative path bytes. */
#define STYXRPC_VFS_TRANSFER_MAX 65536U /* Maximum bytes in one application operation. */
#define STYXRPC_VFS_OFFSET_MAX UINT64_C(0x7fffffffffffffff) /* Styx positional limit. */

struct StyxClient; /* Borrowed mounted session. */

typedef enum StyxRpcIoOutcome
{
    STYXRPC_IO_OK = 0,        /* Confirmed bytes, possibly short or EOF. */
    STYXRPC_IO_NOT_OPEN = 1,  /* Admission failed; no data operation attempted. */
    STYXRPC_IO_FAILED = 2,    /* Confirmed failure without uncertain bytes. */
    STYXRPC_IO_UNCERTAIN = 3, /* A data operation lacked a reliable acknowledgement. */
    STYXRPC_IO_CANCELLED = 4  /* Local wait cancelled; a submitted peer effect may have occurred. */
} StyxRpcIoOutcome;

typedef struct StyxRpcFileInfo
{
    uint64_t length;  /* Advertised length in bytes. */
    unsigned mode;    /* Advertised native mode, including Styx type bits. */
    int directory;   /* Nonzero for a directory. */
} StyxRpcFileInfo;

typedef struct StyxFid StyxRpcVfsFile; /* Owns one opened fid and retains its attached client. */
typedef int (*StyxRpcVfsVisitor)(void *state, const char *name, const StyxRpcFileInfo *info); /* Borrowed entry, 1 to proceed. */
typedef int (*StyxRpcVfsStop)(void *state); /* On the calling thread, return nonzero to stop. */

typedef enum StyxRpcFileMode
{
    STYXRPC_FILE_READ = 0,   /* Open an existing file for reading. */
    STYXRPC_FILE_WRITE = 1,  /* Open an existing file for writing at caller-selected offsets. */
    STYXRPC_FILE_RDWR = 2    /* Open an existing file for both directions. */
} StyxRpcFileMode;


/* EN: Identify one counted reserved component for ordinary path resolution.
 * RU: Распознать один служебный компонент с длиной при разрешении обычного пути. */
int styxrpc_vfs_service_name(const char *name, size_t size);


/* EN: Borrow session and path, publish metadata only on success.
 * RU: Заимствовать session и path, выдать метаданные только при успехе. */
StyxRpcIoOutcome styxrpc_vfs_stat(struct StyxClient *session, const char *path, StyxRpcFileInfo *info);


/* EN: Read a confirmed prefix from one exact opened file. No implicit retries.
 * RU: Прочитать подтверждённый префикс из одного открытого файла. Без повтора. */
StyxRpcIoOutcome styxrpc_vfs_read_at(struct StyxClient *session, const char *path,
    uint64_t offset, void *bytes, size_t size, size_t *used);


/* EN: Write confirmed consecutive suffixes on one fid. Never replay an uncertain write.
 * RU: Записать подтверждённые последовательные остатки на одном fid. Не повторять сомнительную запись. */
StyxRpcIoOutcome styxrpc_vfs_write_at(struct StyxClient *session, const char *path,
    uint64_t offset, const void *bytes, size_t size, size_t *used);


/* EN: Open one existing regular file on the attached tree; return an owned fid.
 * RU: Открыть существующий файл в смонтированном дереве и вернуть собственный fid. */
StyxRpcIoOutcome styxrpc_vfs_open(struct StyxClient *session, const char *path,
    StyxRpcFileMode mode, StyxRpcVfsFile **file);


/* EN: Create a new regular file through its existing visible parent. Open it
 * write-only on success. A failed request may report an uncertain outcome.
 * RU: Создать обычный файл через существующий видимый каталог. При успехе
 * открыть его для записи. При сбое результат может быть неопределённым. */
StyxRpcIoOutcome styxrpc_vfs_create(struct StyxClient *session, const char *path, StyxRpcVfsFile **file);


/* EN: Create one directory at a visible relative path using the peer's mode policy.
 * RU: Создать один каталог по видимому относительному пути с правами провайдера. */
StyxRpcIoOutcome styxrpc_vfs_mkdir(struct StyxClient *session, const char *path);


/* EN: Remove one visible relative entry. Never retry an uncertain removal.
 * RU: Удалить один видимый узел по относительному пути. Без повтора при неопределённости. */
StyxRpcIoOutcome styxrpc_vfs_remove(struct StyxClient *session, const char *path);


/* EN: Query one held file independent of path replacement.
 * RU: Получить метаданные открытого файла независимо от замены пути. */
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info);


/* EN: Like stat, polling optional stop(state) through Twalk and Tstat.
 * Publish no metadata on failure. Empty path addresses the attach root.
 * RU: Как stat, проверяя необязательный stop(state) во время Twalk и Tstat.
 * При ошибке не выдавать метаданные. Пустой путь обозначает корень. */
StyxRpcIoOutcome styxrpc_vfs_stat_until(struct StyxClient *session, const char *path, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state);


/* EN: Like file_stat, polling optional stop(state) on the caller during Tstat.
 * Borrow file, preserving it and info on cancellation.
 * RU: Как file_stat, с опросом stop(state) в потоке вызова при Tstat.
 * Заимствовать file, сохранять его и info при отмене. */
StyxRpcIoOutcome styxrpc_vfs_file_stat_until(StyxRpcVfsFile *file, StyxRpcFileInfo *info,
    StyxRpcVfsStop stop, void *state);


/* EN: Like open, polling optional stop(state) through all admission stages.
 * Transfer one owned exact fid only on acknowledgement and leave file unchanged on refusal.
 * RU: Как open, с опросом stop(state) на всех этапах допуска.
 * Выдать точный собственный fid при подтверждении, при отказе file не менять. */
StyxRpcIoOutcome styxrpc_vfs_open_until(struct StyxClient *session, const char *path, StyxRpcFileMode mode,
    StyxRpcVfsFile **file, StyxRpcVfsStop stop, void *state);


/* EN: Like create, polling optional stop(state) through parent validation and
 * Tcreate. A sent mutation without acknowledgement returns UNCERTAIN even on stop.
 * RU: Как create, с опросом stop(state) при проверке родителя и Tcreate.
 * Отправленное изменение без подтверждения возвращает UNCERTAIN даже при отмене. */
StyxRpcIoOutcome styxrpc_vfs_create_until(struct StyxClient *session, const char *path, StyxRpcVfsFile **file,
    StyxRpcVfsStop stop, void *state);


/* EN: Like mkdir, with the cancellation and uncertainty rules of create_until.
 * RU: Как mkdir, с правилами отмены и неопределённости из create_until. */
StyxRpcIoOutcome styxrpc_vfs_mkdir_until(struct StyxClient *session, const char *path,
    StyxRpcVfsStop stop, void *state);


/* EN: Like remove, polling optional stop(state) through path validation and
 * Tremove. Keep uncertainty if a sent remove lacks an acknowledgement.
 * RU: Как remove, с опросом stop(state) на проверке пути и Tremove.
 * Сохранять неопределённость отправленного удаления без подтверждения. */
StyxRpcIoOutcome styxrpc_vfs_remove_until(struct StyxClient *session, const char *path,
    StyxRpcVfsStop stop, void *state);


/* EN: Transfer a confirmed prefix on file at offset. The caller owns both cursor and buffer.
 * RU: Передать подтверждённый префикс через file со смещения offset. Курсор и буфер у вызывающего. */
StyxRpcIoOutcome styxrpc_vfs_file_read(StyxRpcVfsFile *file, uint64_t offset, void *bytes, size_t size, size_t *used);
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset, const void *bytes, size_t size, size_t *used);


/* EN: Read up to size bytes at offset on borrowed file. Sample optional
 * stop(state) before each chunk and while awaiting a multiplexed reply.
 * Always return the confirmed prefix length in used, including on cancellation.
 * Serialize I/O on file and close it after STYXRPC_IO_CANCELLED.
 * RU: Прочитать до size байтов со смещения offset на заимствованном file.
 * Проверять необязательный stop(state) перед фрагментами и при ожидании ответа
 * на мультиплексоре. Возвращать в used подтверждённую длину, в том числе при
 * отмене. Сериализовать I/O на file и закрыть его после STYXRPC_IO_CANCELLED. */
StyxRpcIoOutcome styxrpc_vfs_file_read_until(StyxRpcVfsFile *file, uint64_t offset, void *bytes,
    size_t size, size_t *used, StyxRpcVfsStop stop, void *state);


/* EN: Write up to size bytes at offset on file using read_until's stop contract.
 * Return the confirmed prefix in used. On cancellation the pending write may
 * have taken effect. Close the cancelled fid and avoid automatic replay.
 * RU: Записать до size байтов на file со смещения offset с отменой как у read_until.
 * Вернуть подтверждённую длину в used. Отменённая запись могла изменить данные.
 * Закрыть отменённый fid и избегать автоматического повтора. */
StyxRpcIoOutcome styxrpc_vfs_file_write_until(StyxRpcVfsFile *file, uint64_t offset, const void *bytes,
    size_t size, size_t *used, StyxRpcVfsStop stop, void *state);


/* EN: Consume file, optionally queue the clunk for safe owner cleanup.
 * RU: Освободить file, при необходимости отложив clunk до безопасной очистки владельца. */
void styxrpc_vfs_close(StyxRpcVfsFile *file, int deferred);


typedef struct StyxRpcVfsDir StyxRpcVfsDir; /* Open directory and its private cursor. */

/* EN/RU: Open an owned directory cursor. / Открыть собственный итератор каталога. */
StyxRpcIoOutcome styxrpc_vfs_dir_open(struct StyxClient *, const char *, StyxRpcVfsDir **);

/* EN: Visit one visible entry. Set found to zero at EOF. Names are borrowed during visit.
 * RU: Передать одну видимую запись. На EOF found равен нулю. Имя заимствуется на время visit. */
StyxRpcIoOutcome styxrpc_vfs_dir_next(StyxRpcVfsDir *, StyxRpcVfsVisitor, void *, int *found);

/* EN: Like dir_open, polling optional stop(state) across admission. Transfer
 * one owned cursor into out on acknowledgement only.
 * RU: Как dir_open, с опросом stop(state) при допуске. Выдать собственный
 * курсор в out только при подтверждении. */
StyxRpcIoOutcome styxrpc_vfs_dir_open_until(struct StyxClient *session, const char *path, StyxRpcVfsDir **out,
    StyxRpcVfsStop stop, void *state);


/* EN: Like dir_next, polling stop(stop_state) before advancing and during
 * directory reads. If stopped retire the exact fid and close the cursor
 * allocation with dir_close. found is zero and no visitor runs on cancellation.
 * RU: Как dir_next, с опросом stop(stop_state) до продвижения и при чтении.
 * При отмене освобождается fid, память курсора нужно закрыть через dir_close.
 * found равен нулю, обработчик при отмене не вызывается. */
StyxRpcIoOutcome styxrpc_vfs_dir_next_until(StyxRpcVfsDir *dir, StyxRpcVfsVisitor visit, void *state, int *found,
    StyxRpcVfsStop stop, void *stop_state);


/* EN: Discard buffered entries and restart on the same fid on the next read.
 * Caller serializes operations. NULL or a cancelled cursor returns NOT_OPEN.
 * RU: Сбросить буфер и начать следующий обход того же fid с нуля.
 * Вызывающий сериализует операции. NULL или отмена курсора дают NOT_OPEN. */
StyxRpcIoOutcome styxrpc_vfs_dir_rewind(StyxRpcVfsDir *dir);

/* EN/RU: Consume dir and optionally defer clunk. / Освободить dir и при необходимости отложить clunk. */
void styxrpc_vfs_dir_close(StyxRpcVfsDir *, int deferred);

/* EN: Enumerate up to limit entries after skip on an opened directory. Empty path means root.
 * RU: Перечислить до limit записей открытого каталога после skip. Пустой путь означает корень. */
StyxRpcIoOutcome styxrpc_vfs_list(struct StyxClient *session, const char *path, size_t skip, size_t limit,
    StyxRpcVfsVisitor visit, void *state);


/* EN: Open a host-selected raw .var cell, including one inside a scoped .prog.
 * RU: Открыть выбранную хостом ячейку .var, включая контекстную .prog. */
StyxRpcIoOutcome styxrpc_vfs_int_open(struct StyxClient *session, const char *path, int writable, StyxRpcVfsFile **out);


/* EN: As int_open, also copy Qid.path from the exact opened fid on success.
 * RU: Как int_open, дополнительно копировать Qid.path точного открытого fid при успехе. */
StyxRpcIoOutcome styxrpc_vfs_int_open_identified(struct StyxClient *session, const char *path,
    int writable, StyxRpcVfsFile **out, uint64_t *identity);


/* EN: Read or replace a whole native .var int at offset zero, with one transfer.
 * RU: Прочитать либо заменить весь int из .var со смещения ноль одной операцией. */
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value);
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value);

#ifdef __cplusplus
}
#endif
#endif
