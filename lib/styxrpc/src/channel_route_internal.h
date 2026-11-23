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
#endif
