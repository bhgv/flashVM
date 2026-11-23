/* Copyright (c) 2026 bhgv
 * EN: Serve a FlashVM mailbox result while Dis provides live reverse channels.
 * RU: Выдача очереди результата FlashVM с живыми встречными каналами Dis.
 */
#include "channel-test.h"
#include "flashvm/exception.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define WAIT_MS 2000U /* Native reverse I/O budget. */
#define CREATE_SIG UINT32_C(0xe747e7ab) /* Limbo int-to-mailbox export. */
#define CHECK_SIG UINT32_C(0x4fef7e8c)  /* Limbo chan(int)-to-int owner probe. */


/* EN: Borrow server and module on the Styx worker through its disconnect.
 * RU: Заимствовать сервер и модуль в рабочем потоке Styx до отключения. */
typedef struct Dispatch
{
    StyxRpcServer *server;           /* Dispatcher-owned connection. */
    StyxRpcChannelHandlers *module; /* Owner job queue to stop on disconnect. */
} Dispatch;


/* [GC:] EN: Verify the copied marker and retain the script's new mailbox in its globals.
 * RU: Проверить скопированный маркер и сохранить новую очередь в глобальных данных. */
static FVM_Result remember(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    (void)self;
    (void)unused;
    if (argc != 2 || args[0].type != FVM_VALUE_OBJECT || args[1].type != FVM_VALUE_NUMBER || args[1].number != 17)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    return fvm_set_global(context, "heldMailbox", args);
}


/* [GC:] EN: Use the admitted PID, echo its exact identity and retain the proxy for expiry.
 * RU: Использовать допущенный PID, вернуть его идентичность и сохранить прокси для отзыва. */
static FVM_Result drain(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_GCRootScope *roots;
    FVM_Value mailbox, first, second, sent, number;
    (void)self;
    (void)args;
    (void)unused;
    if (argc != 1 || fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_number(&number, 43);
    int ok = fvm_get_global(context, "heldMailbox", &mailbox) == FVM_OK && mailbox.type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, mailbox.object, "tryReceive", 0, NULL, &first) == FVM_OK &&
        first.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &first) == FVM_OK &&
        fvm_object_call(context, mailbox.object, "tryReceive", 0, NULL, &second) == FVM_OK &&
        second.type == FVM_VALUE_OBJECT && first.object == second.object &&
        fvm_object_call(context, first.object, "receive", 0, NULL, out) == FVM_OK &&
        out->type == FVM_VALUE_NUMBER && out->number == 81 &&
        fvm_object_call(context, first.object, "send", 1, &number, &sent) == FVM_OK &&
        sent.type == FVM_VALUE_BOOLEAN && sent.boolean &&
        fvm_object_call(context, mailbox.object, "trySend", 1, &first, &sent) == FVM_OK &&
        sent.type == FVM_VALUE_BOOLEAN && sent.boolean &&
        fvm_set_global(context, "heldProxy", &first) == FVM_OK;
    fvm_gc_roots_end(roots);
    if (!ok)
    {
        if (fvm_exception_get(context, &sent) == FVM_OK && sent.type == FVM_VALUE_STRING)
        {
            fprintf(stderr, "%s\n", sent.string);
        }
        fputs("Could not operate on or echo the native PID\n", stderr);
    }
    return ok ? FVM_OK : FVM_ERROR;
}


/* EN: Poll for clunk of the hosted resource, then confirm the saved proxy was revoked.
 * RU: Дождаться clunk ресурса, затем подтвердить отзыв сохранённого прокси. */
