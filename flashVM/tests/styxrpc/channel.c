/* Copyright (c) 2026 bhgv
 * EN: Check result-channel aliases, confirmed selections, exact data and retirement.
 * RU: Проверка алиасов каналов, операций выбора, точных данных и отзыва.
 */
#include "channel-test.h"
#include "managed-test.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include <stdio.h>
#include <string.h>

#define PORTS_SIGNATURE UINT32_C(0xedf0338d) /* Compiled ports interface. */
#define EMPTY_SIGNATURE UINT32_C(0x421b2140) /* Compiled empty interface. */
#define CHANNEL_TEST_WAIT_MS 1500U /* Reply/operation polling budget. */


/* EN: Cut through the exact import selected by the fixture's compiled schema.
 * RU: Вызвать точный импорт из компилированной схемы проверки. */
static FVM_Result call(FVM_Context *context, unsigned argc, const FVM_Value *args, FVM_Value *out)
{
    return fvm_call_bytes(context, "portsRPC", 8, argc, args, out);
}


/* EN: Read and confirm one ordinary ActionScript channel method result.
 * RU: Прочитать и сверить результат обычного метода канала ActionScript. */
static int number(FVM_Context *context, const FVM_Value *channel, const char *method, int expected)
{
    FVM_Value value;
    return fvm_object_call(context, channel->object, method, 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == expected;
}


/* EN: Read an explicit unready outcome without confusing it with nil.
 * RU: Прочитать явный результат отсутствия готовой ветки, отличая его от nil. */
static int idle(FVM_Context *context, const FVM_Value *channel)
{
    FVM_Value value;
    return fvm_object_call(context, channel->object, "tryReceive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_UNDEFINED;
}


/* EN: Read an exact counted string, including the zero between its characters.
 * RU: Прочитать строку с точной длиной и нулём между символами. */
static int text(FVM_Context *context, const FVM_Value *channel, const char *expected, size_t size)
{
    FVM_Value value;
    return fvm_object_call(context, channel->object, "receive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == size &&
        !memcmp(value.string, expected, size);
}


/* EN: Check the closed endpoint's explicit exception before continuing with its sibling.
 * RU: Сверить явное исключение закрытого канала перед работой с соседним. */
static int closed(FVM_Context *context, const FVM_Value *channel)
{
    FVM_Value error;
    return fvm_object_call(context, channel->object, "receive", 0, NULL, &error) == FVM_EXCEPTION &&
        fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING &&
        !strcmp(error.string, "Styx RPC channel is closed") && fvm_exception_clear(context) == FVM_OK;
}


/* EN: Exercise independent channels and another call while both leases are open.
 * RU: Проверить раздельные каналы и новый вызов при двух открытых арендах. */
static int exercise(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *group, const FVM_Value *other)
{
    static const char counted[] = {'a', 0, (char)0xd0, (char)0xb1};
    FVM_Value left, alias, right, second, value, reply;
    unsigned long reclaimed;
    if (group->type != FVM_VALUE_OBJECT || other->type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, group->object, "0", &left) != FVM_OK || left.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, group->object, "1", &alias) != FVM_OK || alias.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, group->object, "2", &right) != FVM_OK || right.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, other->object, "0", &second) != FVM_OK || second.type != FVM_VALUE_OBJECT ||
        fvm_gc_roots_add(roots, &left) != FVM_OK || fvm_gc_roots_add(roots, &right) != FVM_OK ||
        left.object != alias.object || left.object == second.object || left.object == right.object ||
        fvm_gc_collect(context, &reclaimed) != FVM_OK ||
        !number(context, &left, "tryReceive", 41) || !idle(context, &alias) ||
        !number(context, &second, "receive", 91))
    {
        return 0;
    }
    fvm_value_number(&value, -7);
    return fvm_object_call(context, left.object, "trySend", 1, &value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean &&
        fvm_object_call(context, left.object, "trySend", 1, &value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && !reply.boolean &&
        number(context, &alias, "receive", -7) &&
        fvm_object_call(context, left.object, "close", 0, NULL, &reply) == FVM_OK && closed(context, &alias) &&
        text(context, &right, counted, sizeof(counted)) && idle(context, &right) &&
        fvm_value_string_bytes(&value, counted, sizeof(counted)) == FVM_OK &&
        fvm_object_call(context, right.object, "send", 1, &value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean && text(context, &right, counted, sizeof(counted)) &&
        fvm_object_call(context, right.object, "close", 0, NULL, &reply) == FVM_OK &&
        idle(context, &second) && fvm_object_call(context, second.object, "close", 0, NULL, &reply) == FVM_OK;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_call(FVM_Context *context, StyxRpcManagedTarget *target)
{
    const StyxRpcSchema ports = {"(i)", "((c[i]c[i]c[s]))"};
    const StyxRpcSchema empty = {"()", "(c[i])"};
    FVM_GCRootScope *roots = NULL;
    FVM_Value args, group, other, result;
    target->module = "mounted/channel.dis";
    target->name = "ports";
    target->signature = PORTS_SIGNATURE;
    target->schema = ports;
    target->pid = TEST_MANAGED_PID;
    target->wait_ms = CHANNEL_TEST_WAIT_MS;
    fvm_value_number(&args, 41);
    int ok = fvm_gc_roots_begin(context, &roots) == FVM_OK &&
        fvm_styxrpc_channel_import(context, "portsRPC", target) == FVM_OK &&
        call(context, 1, &args, &group) == FVM_OK && fvm_gc_roots_add(roots, &group) == FVM_OK;
    if (ok)
    {
        fvm_value_number(&args, 91);
        ok = call(context, 1, &args, &other) == FVM_OK && fvm_gc_roots_add(roots, &other) == FVM_OK &&
            exercise(context, roots, &group, &other);
    }
    if (ok)
    {
        target->name = "empty";
        target->signature = EMPTY_SIGNATURE;
        target->schema = empty;
        ok = call(context, 0, NULL, &result) == FVM_OK && result.type == FVM_VALUE_NULL;
    }
    if (!ok && fvm_exception_get(context, &result) == FVM_OK && result.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "FlashVM channel import %s failed: %s\n", target->name, result.string);
    }
    if (roots)
    {
        fvm_gc_roots_end(roots);
    }
    target->session = NULL;
    return ok;
}
