/* Copyright (c) 2026 bhgv
 * EN: Caller-owned staging, scheduler callbacks and completion for one channel fid.
 * RU: Буферы приложения, колбэки планировщика и завершение одного fid канала.
 */
#ifndef STYXRPC_CHANNEL_FID_H
#define STYXRPC_CHANNEL_FID_H
#include "styxrpc/channel.h"

#ifdef __cplusplus
extern "C" {
#endif


/* EN: Schedule the complete admitted selection under a host-owned rendezvous policy.
 * RU: Запланировать полную допущенную операцию по правилам синхронизации хоста. */
typedef struct StyxRpcChannelFid
{
    const StyxRpcChannelGrant *grants; /* Borrowed immutable table through close. */
    size_t grant_count;              /* Number of admitted keys. */
    unsigned char *request;          /* Borrowed writable storage through close. */
    size_t request_capacity;          /* In bytes. */
    unsigned char *reply;            /* Disjoint borrowed storage through close. */
    size_t reply_capacity;            /* In bytes. */
    size_t staged;                    /* Bytes accepted in the original frame. */
    size_t ready;                     /* Published reply extent, zero before completion. */
    int pending;                      /* Scheduling accepted exactly once. */
    int refused;                      /* Sticky malformed request or failed schedule. */
    StyxRpcSelection selection;       /* Ordered alternatives, borrows the request. */
    int (*admit)(void *context, struct StyxRpcChannelFid *fid); /* Borrow fid and return 1 to accept. */
    void (*cancel)(void *context, struct StyxRpcChannelFid *fid); /* Remove unresolved work synchronously. */
    void *context;                    /* Borrowed scheduler context. */
} StyxRpcChannelFid;


/* EN: Feed one contiguous request prefix and call admit once on complete validated input.
 * Initialize fid to zero then supply grants, buffers, callbacks and context. Serialize all operations.
 * RU: Принять очередной фрагмент и один раз вызвать admit после полной проверки.
 * Обнулите fid, задайте grants, буферы, колбэки и context. Операции сериализуются. */
StyxRpcStatus styxrpc_channel_fid_write(StyxRpcChannelFid *fid, size_t offset, const void *bytes, size_t size);


/* EN: Publish one selected guard and optional received value. Retry encoding on INVALID only.
 * RU: Опубликовать выбранную ветку и возможный элемент. При INVALID можно повторить кодирование. */
StyxRpcStatus styxrpc_channel_fid_finish(StyxRpcChannelFid *fid, unsigned selected, const void *value, size_t size);


/* EN: Read the immutable reply; MORE before completion, OK and zero bytes after EOF.
 * RU: Читать неизменяемый ответ. MORE до завершения, OK и ноль байт после EOF. */
StyxRpcStatus styxrpc_channel_fid_read(const StyxRpcChannelFid *fid, size_t offset,
    void *bytes, size_t capacity, size_t *size);


/* EN: Retire one fid; cancel unresolved work before its request storage can be freed.
 * RU: Закрыть один fid, отменив ожидающую работу до освобождения буфера запроса. */
void styxrpc_channel_fid_close(StyxRpcChannelFid *fid);

#ifdef __cplusplus
}
#endif
#endif
