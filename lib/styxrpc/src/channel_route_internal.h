/* Copyright (c) 2026 bhgv
 * EN: Share route validation between the portable codec and hosted exchange.
 * RU: Общая проверка маршрута переносимым кодеком и адаптером обмена.
 */
#ifndef STYXRPC_CHANNEL_ROUTE_INTERNAL_H
#define STYXRPC_CHANNEL_ROUTE_INTERNAL_H
#include "styxrpc/channel_route.h"


/* EN: Check the address and element schema in route. Return 1 for a canonical route.
 * RU: Проверить адрес и схему элемента route. Вернуть 1 для канонического маршрута. */
int styxrpc_route_valid(const StyxRpcChannelRoute *route);

struct StyxFid; /* Exact unopened module held by a function lease. */


/* EN: Present token on a temporary bus fid under module, without sending a guard.
 * RU: Предъявить token на временном fid шины модуля, не отправляя ветку. */
int styxrpc_channel_authenticate_bound(struct StyxFid *module, const unsigned char *token);


/* EN: Exchange using grants from one already admitted PID on reverse.
 * RU: Выполнить обмен с полномочиями одного принятого PID на reverse. */
StyxRpcStatus styxrpc_channel_exchange_pid_bound(struct StyxClient *reverse, uint32_t pid,
    const StyxRpcChannelGrant *grants, size_t count, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result);


/* EN: Exchange through the module held at admission, without a fresh module-name lookup.
 * RU: Выполнить обмен через удерживаемый модуль без нового поиска по имени. */
StyxRpcStatus styxrpc_channel_exchange_bound(struct StyxFid *module, const unsigned char *token,
    const StyxRpcChannelGrant *grants, size_t count, const StyxRpcSelection *selection,
    unsigned timeout_ms, void *scratch, size_t scratch_size, void *request, size_t request_capacity,
    void *reply, size_t reply_capacity, StyxRpcChannelResult *result);
#endif
