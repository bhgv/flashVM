/* Copyright (c) 2026 bhgv
 * EN: Portable descriptors and exact per-fid state for mounted native RPC.
 * RU: Переносимые описатели и состояние fid для нативных удалённых вызовов.
 */
#ifndef STYXRPC_RPC_H
#define STYXRPC_RPC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_INT_SIGNATURE UINT32_C(0xe67bf126) /* Limbo fn(int): int. */
#define STYXRPC_BIG_SIGNATURE UINT32_C(0x17f525e4) /* Limbo fn(big): big. */
#define STYXRPC_FRAME_SIZE 32U /* Bounded signed scalar S-record storage. */
#define STYXRPC_MAX_EXPORTS 8U /* Current mounted module profile limit. */

typedef enum StyxRpcStatus
{
    STYXRPC_OK = 0,       /* Complete or acknowledged progress. */
    STYXRPC_REMOTE_ERROR = 2, /* The provider acknowledged a failed call. */
    STYXRPC_MORE = 1,     /* Request incomplete or a reply is pending. */
    STYXRPC_INVALID = -1, /* Refused descriptor, malformed frame or offset. */
    STYXRPC_CANCELLED = -2, /* Stop during discovery or before sending an operation. */
    STYXRPC_CANCELLED_UNCERTAIN = -3 /* Channel stop after a write; peer effects may exist. */
} StyxRpcStatus;


/* EN: Return nonzero to stop an exchange on the calling thread.
 * RU: Вернуть ненулевое значение для остановки обмена в вызывающем потоке. */
typedef int (*StyxRpcCallStop)(void *state);


/* EN: Return 1 on success and set result. Return zero for a remote application error.
 * RU: Вернуть 1 при успехе и заполнить result, ноль при ошибке исполнителя. */
typedef int (*StyxRpcScalar)(void *context, int64_t argument, int64_t *result);

typedef struct StyxRpcExport
{
    const char *name;      /* Borrowed exact NUL-terminated function component. */
    uint32_t signature;   /* Existing compiled interface identity. */
    StyxRpcScalar invoke; /* Called once per complete accepted request. */
    void *context;        /* Borrowed application state through the last opened call. */
} StyxRpcExport;

typedef struct StyxRpcCall
{
    const StyxRpcExport *exported; /* Borrowed descriptor valid until this fid closes. */
    size_t staged;                /* Acknowledged request prefix. */
    size_t ready;                 /* Exact reply length, zero before completion. */
    int refused;                  /* Sticky framing failure on this fid. */
    unsigned char request[STYXRPC_FRAME_SIZE]; /* Fixed native request storage. */
    unsigned char reply[STYXRPC_FRAME_SIZE];   /* Fixed immutable reply storage. */
} StyxRpcCall;


/* EN: Encode a scalar request after checking the compiled signature and its width.
 * RU: Закодировать скалярный запрос после проверки сигнатуры и разрядности. */
StyxRpcStatus styxrpc_request(uint32_t signature, int64_t argument, void *bytes,
    size_t capacity, size_t *size);

/* EN: Decode one acknowledged complete scalar reply. Change value only on success.
 * RU: Декодировать полный подтверждённый ответ. Изменить value только при успехе. */
StyxRpcStatus styxrpc_reply(uint32_t signature, const void *bytes, size_t size, int64_t *value);


/* EN: Write .interface for count distinct exports into the borrowed buffer.
 * Invalid entries or insufficient space return INVALID and leave size zero.
 * RU: Записать .interface для count разных экспортов в заимствованный буфер.
 * Ошибка описателя или ёмкости возвращает INVALID и оставляет size равным нулю. */
StyxRpcStatus styxrpc_profile(const StyxRpcExport *exports, size_t count,
    void *bytes, size_t capacity, size_t *size);


/* EN: Initialize one caller-owned open function state from a borrowed descriptor.
 * RU: Инициализировать состояние одной открытой функции по заимствованному описателю. */
StyxRpcStatus styxrpc_open(StyxRpcCall *call, const StyxRpcExport *exported);


/* EN: Accept a contiguous write at offset. A complete valid frame invokes the
 * callback synchronously. Return MORE after a partial write. Serialize per fid.
 * RU: Принять очередную запись по смещению offset. Полный кадр вызывает обработчик
 * синхронно. Для части вернуть MORE. Операции одного fid сериализует вызывающий. */
StyxRpcStatus styxrpc_write(StyxRpcCall *call, size_t offset, const void *bytes, size_t size);


/* EN: Copy a positional reply prefix and return OK with zero bytes at EOF.
 * Before a complete request return MORE. No callback or allocation.
 * RU: Скопировать часть ответа, вернуть OK и ноль байт в EOF.
 * До приёма полного запроса вернуть MORE. Без колбэка и выделения памяти. */
StyxRpcStatus styxrpc_read(const StyxRpcCall *call, size_t offset, void *bytes,
    size_t capacity, size_t *size);


#ifdef __cplusplus
}
#endif
#endif
