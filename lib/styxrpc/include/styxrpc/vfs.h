/* Copyright (c) 2026 bhgv
 * EN: Host-defined virtual files with stable node identities and per-fid cookies.
 * RU: Виртуальные файлы хоста с устойчивыми идентификаторами и состоянием на fid.
 */
#ifndef STYXRPC_VFS_H
#define STYXRPC_VFS_H
#include "styxrpc/rpc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_READ 1U  /* Open for positional reads. */
#define STYXRPC_WRITE 2U /* Open for positional writes. */

typedef struct StyxRpcEntry
{
    uint64_t id;      /* Stable logical node identity within the publication. */
    const char *name; /* Borrowed single path component, valid until the next enum call. */
    unsigned mode;   /* Unix permission bits, excluding the directory marker. */
    int directory;   /* Nonzero for a directory, zero for a byte file. */
} StyxRpcEntry;

typedef struct StyxRpcVfs
{
    /* EN: Return one immediate child, NULL at end. Keep a stable order for a cursor.
     * RU: Вернуть одного непосредственного потомка или NULL в конце. Сохранить порядок курсора. */
    const StyxRpcEntry *(*enumerate)(void *context, uint64_t directory, size_t index);
    /* EN: Open a stable node with selected access; set a private handle or return 0.
     * RU: Открыть устойчивый узел с указанным доступом и выдать своё состояние либо вернуть 0. */
    int (*open)(void *context, uint64_t node, unsigned access, void **cookie);
    /* EN: Acknowledge the exact transferred byte count, or return 0 on failure.
     * RU: Подтвердить точное число байт или вернуть 0 при ошибке. */
    int (*read)(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size);
    int (*write)(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size);
    void (*close)(void *cookie); /* Consumes one successful open, including on disconnect. */
} StyxRpcVfs;

typedef struct StyxRpcFile
{
    const StyxRpcVfs *ops; /* Borrowed immutable table through close, NULL when closed. */
    void *cookie;         /* State obtained at Topen, independent of other fids. */
    unsigned access;      /* Immutable open access bits. */
} StyxRpcFile;


/* EN: Obtain one child and check its component, returning MORE at end or INVALID on bad metadata.
 * RU: Получить и проверить одного потомка. MORE в конце, INVALID при неверных метаданных. */
StyxRpcStatus styxrpc_dir_entry(const StyxRpcVfs *ops, void *context, uint64_t directory,
    size_t index, StyxRpcEntry *entry);

/* EN: Bind an empty file state to node and access. Zero-initialize file before opening.
 * RU: Связать пустое состояние с узлом и доступом. Обнулить file перед открытием. */
StyxRpcStatus styxrpc_file_open(StyxRpcFile *file, const StyxRpcVfs *ops,
    void *context, uint64_t node, unsigned access);

/* EN: Dispatch one positional read through its exact open handle. Zero bytes means EOF.
 * RU: Выполнить позиционное чтение через точный открытый handle. Ноль байт означает EOF. */
StyxRpcStatus styxrpc_file_read(StyxRpcFile *file, uint64_t offset, void *bytes,
    size_t capacity, size_t *size);

/* EN: Dispatch one positional write without replay, returning the acknowledged count.
 * RU: Выполнить позиционную запись без повтора и вернуть подтверждённое число байт. */
StyxRpcStatus styxrpc_file_write(StyxRpcFile *file, uint64_t offset, const void *bytes,
    size_t count, size_t *size);

/* EN: Close a successful open once. A zeroed or closed file is harmless.
 * RU: Закрыть успешное открытие ровно раз. Пустой либо закрытый file допустим. */
void styxrpc_file_close(StyxRpcFile *file);

#ifdef __cplusplus
}
#endif
#endif
