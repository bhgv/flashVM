/* Copyright (c) 2026 bhgv
 * EN: Check native Dis operations on argument queues created in FlashVM.
 * RU: Проверка операций Dis над очередями аргументов, созданными в FlashVM.
 */
#include "channel-test.h"
#include "managed-test.h"
#include "flashvm/avm1.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/gc_roots.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <string.h>

#define EXCHANGE_SIGNATURE UINT32_C(0x89f34ef5) /* Compiled array/channel argument shapes. */
#define REJECT_SIGNATURE UINT32_C(0xe4cc716f)    /* Compiled channel argument and void result. */
#define RESULT_SIGNATURE UINT32_C(0x9c30df25)    /* Compiled channel argument and result. */
#define RELAY_SIGNATURE UINT32_C(0x4fef7e8c)     /* Compiled channel argument and int result. */
#define NESTED_SIGNATURE UINT32_C(0x8e188767) /* Outer queue, direct child and result. */
#define ARGUMENT_RPC_WAIT 3000U /* Bounded wait while the reverse dispatcher is active. */
#define ARGUMENT_TIMEOUT_WAIT 150U /* Deadline for the controlled pending receive. */
#define ACTION_END 0x00           /* Finish the AVM1 action block. */
#define ACTION_GET_VARIABLE 0x1c  /* Read a rooted script argument. */
#define ACTION_SET_VARIABLE 0x1d  /* Save the ordinary script result. */
#define ACTION_CALL_FUNCTION 0x3d /* Invoke the registered channel global. */
#define ACTION_PUSH 0x96          /* Header for a typed string or int operand. */
#define PUSH_NAME(name) ACTION_PUSH, 3, 0, 0, name, 0 /* One checked short ASCII name. */

static const StyxRpcSchema exchange = {"(a[c[i]]c[s])", "(i)"}; /* Distinct typed queues and aliases. */
static const StyxRpcSchema reject = {"(c[i])", "()"};       /* Live effect before exception. */
static const StyxRpcSchema result = {"(c[i])", "(c[i])"};  /* Argument retirement plus result lease. */
static const StyxRpcSchema relay = {"(c[i])", "(i)"};      /* Native rendezvous between two Dis owners. */


/* [GC:] EN: Create a queue from the script constructor and add it to the active roots.
 * RU: Создать очередь функцией AS и добавить её под текущие корни. */
static int create(FVM_Context *context, FVM_GCRootScope *roots, const char *type, unsigned capacity, FVM_Value *value)
{
    FVM_Value args[2];
    fvm_value_number(args + 1, capacity);
    return fvm_value_string(args, type) == FVM_OK &&
        fvm_call_bytes(context, "newQueue", 8, 2, args, value) == FVM_OK &&
        value->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, value) == FVM_OK;
}


/* EN: Attempt a local typed send and check its Boolean result.
 * RU: Попробовать локальную типизированную отправку и сверить булев результат. */
static int send(FVM_Context *context, const FVM_Value *channel, const FVM_Value *value, int expected)
{
    FVM_Value sent;
    return fvm_object_call(context, channel->object, "trySend", 1, value, &sent) == FVM_OK &&
        sent.type == FVM_VALUE_BOOLEAN && sent.boolean == expected;
}


/* EN: Read one expected signed int from the selected channel method.
 * RU: Прочитать ожидаемый знаковый int выбранным методом канала. */
static int receive_int(FVM_Context *context, const FVM_Value *channel, const char *method, int expected)
{
    FVM_Value value;
    return channel->type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, channel->object, method, 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == expected;
}


/* EN: Check one explicit error before clearing the context's exception.
 * RU: Сверить явную ошибку перед очисткой исключения контекста. */
static int exception(FVM_Context *context, const char *expected)
{
    FVM_Value value;
    return fvm_exception_get(context, &value) == FVM_OK && value.type == FVM_VALUE_STRING &&
        !strcmp(value.string, expected) && fvm_exception_clear(context) == FVM_OK;
}


/* EN: Switch only the borrowed compiled offer before calling the registered script global.
 * RU: Сменить заимствованное предложение перед вызовом глобальной функции AS. */
static FVM_Result call(FVM_Context *context, StyxRpcManagedTarget *target, const char *name,
    uint32_t signature, const StyxRpcSchema *schema, unsigned count, const FVM_Value *args, FVM_Value *out)
{
    target->name = name;
    target->signature = signature;
    target->schema = *schema;
    return fvm_call_bytes(context, "queueRPC", 8, count, args, out);
}


