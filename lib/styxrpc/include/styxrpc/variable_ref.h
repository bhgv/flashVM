/* Copyright (c) 2026 bhgv
 * EN: Session-relative descriptors for leased live Int32 variables.
 * RU: Дескрипторы живых переменных Int32 относительно сессии.
 */
#ifndef STYXRPC_VARIABLE_REF_H
#define STYXRPC_VARIABLE_REF_H
#include "styxrpc/program.h"
#include "styxrpc/vfs_client.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STYXRPC_INT_REF_SIZE 8U /* Exact length of the version-one payload. */
#define STYXRPC_INT_REF_FIRST_PID UINT32_C(0x80000000) /* First temporary PID. */

typedef struct StyxRpcIntRef
{
    uint32_t pid; /* Nonzero temporary PID on the issuer's attached session. */
    int writable; /* Exact 0 (read-only) or 1 (read/write). */
} StyxRpcIntRef;


/* EN: Encode ref into capacity bytes. Return 1 on success, preserving output on failure.
 * RU: Закодировать ref в буфер capacity байт. При успехе вернуть 1, при ошибке сохранить буфер. */
int styxrpc_int_ref_encode(const StyxRpcIntRef *ref, void *bytes, size_t capacity);


/* EN: Decode exactly one payload. Return 1 on success and preserve out on failure.
 * RU: Декодировать точный payload. При успехе вернуть 1, при ошибке сохранить out. */
int styxrpc_int_ref_decode(const void *bytes, size_t size, StyxRpcIntRef *out);


/* EN: Open an exact live cell on the issuer's attached session. The caller owns the returned fid.
 * RU: Открыть точную живую ячейку в сессии владельца. Вызывающий владеет возвращённым fid. */
StyxRpcIoOutcome styxrpc_int_ref_open(struct StyxClient *issuer, const StyxRpcIntRef *ref, StyxRpcVfsFile **out);


/* EN: Publish one temporary process cell. Borrow callbacks until revoke or server teardown.
 * RU: Опубликовать временную ячейку процесса. Заимствовать колбэки до отзыва или остановки сервера. */
int styxrpc_int_ref_grant(StyxRpcServer *server, const StyxIntVariable *variable,
    int writable, StyxRpcIntRef *out);


/* EN: Revoke the exact issued cell and invalidate already opened fids. Return 1 on first revocation.
 * RU: Отозвать выданную ячейку вместе с уже открытыми fid. При первом отзыве вернуть 1. */
int styxrpc_int_ref_revoke(StyxRpcServer *server, const StyxRpcIntRef *ref);

#ifdef __cplusplus
}
#endif
#endif
