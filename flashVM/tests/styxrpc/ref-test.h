/* Copyright (c) 2026 bhgv
 * EN: Flat ref acceptance roles for the shared FlashVM test host.
 * RU: Роли приёмки плоских ref для общего тестового хоста FlashVM.
 */
#ifndef FVM_TEST_STYXRPC_REF_H
#define FVM_TEST_STYXRPC_REF_H
#include "flashvm/styxrpc_managed.h"


/* EN: Consume fd and publish the AVM1 ref callbacks on the owner thread.
 * RU: Принять fd и опубликовать ref-функции AVM1 в потоке владельца. */
int test_refs_serve(FVM_Context *context, int fd);


/* EN: Verify native ref imports on the mounted target and disable it before return.
 * RU: Проверить импорты ref на смонтированном target и отключить его при возврате. */
int test_refs_call(FVM_Context *context, StyxRpcManagedTarget *target);
#endif
