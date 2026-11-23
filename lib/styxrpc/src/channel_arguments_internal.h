/* Copyright (c) 2026 bhgv
 * EN: Connect the checked scoped request to its exact staged reverse grants.
 * RU: Связать проверенный контекстный запрос с подготовленными встречными ключами.
 */
#ifndef STYXRPC_CHANNEL_ARGUMENTS_INTERNAL_H
#define STYXRPC_CHANNEL_ARGUMENTS_INTERNAL_H
#include "styxrpc/channel_arguments.h"


/* EN: Check PID, every typed key and alias, then publish the complete scope.
 * RU: Проверить PID, ключи с типами и алиасы, затем опубликовать полную область. */
int styxrpc_argument_scope_match(StyxRpcArgumentScope *scope, const StyxRpcChannelCall *call);

#endif
