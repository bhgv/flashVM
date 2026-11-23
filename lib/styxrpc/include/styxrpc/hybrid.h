/* Copyright (c) 2026 bhgv
 * EN: Owner-local tables and canonical Dis snapshot/patch handling for graph calls.
 * RU: Локальные таблицы и штатные снимки и патчи Dis для графовых вызовов.
 */
#ifndef STYXRPC_HYBRID_H
#define STYXRPC_HYBRID_H
#include "styxrpc/managed.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_HYBRID_BIG_TEXT 21U /* Signed decimal big including the terminal NUL. */

typedef struct Fht StyxRpcHybrid; /* Owned table storage, serialized by its owner. */

typedef enum StyxRpcHybridOperation
{
    STYXRPC_HYBRID_GET,    /* Read one key into value. */
    STYXRPC_HYBRID_SET,    /* Insert or replace one key using value. */
    STYXRPC_HYBRID_DELETE, /* Remove one key. */
    STYXRPC_HYBRID_LENGTH, /* Report the dense prefix. */
    STYXRPC_HYBRID_COUNT   /* Report the total number of keys. */
} StyxRpcHybridOperation;

typedef struct StyxRpcHybridKey
{
    int text;          /* 0 numeric, 1 UTF-8 text. */
    int64_t index;     /* Numeric key, with negative indices relative to length. */
    const char *bytes; /* Borrowed UTF-8, including embedded zero bytes. */
    size_t size;       /* Exact text length. */
} StyxRpcHybridKey;

typedef struct StyxRpcHybridRequest
{
    StyxRpcHybridOperation operation; /* Selected operation. */
    StyxRpcHybridKey key;             /* Borrowed key for GET, SET and DELETE. */
    int64_t value;                    /* GET output or SET input. */
    uint64_t count;                   /* LENGTH or COUNT output. */
} StyxRpcHybridRequest;


/* EN: Open an owned table from a complete snapshot, or pass NULL and zero for an empty table.
 * RU: Открыть собственную таблицу из полного снимка, либо передать NULL и ноль для пустой. */
StyxRpcHybrid *styxrpc_hybrid_open(const void *snapshot, size_t size);


/* EN: Release owned storage; NULL is permitted.
 * RU: Освободить собственное хранилище; допускается NULL. */
void styxrpc_hybrid_close(StyxRpcHybrid *table);


/* EN: Run one local operation, returning 1 on completion and 0 on refusal.
 * RU: Выполнить одну локальную операцию, вернуть 1 при успехе или 0 при отказе. */
int styxrpc_hybrid_apply(StyxRpcHybrid *table, StyxRpcHybridRequest *request);


/* EN: Write a complete snapshot with ordered numeric and text keys; return 1 on success.
 * RU: Записать полный снимок с упорядоченными ключами; вернуть 1 при успехе. */
int styxrpc_hybrid_snapshot(const StyxRpcHybrid *table, void *bytes, size_t capacity, size_t *used);


/* EN: Write one canonical patch relative to the baseline snapshot; set used to zero on equality.
 * RU: Записать штатный патч относительно исходного снимка; при равенстве used равен нулю. */
int styxrpc_hybrid_diff(const StyxRpcHybrid *table, const void *baseline, size_t size,
    void *bytes, size_t capacity, size_t *used);


/* EN: Apply a complete patch to a private copy of the current table; return owned storage.
 * RU: Применить полный патч к частной копии текущей таблицы; вернуть собственное хранилище. */
StyxRpcHybrid *styxrpc_hybrid_stage(const StyxRpcHybrid *table, const void *patch, size_t size);


/* EN: Parse exact signed decimal big from a borrowed counted span; return 1 or 0.
 * RU: Прочитать точный знаковый big из десятичной строки с длиной; вернуть 1 или 0. */
int styxrpc_hybrid_big_parse(const char *bytes, size_t size, int64_t *value);


/* EN: Write canonical decimal big into capacity >=21; return length without NUL or zero.
 * RU: Записать десятичный big в буфер от 21 байта; вернуть длину без NUL или ноль. */
size_t styxrpc_hybrid_big_format(int64_t value, char *bytes, size_t capacity);

#ifdef __cplusplus
}
#endif
#endif
