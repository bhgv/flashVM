/* Copyright (c) 2026 bhgv
 * EN: Callback-backed VFS projection of the Limbo hybrid big-valued table.
 * RU: Представление гибридной таблицы Limbo со значениями big через колбэки VFS.
 */
#ifndef STYXRPC_BIGTABLE_H
#define STYXRPC_BIGTABLE_H
#include "styxrpc/program.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_TABLE_KEY_LIMIT 255U /* Maximum counted UTF-8 key bytes. */

typedef enum StyxRpcBigOperation
{
    STYXRPC_BIG_LENGTH, /* Dense positional prefix length. */
    STYXRPC_BIG_COUNT,  /* Count including sparse and text keys. */
    STYXRPC_BIG_SELECT, /* Check presence and resolve a relative numeric key. */
    STYXRPC_BIG_GET,    /* Read an existing value. */
    STYXRPC_BIG_UPDATE, /* Replace an existing value. */
    STYXRPC_BIG_INSERT, /* Insert at a dense position. */
    STYXRPC_BIG_PUSH,   /* Append to the dense part. */
    STYXRPC_BIG_POP,    /* Remove and return the last dense element. */
    STYXRPC_BIG_ADD     /* Add a key only if absent. */
} StyxRpcBigOperation;

typedef struct StyxRpcBigKey
{
    int text;           /* Zero for signed numeric key, one for counted UTF-8. */
    int64_t index;      /* Negative numeric SELECT keys are relative to the dense end. */
    const char *bytes;  /* Borrowed text key, including embedded zero bytes. */
    size_t size;        /* Exact text length in bytes. */
} StyxRpcBigKey;

typedef struct StyxRpcBigRequest
{
    StyxRpcBigOperation operation; /* Selects a single atomic application operation. */
    StyxRpcBigKey key;              /* Borrowed key. SELECT resolves numeric keys in place. */
    int64_t value;                  /* Big value input or output, according to operation. */
    int64_t position;               /* Dense insertion position. */
    uint64_t count;                 /* Length/count output. */
} StyxRpcBigRequest;

typedef struct StyxRpcBigTable
{
    void *context; /* Borrowed through all opened files. */
    int (*apply)(void *context, StyxRpcBigRequest *request); /* 1 on success, 0 without effects on refusal. */
    int writable;  /* Zero removes mutation files and denies changes through .val. */
} StyxRpcBigTable;


/* EN: Register one complete .var table directory on the dispatch owner.
 * The descriptor is borrowed until server destruction; callbacks serialize with native users.
 * SELECT of an existing numeric key must return its canonical nonnegative index.
 * RU: Зарегистрировать каталог таблицы .var в потоке диспетчера.
 * Описатель живёт до закрытия сервера, колбэки согласуются с нативными читателями.
 * SELECT существующего числового ключа возвращает его канонический неотрицательный индекс. */
int styxrpc_program_bigtable(StyxRpcPrograms *programs, uint32_t pid, const char *name, const StyxRpcBigTable *table);

#ifdef __cplusplus
}
#endif
#endif
