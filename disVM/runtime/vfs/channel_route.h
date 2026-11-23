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

/* EN: Register one proxy token with its remote key and type before publication.
 * RU: Зарегистрировать токен прокси с сетевым ключом и типом до публикации. */
int dvm_channel_route_add(DVM_ChannelRoute *route, uint32_t token, const DVM_ChannelSchema *schema);

/* EN: Resolve a token in this resident's active or inherited routes. Outputs are borrowed.
 * RU: Найти токен в текущем или унаследованном маршруте. Результаты заимствованы. */
const DVM_ChannelSchema *dvm_channel_route_lookup(uint32_t token, DVM_Provider **provider, DVM_Cancel **cancel);


/* EN: Resolve a returned token strictly within the active function invocation.
 * RU: Найти возвращаемый токен строго в текущем вызове функции. */
const DVM_ChannelSchema *dvm_channel_route_result(uint32_t token);

#endif
