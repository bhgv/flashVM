/* Copyright (c) 2026 bhgv
 * EN: Public bridge acceptance against real scoped UniBus exports.
 * RU: Проверка открытого моста с настоящими контекстными экспортами UniBus.
 */
#ifndef FVM_STYXRPC_CHANNEL_TEST_H
#define FVM_STYXRPC_CHANNEL_TEST_H
#include "flashvm/styxrpc_channel.h"
#include "flashvm/gc_roots.h"


/* EN: Mount live channel results and exercise script operations on the FlashVM owner.
 * RU: Принять живые каналы и выполнить методы на владельце FlashVM. */
int test_channel_call(FVM_Context *context, StyxRpcManagedTarget *target);


/* EN: Serve seeded and synchronous channels on the FlashVM dispatcher.
 * RU: Обслуживать заполненные и синхронные каналы на диспетчере FlashVM. */
int test_channel_serve(FVM_Context *context, int fd);


/* EN: Call Dis with typed queues published through bus on the reverse session.
 * RU: Вызвать Dis с типизированными очередями, выданными во встречной сессии. */
int test_channel_args(FVM_Context *context, StyxRpcManagedTarget *target, StyxRpcArgumentBus *bus);


/* EN: Serve Dis-owned channel arguments on the context owner using the connected fd.
 * RU: Обслужить аргументы-каналы Dis у владельца контекста через подключённый fd. */
int test_channel_incoming(FVM_Context *context, int fd);


/* EN: Fill length bytes of host test storage with OS entropy, returning 1 on success.
 * RU: Заполнить length байт тестового буфера энтропией ОС. 1 при успехе. */
int test_channel_entropy(void *unused, void *bytes, size_t length);


/* EN: Receive and root one nested message, checking aliases and nil at positions 0–2.
 * RU: Принять и удержать вложенное сообщение, сверив алиасы и nil в позициях 0–2. */
int test_channel_nested_message(FVM_Context *context, FVM_GCRootScope *roots,
    const FVM_Value *outer, FVM_Value *message, FVM_Value *child);
#endif
