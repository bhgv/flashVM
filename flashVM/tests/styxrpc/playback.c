/* Copyright (c) 2026 bhgv
 * EN: Native FlashVM control owner for the Limbo lifecycle test.
 * RU: Нативный владелец плеера FlashVM для проверки управления из Limbo.
 */
#include "flashvm/styxrpc_playback.h"
#include "movie-assets.h"
#include "styxrpc/module.h"
#include "drivers/posix/fd.h"
#include <stdio.h>
#include <string.h>


typedef struct PlaybackTrace
{
    unsigned int debug_calls; /* Confirmed debugger attachments. */
    unsigned int child;       /* First frame executed in the fetched child. */
    unsigned int leaf;        /* First frame executed in the fetched grandchild. */
    unsigned int other;       /* Unexpected trace, including the local decoy. */
} PlaybackTrace;


/* EN: Count exact AVM1 trace messages from the expected frames.
 * RU: Подсчитать точные сообщения AVM1 из ожидаемых кадров. */
static FVM_Result trace(FVM_Context *context, FVM_Object *receiver, unsigned int argc,
    const FVM_Value *argv, FVM_Value *out, void *host)
{
    PlaybackTrace *observed = host;
    (void)context;
    (void)receiver;
    (void)out;
    if (argc == 1 && argv->type == FVM_VALUE_STRING)
    {
        size_t length = fvm_value_string_size(argv);
        if (length == sizeof("styx-child") - 1 && !memcmp(argv->string, "styx-child", length))
        {
            ++observed->child;
        }
        else if (length == sizeof("styx-leaf") - 1 && !memcmp(argv->string, "styx-leaf", length))
        {
            ++observed->leaf;
        }
        else
        {
            ++observed->other;
        }
    }
    return FVM_OK;
}


/* EN: Attach the trace observer to every freshly loaded test context.
 * RU: Подключить наблюдателя к каждому новому контексту загрузки. */
static FVM_Result setup(void *host, FVM_Context *context)
{
    return fvm_avm1_set_trace_rooted(context, trace, host, NULL);
}


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
    ++((PlaybackTrace *)host)->debug_calls;
    return FVM_OK;
}


/* EN: Append the application debugger count to the playback snapshot.
 * RU: Добавить счётчик подключений отладчика к снимку состояния. */
static int status(void *host, char *bytes, size_t capacity, size_t *used)
{
    PlaybackTrace *observed = host;
    int result = snprintf(bytes, capacity, "debug_attached=%u\nchild_traces=%u\nleaf_traces=%u\nother_traces=%u\n",
        observed->debug_calls, observed->child, observed->leaf, observed->other);
    if (result < 0 || (size_t)result >= capacity)
    {
        return 0;
    }
    *used = (size_t)result;
    return 1;
}


/* EN: Serve the mixed tree on the native owner and verify the debugger callback.
 * RU: Обслужить смешанное дерево у владельца и проверить колбэк отладчика. */
int test_playback_serve(FVM_Runtime *runtime, int fd, int jit, int children)
{
    PlaybackTrace observed = {0};
    MovieAssets assets = {0};
    if (children && !test_movie_assets_start(&assets))
    {
        test_movie_assets_stop(&assets);
        fputs("Cannot start fixture asset provider\n", stderr);
        return 0;
    }
    FVM_StyxRpcPlaybackSpec playback_spec = {runtime, assets.client, children ? 16384 : 4096,
        jit ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, &observed, setup, debugger, status};
    FVM_StyxRpcPlayback *playback = NULL;
    if (fvm_styxrpc_playback_create(&playback_spec, &playback) != FVM_OK)
    {
        test_movie_assets_stop(&assets);
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
    int ok = server != NULL && observed.debug_calls == 1 &&
        (!children || (observed.child == 1 && observed.leaf == 1 && observed.other == 0));
    styxrpc_server_destroy(server);
    styxrpc_module_destroy(module);
    fvm_styxrpc_playback_destroy(playback);
    test_movie_assets_stop(&assets);
    return ok;
}
