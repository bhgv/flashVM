/* Copyright (c) 2026 bhgv
 * EN: Player-only registration of a credited terminal and its AVM1 object.
 * RU: Регистрация терминала и его объекта AVM1 в штатном плеере.
 */
#ifndef FLASHVM_STYXRPC_PLAYER_TERMINAL_H
#define FLASHVM_STYXRPC_PLAYER_TERMINAL_H
#include "styxrpc/unibus_terminal.h"
#include "flashvm/value.h"


/* EN: Publish the terminal service on .unibus, borrowing terminal through server shutdown.
 * RU: Опубликовать терминал в .unibus, заимствуя terminal до закрытия сервера. */
int fvm_styxrpc_player_publish_terminal(StyxRpcServer *server, StyxRpcTerminal *terminal);


/* EN: Install styxTerminal on the fresh FlashVM owner and borrow terminal through context destroy.
 * RU: Установить styxTerminal у нового владельца FlashVM и заимствовать terminal до закрытия. */
FVM_Result fvm_styxrpc_player_terminal(FVM_Context *context, StyxRpcTerminal *terminal);

#endif
