/* Copyright (c) 2026 bhgv
 * EN: Native FlashVM control owner for the Limbo lifecycle test.
 * RU: Нативный владелец плеера FlashVM для проверки управления из Limbo.
 */
#include "flashvm/styxrpc_playback.h"
#include "styxrpc/module.h"
#include "drivers/posix/fd.h"
#include <stdio.h>


/* EN: Return the input in the mixed server’s unrelated scalar method.
 * RU: Вернуть вход в отдельном скалярном методе смешанного сервера. */
static int identity(void *unused, int64_t input, int64_t *output)
{
    (void)unused;
    *output = input;
    return 1;
}


/* EN: Check the selected interpreter mode and record the attachment.
 * RU: Проверить режим интерпретатора и учесть подключение отладчика. */
static FVM_Result debugger(void *host, FVM_Context *context)
{
    FVM_ExecutionOptions mode;
    if (fvm_execution_options_init(&mode) != FVM_OK || fvm_execution_get_options(context, &mode) != FVM_OK ||
        mode.mode != FVM_EXECUTION_INTERPRETER)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ++*(int *)host;
    return FVM_OK;
}


/* EN: Append the application debugger count to the playback snapshot.
 * RU: Добавить счётчик подключений отладчика к снимку состояния. */
static int status(void *host, char *bytes, size_t capacity, size_t *used)
{
    int result = snprintf(bytes, capacity, "debug_attached=%d\n", *(int *)host);
    if (result < 0 || (size_t)result >= capacity)
    {
        return 0;
    }
    *used = (size_t)result;
    return 1;
}


/* EN: Serve the mixed tree on the native owner and verify the debugger callback.
 * RU: Обслужить смешанное дерево у владельца и проверить колбэк отладчика. */
int test_playback_serve(FVM_Runtime *runtime, int fd, int jit)
{
    int debug_calls = 0;
    FVM_StyxRpcPlaybackSpec playback_spec = {runtime, NULL, 4096,
        jit ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, &debug_calls, NULL, debugger, status};
    FVM_StyxRpcPlayback *playback = NULL;
    if (fvm_styxrpc_playback_create(&playback_spec, &playback) != FVM_OK)
    {
        return 0;
    }
    StyxRpcModuleEntry entry = {.kind = STYXRPC_MODULE_INT, .callback.integer = {"identity", STYXRPC_INT_SIGNATURE, identity, NULL}};
    StyxRpcModuleSpec module_spec = {"control-test.dis", &entry, 1, NULL, 0, NULL,
        fvm_styxrpc_playback_control(playback)};
    StyxRpcModule *module = NULL;
    StyxRpcServer *server = NULL;
    if (styxrpc_module_create(&module_spec, &module) == STYXRPC_OK)
    {
        server = styxrpc_server_create(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE),
            &styxrpc_module_tree, module);
        while (styxrpc_server_step(server))
        {
        }
    }
    int ok = server != NULL && debug_calls == 1;
    styxrpc_server_destroy(server);
    styxrpc_module_destroy(module);
    fvm_styxrpc_playback_destroy(playback);
    return ok;
}
