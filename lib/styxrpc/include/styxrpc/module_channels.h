/* Copyright (c) 2026 bhgv
 * EN: Multiplex C-owned result channel resources through one Styx module bus.
 * RU: Мультиплексирование возвращаемых каналов C через общую шину модуля Styx.
 */
#ifndef STYXRPC_MODULE_CHANNELS_H
#define STYXRPC_MODULE_CHANNELS_H
#include "styxrpc/channel_route.h"
#include "styxrpc/program.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcModuleChannel
{
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]; /* Exact binding for one issuance. */
    StyxRpcResource resource;                       /* Copied operation callbacks. */
    struct StyxRpcModuleOpen *opens;                /* Active authenticated operation handles. */
    unsigned state;                                /* Free, live or retiring. */
} StyxRpcModuleChannel;

typedef struct StyxRpcModuleChannels
{
    StyxRpcModuleChannel *slots;                /* Caller-owned array used until server shutdown. */
    size_t capacity;                            /* Maximum simultaneous leases. */
    uint32_t serial;                            /* Last issuance number for this bus. */
    int (*entropy)(void *, void *, size_t);     /* CSPRNG fill, returning 1 for all requested bytes. */
    void *entropy_context;                      /* Borrowed by the entropy callback. */
} StyxRpcModuleChannels;


/* EN: Initialize stable storage and an entropy callback for one module bus and session.
 * RU: Инициализировать память и колбэк случайных байтов для шины модуля в одной сессии. */
int styxrpc_module_channels_init(StyxRpcModuleChannels *channels, StyxRpcModuleChannel *slots,
    size_t capacity, int (*entropy)(void *, void *, size_t), void *context);


/* EN: Publish one resource with a fresh token; the issuing function fid owns its lifetime.
 * RU: Опубликовать ресурс с новым токеном. Его срок жизни принадлежит выдавшему fid функции. */
int styxrpc_module_channels_publish(StyxRpcModuleChannels *channels, const StyxRpcResource *resource,
    unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);


/* EN: Refuse further I/O and synchronously close the exact token's authenticated handles.
 * RU: Отклонить дальнейший ввод-вывод и синхронно закрыть дескрипторы точного токена. */
int styxrpc_module_channels_revoke(StyxRpcModuleChannels *channels,
    const unsigned char token[STYXRPC_CHANNEL_TOKEN_SIZE]);


/* EN: Return callbacks for the module's .unibus node, borrowing the initialized registry.
 * RU: Вернуть колбэки узла .unibus модуля с заимствованием инициализированного реестра. */
StyxRpcResource styxrpc_module_channels_resource(StyxRpcModuleChannels *channels);

#ifdef __cplusplus
}
#endif
#endif
