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
    STYXRPC_IO_UNCERTAIN = 3  /* A data operation lacked a reliable acknowledgement. */
} StyxRpcIoOutcome;

typedef struct StyxRpcFileInfo
{
    uint64_t length;  /* Advertised length in bytes. */
    unsigned mode;    /* Advertised native mode, including Styx type bits. */
    int directory;   /* Nonzero for a directory. */
} StyxRpcFileInfo;

typedef struct StyxFid StyxRpcVfsFile; /* Owns one opened fid and retains its attached client. */
typedef int (*StyxRpcVfsVisitor)(void *state, const char *name, const StyxRpcFileInfo *info); /* Borrowed entry, 1 to proceed. */

typedef enum StyxRpcFileMode
{
    STYXRPC_FILE_READ = 0,   /* Open an existing file for reading. */
    STYXRPC_FILE_WRITE = 1,  /* Open an existing file for writing at caller-selected offsets. */
    STYXRPC_FILE_RDWR = 2    /* Open an existing file for both directions. */
} StyxRpcFileMode;


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


/* EN: Query one held file independent of path replacement.
 * RU: Получить метаданные открытого файла независимо от замены пути. */
StyxRpcIoOutcome styxrpc_vfs_file_stat(StyxRpcVfsFile *file, StyxRpcFileInfo *info);


/* EN: Transfer a confirmed prefix on file at offset. The caller owns both cursor and buffer.
 * RU: Передать подтверждённый префикс через file со смещения offset. Курсор и буфер у вызывающего. */
StyxRpcIoOutcome styxrpc_vfs_file_read(StyxRpcVfsFile *file, uint64_t offset, void *bytes, size_t size, size_t *used);
StyxRpcIoOutcome styxrpc_vfs_file_write(StyxRpcVfsFile *file, uint64_t offset, const void *bytes, size_t size, size_t *used);


/* EN: Consume file, optionally queue the clunk for safe owner cleanup.
 * RU: Освободить file, при необходимости отложив clunk до безопасной очистки владельца. */
void styxrpc_vfs_close(StyxRpcVfsFile *file, int deferred);


/* EN: Enumerate up to limit entries after skip on an opened directory. Empty path means root.
 * RU: Перечислить до limit записей открытого каталога после skip. Пустой путь означает корень. */
StyxRpcIoOutcome styxrpc_vfs_list(struct StyxClient *session, const char *path, size_t skip, size_t limit,
    StyxRpcVfsVisitor visit, void *state);


/* EN: Read or replace a whole native .var int at offset zero, with one transfer.
 * RU: Прочитать либо заменить весь int из .var со смещения ноль одной операцией. */
StyxRpcIoOutcome styxrpc_vfs_int_read(StyxRpcVfsFile *file, int32_t *value);
StyxRpcIoOutcome styxrpc_vfs_int_write(StyxRpcVfsFile *file, int32_t value);

#ifdef __cplusplus
}
#endif
#endif