static FVM_Result retired(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_Value mailbox, proxy, result;
    FVM_Result status;
    (void)self;
    (void)args;
    (void)unused;
    if (argc != 1 || fvm_get_global(context, "heldMailbox", &mailbox) != FVM_OK ||
        mailbox.type != FVM_VALUE_OBJECT)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = fvm_object_call(context, mailbox.object, "tryReceive", 0, NULL, &result);
    if (status == FVM_OK && result.type == FVM_VALUE_UNDEFINED)
    {
        fvm_value_number(out, 0);
        return FVM_OK;
    }
    if (status != FVM_EXCEPTION || fvm_exception_get(context, &result) != FVM_OK ||
        result.type != FVM_VALUE_STRING || strcmp(result.string, "Styx RPC mailbox is closed") ||
        fvm_exception_clear(context) != FVM_OK ||
        fvm_get_global(context, "heldProxy", &proxy) != FVM_OK || proxy.type != FVM_VALUE_OBJECT ||
        fvm_object_call(context, proxy.object, "tryReceive", 0, NULL, &result) != FVM_EXCEPTION ||
        fvm_exception_get(context, &result) != FVM_OK || result.type != FVM_VALUE_STRING ||
        strcmp(result.string, "Styx RPC channel is closed") || fvm_exception_clear(context) != FVM_OK)
    {
        return FVM_ERROR;
    }
    fvm_value_number(out, 1);
    return FVM_OK;
}


/* EN: Own the forward Styx dispatcher and wake the ActionScript owner on disconnect.
 * RU: Обслуживать прямой диспетчер Styx и разбудить владельца AS при отключении. */
static void *serve(void *state)
{
    Dispatch *owner = state;
    while (styxrpc_server_step(owner->server))
    {
    }
    styxrpc_server_destroy(owner->server);
    styxrpc_channel_handlers_stop(owner->module);
    return NULL;
}


/* [GC:] EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_pid_result(FVM_Context *context, int fd)
{
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 0;
    }
    StyxClient *reverse = styxclientmountphy(phybidirchannel(mux, 1), 4096, "flash", "", WAIT_MS, 8);
    FVM_StyxRpcPidMailboxExport binding = {context, "rememberMailbox", reverse, {"(i)", "(c[c[i]])"}, 2, WAIT_MS};
    FVM_StyxRpcChannelHandler calls[] = {
        {context, "drainMailbox", reverse, {"(c[i])", "(i)"}, WAIT_MS},
        {context, "retiredMailbox", reverse, {"(c[i])", "(i)"}, WAIT_MS}
    };
    StyxRpcChannelFunction functions[2];
    StyxRpcChannelExport result;
    StyxRpcModuleChannels registry;
    StyxRpcModuleChannel slots[2];
    int ok = reverse && fvm_register_function(context, "rememberMailbox", remember, NULL) == FVM_OK &&
        fvm_register_function(context, "drainMailbox", drain, NULL) == FVM_OK &&
        fvm_register_function(context, "retiredMailbox", retired, NULL) == FVM_OK &&
        fvm_styxrpc_channel_handler(calls, "drain", CHECK_SIG, functions) == FVM_OK &&
        fvm_styxrpc_channel_handler(calls + 1, "retired", CHECK_SIG, functions + 1) == FVM_OK &&
        fvm_styxrpc_pid_mailbox_export(&binding, "mailbox", CREATE_SIG, &result) == FVM_OK &&
        styxrpc_module_channels_init(&registry, slots, 2, test_channel_entropy, NULL);
    Dispatch owner = {0};
    owner.module = ok ? styxrpc_channel_handlers_create_with_results("flash-pid-result.dis",
        functions, 2, &result, 1, &registry) : NULL;
    owner.server = owner.module ? styxrpc_server_create(phybidirchannel(mux, 0),
        &styxrpc_channel_handlers_tree, owner.module) : NULL;
    pthread_t thread;
    ok = owner.server && !pthread_create(&thread, NULL, serve, &owner);
    if (ok)
    {
        while (styxrpc_channel_handlers_step(owner.module))
        {
        }
        pthread_join(thread, NULL);
    }
    else
    {
        styxrpc_server_destroy(owner.server);
    }
    styxrpc_channel_handlers_destroy(owner.module);
    styxclientshutdown(reverse);
    styxclientfree(reverse);
    phybidirfree(mux);
    return ok;
}
