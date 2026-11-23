/* Copyright (c) 2026 bhgv
 * EN: Exercise borrowed Dis arguments on a real AVM1 owner and a separate Styx dispatcher.
 * RU: Проверка аргументов Dis у настоящего владельца AVM1 и отдельного диспетчера Styx.
 */
#include "channel-test.h"
#include "flashvm/exception.h"
#include "flashvm/function.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#define EXCHANGE_SIGNATURE UINT32_C(0x89f34ef5) /* Compiled channel array and string port. */
#define VOID_SIGNATURE UINT32_C(0xe4cc716f) /* Compiled int port with no result. */
#define INT_SIGNATURE UINT32_C(0x4fef7e8c)  /* Compiled int port with int result. */
#define STYX_WAIT 3000U     /* Client mount/operation polling budget, in ms. */
#define RECEIVE_WAIT 120U  /* Controlled timeout on an empty buffered port. */
#define ACTION_PUSH 0x96          /* Push a typed operand. */
#define ACTION_GET_VARIABLE 0x1c  /* Resolve the callable's formal. */
#define ACTION_CALL_METHOD 0x52   /* Call one method on the borrowed object. */
#define ACTION_RETURN 0x3e        /* Return the received exact value. */
#define ACTION_END 0x00           /* Finish the action stream. */

typedef struct Dispatch
{
    StyxRpcServer *server;           /* Owned by the network worker after launch. */
    StyxRpcChannelHandlers *module; /* Borrowed through worker and owner shutdown. */
} Dispatch;


/* EN: Fail the fixture with a precise local diagnostic.
 * RU: Завершить проверку с точной локальной диагностикой. */
static FVM_Result failure(const char *reason)
{
    fprintf(stderr, "FlashVM incoming channels: %s\n", reason);
    return FVM_INVALID_ARGUMENT;
}


/* EN: Send one value with trySend and confirm the queue selected the operation.
 * RU: Отправить значение через trySend и проверить выбор операции очередью. */
static int send(FVM_Context *context, FVM_Object *port, const FVM_Value *value)
{
    FVM_Value out;
    return fvm_object_call(context, port, "trySend", 1, value, &out) == FVM_OK &&
        out.type == FVM_VALUE_BOOLEAN && out.boolean;
}


/* EN: Exchange native values, verify aliases and save one object past callback return.
 * RU: Обменяться нативными значениями, сверить алиасы и сохранить объект после возврата. */
static FVM_Result exchange(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    static const char original[] = {'a', 0, (char)0xd0, (char)0xb1};
    static const char changed[] = {'a', 0, (char)0xd0, (char)0xb1, '!'};
    FVM_Value first, alias, nil, value;
    (void)self;
    (void)unused;
    if (argc != 2 || args[0].type != FVM_VALUE_OBJECT || args[1].type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, args[0].object, "0", &first) != FVM_OK || first.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, args[0].object, "1", &alias) != FVM_OK ||
        alias.type != FVM_VALUE_OBJECT || first.object != alias.object ||
        fvm_object_get(context, args[0].object, "2", &nil) != FVM_OK || nil.type != FVM_VALUE_NULL)
    {
        return failure("invalid aliases or nil argument");
    }
    if (fvm_object_call(context, first.object, "receive", 0, NULL, &value) != FVM_OK ||
        value.type != FVM_VALUE_NUMBER || value.number != 41)
    {
        return failure("invalid integer receive");
    }
    fvm_value_number(&value, 42);
    if (!send(context, alias.object, &value) ||
        fvm_object_call(context, args[1].object, "tryReceive", 0, NULL, &value) != FVM_OK ||
        value.type != FVM_VALUE_STRING || fvm_value_string_size(&value) != sizeof(original) ||
        memcmp(value.string, original, sizeof(original)) ||
        fvm_value_string_bytes(&value, changed, sizeof(changed)) != FVM_OK ||
        !send(context, args[1].object, &value) || fvm_set_global(context, "previous", &first) != FVM_OK)
    {
        return failure("invalid send or counted string exchange");
    }
    fvm_value_number(out, 41);
    return FVM_OK;
}


/* EN: Check that a globally retained old grant is invalid, then clear the exact exception.
 * RU: Сверить закрытие сохранённого старого полномочия и очистить точное исключение. */
static int closed(FVM_Context *context, const char *global)
{
    FVM_Value channel, result;
    return fvm_get_global(context, global, &channel) == FVM_OK && channel.type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, channel.object, "tryReceive", 0, NULL, &result) == FVM_EXCEPTION &&
        fvm_exception_get(context, &result) == FVM_OK && result.type == FVM_VALUE_STRING &&
        !strcmp(result.string, "Styx RPC channel is closed") && fvm_exception_clear(context) == FVM_OK;
}


/* EN: Confirm retirement of an earlier success, then cause an exception after a committed send.
 * RU: Проверить отзыв после успешного вызова, затем вызвать исключение после отправки. */
static FVM_Result reject(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    FVM_Value value;
    (void)self;
    (void)out;
    (void)unused;
    fvm_value_number(&value, 99);
    if (argc != 1 || args->type != FVM_VALUE_OBJECT || !closed(context, "previous") ||
        !send(context, args->object, &value) || fvm_set_global(context, "rejected", args) != FVM_OK ||
        fvm_value_string(&value, "test exception after send") != FVM_OK)
    {
        return failure("reject callback did not finish its send");
    }
    return fvm_exception_raise(context, &value);
}


