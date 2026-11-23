/* Copyright (c) 2026 bhgv
 * EN: Public bridge acceptance against real scoped UniBus exports.
 * RU: Проверка открытого моста с настоящими контекстными экспортами UniBus.
 */
#ifndef FVM_STYXRPC_CHANNEL_TEST_H
#define FVM_STYXRPC_CHANNEL_TEST_H
#include "flashvm/styxrpc_channel.h"


/* EN: Mount live channel results and exercise script operations on the FlashVM owner.
 * RU: Принять живые каналы и выполнить методы на владельце FlashVM. */
int test_channel_call(FVM_Context *context, StyxRpcManagedTarget *target);


/* EN: Serve seeded and synchronous channels on the FlashVM dispatcher.
 * RU: Обслуживать заполненные и синхронные каналы на диспетчере FlashVM. */
int test_channel_serve(FVM_Context *context, int fd);


/* EN: Call Dis with typed queues published through bus on the reverse session.
 * RU: Вызвать Dis с типизированными очередями, выданными во встречной сессии. */
int test_channel_args(FVM_Context *context, StyxRpcManagedTarget *target, StyxRpcArgumentBus *bus);
#endif
