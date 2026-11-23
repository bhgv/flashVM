/* Copyright (c) 2026 bhgv
 * EN: Immutable native routes for local Dis channel proxies.
 * RU: Неизменяемые нативные маршруты локальных прокси каналов Dis.
 */
#ifndef DVM_CHANNEL_ROUTE_H
#define DVM_CHANNEL_ROUTE_H
#include "channel_proxy.h"

/* EN: Create a provider-side route, retaining context and following its cancellation.
 * RU: Создать маршрут провайдера с удержанием контекста и наследованием его отмены. */
DVM_ChannelRoute *dvm_channel_route_create(void);

/* [GC:] EN: Acquire the canonical proxy for a remote key, optionally registering
 * the borrowed candidate first. Return an owned local object or NULL.
 * RU: Удержать канонический прокси ключа, при наличии зарегистрировав
 * заимствованный candidate. Вернуть свою локальную ссылку либо NULL. */
struct Channel *dvm_channel_route_proxy(DVM_ChannelRoute *route, const DVM_ChannelSchema *schema,
    struct Channel *candidate);

/* EN: Borrow the resident's route for the exact channel message cancellation scope.
 * RU: Заимствовать маршрут потока для точной области отмены сообщения. */
DVM_ChannelRoute *dvm_channel_route_for_message(DVM_Cancel *scope);

/* EN: Resolve a token in this resident's active or inherited routes. Outputs are borrowed.
 * RU: Найти токен в текущем или унаследованном маршруте. Результаты заимствованы. */
const DVM_ChannelSchema *dvm_channel_route_lookup(uint32_t token, DVM_Provider **provider, DVM_Cancel **cancel);


/* EN: Resolve a returned token strictly within the active function invocation.
 * RU: Найти возвращаемый токен строго в текущем вызове функции. */
const DVM_ChannelSchema *dvm_channel_route_result(uint32_t token);

#endif