/* EN: Invoke the imported global from real AVM1 using the two script-visible arguments.
 * RU: Вызвать импортированную глобальную функцию из AVM1 с двумя аргументами AS. */
static int script_exchange(FVM_Context *context, const FVM_Value *args, FVM_Value *out)
{
    static const unsigned char actions[] = {
        PUSH_NAME('r'), PUSH_NAME('q'), ACTION_GET_VARIABLE, PUSH_NAME('p'), ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, 7, 2, 0, 0, 0,
        ACTION_PUSH, 10, 0, 0, 'q', 'u', 'e', 'u', 'e', 'R', 'P', 'C', 0,
        ACTION_CALL_FUNCTION, ACTION_SET_VARIABLE, ACTION_END
    };
    return fvm_set_global(context, "p", args) == FVM_OK &&
        fvm_set_global(context, "q", args + 1) == FVM_OK &&
        fvm_avm1_execute(context, actions, sizeof(actions)) == FVM_OK &&
        fvm_get_global(context, "r", out) == FVM_OK;
}


/* [GC:] EN: Check aliases, counted typed messages, invalid local inputs and remote exceptions.
 * RU: Сверить алиасы, сообщения с длиной, неверные локальные аргументы и удалённые ошибки. */
static int exercise(FVM_Context *context, FVM_GCRootScope *roots, StyxRpcManagedTarget *target,
    const FVM_Value *ints, const FVM_Value *text, const FVM_Value *sync)
{
    static const char counted[] = {'a', 0, (char)0xd0, (char)0xb1, '!'};
    FVM_Value args[2], channels[4], value, out, remote;
    channels[0] = channels[1] = *ints;
    fvm_value_undefined(channels + 2);
    channels[2].type = FVM_VALUE_NULL;
    channels[3] = *sync;
    fvm_value_number(&value, 41);
    if (!send(context, ints, &value, 1) || !send(context, ints, &value, 0) ||
        fvm_value_string_bytes(&value, counted, sizeof(counted) - 1) != FVM_OK ||
        !send(context, text, &value, 1) ||
        fvm_exec_literal_array(context, 4, channels, args) != FVM_OK ||
        fvm_gc_roots_add(roots, args) != FVM_OK)
    {
        return 0;
    }
    args[1] = *ints;
    if (call(context, target, "exchange", EXCHANGE_SIGNATURE, &exchange, 2, args, &out) != FVM_EXCEPTION ||
        !exception(context, "Styx RPC channel arguments do not fit the schema"))
    {
        return 0;
    }
    args[1] = *text;
    if (!script_exchange(context, args, &out) ||
        out.type != FVM_VALUE_NUMBER || out.number != 41 || !receive_int(context, ints, "tryReceive", 42) ||
        fvm_object_call(context, text->object, "tryReceive", 0, NULL, &out) != FVM_OK ||
        out.type != FVM_VALUE_STRING || fvm_value_string_size(&out) != sizeof(counted) ||
        memcmp(out.string, counted, sizeof(counted)))
    {
        return 0;
    }
    if (call(context, target, "reject", REJECT_SIGNATURE, &reject, 1, ints, &out) != FVM_EXCEPTION ||
        !exception(context, "Styx RPC remote function failed") || !receive_int(context, ints, "tryReceive", 99))
    {
        return 0;
    }
    target->wait_ms = ARGUMENT_TIMEOUT_WAIT;
    int cancelled = call(context, target, "wait", RELAY_SIGNATURE, &relay, 1, ints, &out) == FVM_EXCEPTION &&
        exception(context, "Styx RPC outcome is uncertain");
    target->wait_ms = ARGUMENT_RPC_WAIT;
    if (!cancelled)
    {
        return 0;
    }
    fvm_value_number(&value, 19);
    if (!send(context, ints, &value, 1) ||
        call(context, target, "result", RESULT_SIGNATURE, &result, 1, ints, &remote) != FVM_OK ||
        !receive_int(context, &remote, "receive", 21) ||
        fvm_object_call(context, remote.object, "close", 0, NULL, &out) != FVM_OK)
    {
        return 0;
    }
    return send(context, sync, &value, 0) &&
        call(context, target, "relay", RELAY_SIGNATURE, &relay, 1, sync, &out) == FVM_OK &&
        out.type == FVM_VALUE_NUMBER && out.number == -19;
}


