/* Copyright (c) 2026 bhgv
 * EN: Managed bridge acceptance roles for the common FlashVM host.
 * RU: Роли приёмки управляемого моста для общего хоста FlashVM.
 */
#ifndef FVM_TEST_STYXRPC_MANAGED_H
#define FVM_TEST_STYXRPC_MANAGED_H
#include "flashvm/styxrpc_managed.h"

#define TEST_MANAGED_PID 42U /* Reverse program directory selected by the caller. */


/* EN: Publish the managed AS function on the supplied owned raw fd.
 * RU: Опубликовать управляемую функцию AS по выданному fd. */
int test_managed_serve(FVM_Context *context, int fd);


/* EN: Verify an imported managed function over the mounted target, then clear its session.
 * RU: Проверить управляемый импорт по смонтированному target, затем обнулить его сессию. */
int test_managed_call(FVM_Context *context, StyxRpcManagedTarget *target);
#endif
