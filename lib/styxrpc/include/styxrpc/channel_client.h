/* Copyright (c) 2026 bhgv
 * EN: Hosted scoped calls with live results pinned to their issuing Styx fid.
 * RU: Контекстные вызовы с живыми результатами, привязанными к выдавшему fid Styx.
 */
#ifndef STYXRPC_CHANNEL_CLIENT_H
#define STYXRPC_CHANNEL_CLIENT_H
#include "styxrpc/channel_call.h"
#include "styxrpc/managed_client.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct StyxRpcChannelLease StyxRpcChannelLease; /* Serialized owner of one issuing fid and module. */

typedef struct StyxRpcLeasedReply
{
    FVM_SRecord result;           /* Validated result record borrowing the reply buffer. */
    StyxRpcChannelRoutes routes;  /* Distinct result routes in first-occurrence order. */
    StyxRpcChannelLease *lease;   /* One owned reference, or NULL when no channel was returned. */
} StyxRpcLeasedReply;


/* EN: Invoke a scoped channel profile once and retain all non-nil result channels.
 * RU: Однократно выполнить контекстный вызов и удерживать все непустые каналы результата. */
StyxRpcOutcome styxrpc_channel_invoke(const StyxRpcManagedTarget *target, const void *arguments,
    size_t size, void *reply, size_t capacity, StyxRpcLeasedReply *result);


/* EN: Retain one additional serialized reference. Return 1 on success, 0 on overflow or NULL.
 * RU: Удержать дополнительную ссылку на одном владельце. Вернуть 1 или 0 при переполнении/NULL. */
int styxrpc_channel_lease_retain(StyxRpcChannelLease *lease);


/* EN: Release a reference. Last release clunks the function, or defers cleanup if requested.
 * RU: Снять ссылку. Последняя закрывает fid функции, при deferred закрытие откладывается. */
void styxrpc_channel_lease_release(StyxRpcChannelLease *lease, int deferred);


/* EN: Execute guards within the token of route_index on the exact leased module.
 * RU: Выполнить ветки с токеном route_index на точном арендованном модуле. */
StyxRpcStatus styxrpc_channel_lease_exchange(StyxRpcChannelLease *lease, size_t route_index,
    const StyxRpcSelection *selection, unsigned timeout_ms, void *scratch, size_t scratch_size,
    void *request, size_t request_capacity, void *reply, size_t reply_capacity, StyxRpcChannelResult *result);

/* EN: Admit current-bus channels from a confirmed typed receive on route_index.
 * Borrow bytes for validation only. On failure the lease stays unchanged.
 * RU: Принять каналы текущей шины из подтверждённого приёма на route_index.
 * Байты нужны только для проверки. Ошибка сохраняет прежнюю аренду. */
StyxRpcStatus styxrpc_channel_lease_import_element(StyxRpcChannelLease *lease, size_t route_index,
    const void *bytes, size_t size);


/* EN: View the checked route at index until lease release. Return NULL outside the admitted table.
 * RU: Получить проверенный маршрут index до освобождения аренды. NULL вне таблицы. */
const StyxRpcChannelRoute *styxrpc_channel_lease_route(const StyxRpcChannelLease *lease, size_t index);

/* EN: Admit a module grant the host authenticated for this exact live lease.
 * It will use the pinned module. Failure leaves lease and index unchanged.
 * RU: Принять полномочие модуля, проверенное хостом для точной живой аренды.
 * Используется удерживаемый модуль. Ошибка сохраняет аренду и index. */
StyxRpcStatus styxrpc_channel_lease_admit_module(StyxRpcChannelLease *lease,
    const StyxRpcChannelRoute *route, size_t *index);


/* EN: Import a typed reverse message delivered for lease, authenticating each
 * module token on the pinned module before changing the route table.
 * RU: Принять типизированное встречное сообщение для lease, проверив каждый
 * токен на удерживаемом модуле до изменения таблицы маршрутов. */
StyxRpcStatus styxrpc_channel_lease_import_message(StyxRpcChannelLease *lease,
    const char *shape, const void *bytes, size_t size);

#ifdef __cplusplus
}
#endif
#endif