/* [GC:] EN: Enqueue aliases under short roots so only the FIFO keeps the hidden channel alive.
 * RU: Поставить алиасы под короткими корнями, чтобы скрытый канал удерживала только очередь. */
static int seed_nested(FVM_Context *context, const FVM_Value *outer, const FVM_Value *direct)
{
    FVM_GCRootScope *roots;
    FVM_Value children[4], message, value;
    if (fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&value, 41);
    int ok = create(context, roots, "i", 1, children) && send(context, children, &value, 1);
    if (ok)
    {
        children[1] = children[0];
        fvm_value_undefined(children + 2);
        children[2].type = FVM_VALUE_NULL;
        children[3] = *direct;
        ok = fvm_exec_literal_array(context, 4, children, &message) == FVM_OK &&
            send(context, outer, &message, 1);
    }
    fvm_gc_roots_end(roots);
    return ok;
}


/* [GC:] EN: Check the restored message and child identity after two distinct generations.
 * RU: Сверить восстановленное сообщение и идентичность канала после двух разных поколений. */
static int nested_calls(FVM_Context *context, FVM_GCRootScope *roots, StyxRpcManagedTarget *target)
{
    const StyxRpcSchema schema = {"(c[a[c[i]]]c[i]i)", "(i)"};
    FVM_Value args[3], out, message, child, alias, nil, direct;
    unsigned long reclaimed;
    fvm_value_number(args + 2, 0);
    if (!create(context, roots, "a[c[i]]", 1, args) || !create(context, roots, "i", 1, args + 1) ||
        !seed_nested(context, args, args + 1) || fvm_gc_collect(context, &reclaimed) != FVM_OK ||
        call(context, target, "nested", NESTED_SIGNATURE, &schema, 3, args, &out) != FVM_OK ||
        out.type != FVM_VALUE_NUMBER || out.number != 41)
    {
        return 0;
    }
    fvm_value_number(args + 2, 1);
    return call(context, target, "nested", NESTED_SIGNATURE, &schema, 3, args, &out) == FVM_EXCEPTION &&
        exception(context, "Styx RPC remote function failed") &&
        fvm_gc_collect(context, &reclaimed) == FVM_OK &&
        fvm_object_call(context, args[0].object, "tryReceive", 0, NULL, &message) == FVM_OK &&
        message.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &message) == FVM_OK &&
        fvm_object_get(context, message.object, "0", &child) == FVM_OK && child.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, message.object, "1", &alias) == FVM_OK &&
        alias.type == FVM_VALUE_OBJECT && alias.object == child.object &&
        fvm_object_get(context, message.object, "2", &nil) == FVM_OK && nil.type == FVM_VALUE_NULL &&
        fvm_object_get(context, message.object, "3", &direct) == FVM_OK &&
        direct.type == FVM_VALUE_OBJECT && direct.object == args[1].object &&
        receive_int(context, &child, "tryReceive", 43) &&
        fvm_object_call(context, args[0].object, "tryReceive", 0, NULL, &out) == FVM_OK &&
        out.type == FVM_VALUE_UNDEFINED;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_args(FVM_Context *context, StyxRpcManagedTarget *target, StyxRpcArgumentBus *bus)
{
    FVM_StyxRpcChannelArguments binding = {target, bus};
    FVM_GCRootScope *roots = NULL;
    FVM_Value ints, text, sync, error;
    target->module = "mounted/channel-args.dis";
    target->name = "exchange";
    target->signature = EXCHANGE_SIGNATURE;
    target->schema = exchange;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = ARGUMENT_RPC_WAIT;
    int ok = fvm_gc_roots_begin(context, &roots) == FVM_OK &&
        fvm_styxrpc_channel_factory(context, "newQueue") == FVM_OK &&
        fvm_styxrpc_channel_import_arguments(context, "queueRPC", &binding) == FVM_OK &&
        create(context, roots, "i", 1, &ints) && create(context, roots, "s", 1, &text) &&
        create(context, roots, "i", 0, &sync) &&
        exercise(context, roots, target, &ints, &text, &sync) && nested_calls(context, roots, target);
    if (!ok)
    {
        if (fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING)
        {
            fprintf(stderr, "FlashVM argument call %s failed: %s\n", target->name, error.string);
        }
        else
        {
            fprintf(stderr, "FlashVM argument results from %s did not match\n", target->name);
        }
    }
    if (roots)
    {
        fvm_gc_roots_end(roots);
    }
    target->session = NULL;
    return ok;
}