/* EN: Confirm exception cleanup, idle TRY and local closure of the current grant.
 * RU: Сверить очистку исключения, пустой TRY и локальное закрытие текущего полномочия. */
static FVM_Result finish(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *unused)
{
    (void)self;
    (void)unused;
    if (argc != 1 || args->type != FVM_VALUE_OBJECT || !closed(context, "rejected") ||
        fvm_object_call(context, args->object, "tryReceive", 0, NULL, out) != FVM_OK ||
        out->type != FVM_VALUE_UNDEFINED ||
        fvm_object_call(context, args->object, "close", 0, NULL, out) != FVM_OK ||
        fvm_set_global(context, "explicitlyClosed", args) != FVM_OK || !closed(context, "explicitlyClosed"))
    {
        return failure("cleanup did not preserve the current channel's scope");
    }
    return FVM_OK;
}


/* EN: Install real AVM1 bytecode for return p.receive().
 * RU: Установить настоящий байткод AVM1 для return p.receive(). */
static int define_receive(FVM_Context *context)
{
    static const unsigned char actions[] = {
        ACTION_PUSH, 5, 0, 7, 0, 0, 0, 0,
        ACTION_PUSH, 3, 0, 0, 'p', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 9, 0, 0, 'r', 'e', 'c', 'e', 'i', 'v', 'e', 0,
        ACTION_CALL_METHOD, ACTION_RETURN, ACTION_END
    };
    const char *parameters[] = {"p"};
    FVM_Object *function;
    FVM_Value value;
    if (fvm_function_create_avm1(context, actions, sizeof(actions), 1, parameters, NULL, &function) != FVM_OK)
    {
        return 0;
    }
    fvm_value_undefined(&value);
    value.type = FVM_VALUE_OBJECT;
    value.object = function;
    return fvm_set_global(context, "receiveDis", &value) == FVM_OK;
}


/* EN: Run the Styx dispatcher for the physical session, then wake the owner for teardown.
 * RU: Обслуживать физическую сессию Styx, затем разбудить владельца для завершения. */
static void *serve(void *state)
{
    Dispatch *dispatch = state;
    while (styxrpc_server_step(dispatch->server))
    {
    }
    styxrpc_server_destroy(dispatch->server);
    styxrpc_channel_handlers_stop(dispatch->module);
    return NULL;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_incoming(FVM_Context *context, int fd)
{
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 0;
    }
    StyxClient *reverse = styxclientmountphy(phybidirchannel(mux, 1), 4096,
        "flash-incoming", "", STYX_WAIT, 8);
    FVM_StyxRpcChannelHandler bindings[] = {
        {context, "exchangeDis", reverse, {"(a[c[i]]c[s])", "(i)"}, STYX_WAIT},
        {context, "receiveDis", reverse, {"(c[i])", "(i)"}, STYX_WAIT},
        {context, "rejectDis", reverse, {"(c[i])", "()"}, STYX_WAIT},
        {context, "finishDis", reverse, {"(c[i])", "()"}, STYX_WAIT},
        {context, "receiveDis", reverse, {"(c[i])", "(i)"}, RECEIVE_WAIT}
    };
    const char *names[] = {"exchange", "relay", "reject", "finish", "timeout"};
    const uint32_t signatures[] = {EXCHANGE_SIGNATURE, INT_SIGNATURE, VOID_SIGNATURE, VOID_SIGNATURE, INT_SIGNATURE};
    StyxRpcChannelFunction functions[sizeof(bindings) / sizeof(*bindings)];
    size_t count = sizeof(functions) / sizeof(*functions);
    int ok = reverse && define_receive(context) &&
        fvm_register_function(context, "exchangeDis", exchange, NULL) == FVM_OK &&
        fvm_register_function(context, "rejectDis", reject, NULL) == FVM_OK &&
        fvm_register_function(context, "finishDis", finish, NULL) == FVM_OK;
    for (size_t i = 0; ok && i < count; ++i)
    {
        ok = fvm_styxrpc_channel_handler(bindings + i, names[i], signatures[i], functions + i) == FVM_OK;
    }
    Dispatch dispatch = {0};
    dispatch.module = ok ? styxrpc_channel_handlers_create("flash-incoming.dis", functions, count) : NULL;
    dispatch.server = dispatch.module ? styxrpc_server_create(phybidirchannel(mux, 0),
        &styxrpc_channel_handlers_tree, dispatch.module) : NULL;
    pthread_t thread;
    ok = dispatch.server && !pthread_create(&thread, NULL, serve, &dispatch);
    if (ok)
    {
        while (styxrpc_channel_handlers_step(dispatch.module))
        {
        }
        pthread_join(thread, NULL);
    }
    else
    {
        styxrpc_server_destroy(dispatch.server);
        fputs("FlashVM argument handler setup failed\n", stderr);
    }
    styxrpc_channel_handlers_destroy(dispatch.module);
    styxclientshutdown(reverse);
    styxclientfree(reverse);
    phybidirfree(mux);
    return ok;
}
