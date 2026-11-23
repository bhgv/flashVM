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

#ifdef __cplusplus
}
#endif
#endif
