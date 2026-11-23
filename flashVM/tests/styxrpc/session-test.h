/* Copyright (c) 2026 bhgv
 * EN: Launch and VFS acceptance roles for the FlashVM host.
 * RU: Роли приёмки запуска и VFS для хоста FlashVM.
 */
#ifndef FVM_TEST_STYXRPC_SESSION_H
#define FVM_TEST_STYXRPC_SESSION_H
#include "flashvm/styxrpc_session.h"
#include "styxrpc/program.h"

typedef struct TestLaunchContext
{
    int32_t value;        /* Count written by the launched guest. */
    unsigned char output; /* Last write to descriptor 7. */
    int parked;           /* Atomic cancellation checkpoint. */
} TestLaunchContext;


/* EN: Publish the test caller's fixed reverse assets before owning the dispatcher.
 * RU: Опубликовать фиксированные встречные ресурсы до запуска диспетчера. */
int test_launch_resources(StyxRpcPrograms *programs, TestLaunchContext *state);


/* EN: Exercise named startup and confirmed receipts on the attached client.
 * RU: Проверить именованный запуск и подтверждённые квитанции на клиенте. */
int test_launch_call(FVM_Context *context, FVM_StyxRpcSession *binding, struct StyxClient *client,
    TestLaunchContext *state);


/* EN: Consume fd, then exercise bounded ActionScript byte files and teardown.
 * RU: Принять fd и проверить ограниченное байтовое I/O ActionScript и закрытие. */
int test_vfs_call(FVM_Context *context, FVM_StyxRpcSession *binding, int fd);
#endif
