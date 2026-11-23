/* Copyright (c) 2026 bhgv
 * EN: Check result-channel aliases, confirmed selections, exact data and retirement.
 * RU: Проверка алиасов каналов, операций выбора, точных данных и отзыва.
 */
#include "channel-test.h"
#include "managed-test.h"
#include "flashvm/exception.h"
#include "flashvm/execution.h"
#include "flashvm/gc.h"
#include "flashvm/gc_roots.h"
#include <stdio.h>
#include <string.h>

#define PORTS_SIGNATURE UINT32_C(0xedf0338d) /* Compiled ports interface. */
#define EMPTY_SIGNATURE UINT32_C(0x421b2140) /* Compiled empty interface. */
#define NESTED_SIGNATURE UINT32_C(0xe74e6a20) /* Compiled nested result. */
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


/* [GC:] EN/RU: See channel-test.h. / См. channel-test.h. */
FVM_Result test_channel_select(FVM_Context *context, const FVM_Value *anchor, const char *method,
    unsigned count, const FVM_Value *ports, const FVM_Value *const *operands, unsigned *ordinal, FVM_Value *out)
{
    FVM_GCRootScope *roots;
    FVM_Value guards[STYXRPC_CHANNEL_SLOTS], list, result, index;
    if (!count || count > STYXRPC_CHANNEL_SLOTS)
    {
        return FVM_INVALID_ARGUMENT;
    }
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    for (unsigned i = 0; status == FVM_OK && i < count; ++i)
    {
        FVM_Value fields[2] = {ports[i]};
        const FVM_Value *operand = operands ? operands[i] : NULL;
        if (operand)
        {
            fields[1] = *operand;
        }
        status = fvm_exec_literal_array(context, operand ? 2 : 1, fields, guards + i);
        if (status == FVM_OK)
        {
            status = fvm_gc_roots_add(roots, guards + i);
        }
    }
    if (status == FVM_OK)
    {
        status = fvm_exec_literal_array(context, count, guards, &list);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_call(context, anchor->object, method, 1, &list, &result);
    }
    if (status == FVM_OK && result.type == FVM_VALUE_UNDEFINED)
    {
        *ordinal = 0;
        *out = result;
    }
    else if (status == FVM_OK)
    {
        if (result.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &result) == FVM_OK &&
            fvm_object_get(context, result.object, "0", &index) == FVM_OK && index.type == FVM_VALUE_NUMBER &&
            index.number >= 1 && index.number <= count && (unsigned)index.number == index.number &&
            fvm_object_get(context, result.object, "1", out) == FVM_OK)
        {
            *ordinal = (unsigned)index.number;
        }
        else
        {
            status = FVM_ERROR;
        }
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* EN: Select across int and string, reject another call, and commit only the winning send.
 * RU: Выбрать между int и string, отклонить другой вызов и подтвердить только выигравшую отправку. */
static int alternatives(FVM_Context *context, const FVM_Value *left, const FVM_Value *right,
    const FVM_Value *foreign, const char *counted, size_t length)
{
    FVM_Value ports[] = {*left, *right}, value, replacement, out;
    const FVM_Value *operands[] = {NULL, &replacement};
    unsigned branch;
    if (fvm_value_string_bytes(&replacement, counted, length) != FVM_OK ||
        test_channel_select(context, right, "select", 2, ports, operands, &branch, &out) != FVM_OK ||
        branch != 1 || out.type != FVM_VALUE_NUMBER || out.number != 41 ||
        test_channel_select(context, left, "trySelect", 2, ports, NULL, &branch, &out) != FVM_OK ||
        branch != 2 || out.type != FVM_VALUE_STRING || fvm_value_string_size(&out) != length ||
        memcmp(out.string, counted, length) ||
        test_channel_select(context, left, "trySelect", 2, ports, NULL, &branch, &out) != FVM_OK || branch)
    {
        return 0;
    }
    fvm_value_number(&value, -7);
    ports[1] = *foreign;
    operands[0] = &value;
    operands[1] = NULL;
    if (test_channel_select(context, left, "trySelect", 2, ports, operands, &branch, &out) != FVM_EXCEPTION ||
        fvm_exception_clear(context) != FVM_OK || !idle(context, left) ||
        fvm_object_call(context, left->object, "send", 1, &value, &out) != FVM_OK)
    {
        return 0;
    }
    ports[1] = *right;
    operands[1] = &replacement;
    return test_channel_select(context, left, "trySelect", 2, ports, operands, &branch, &out) == FVM_OK &&
        branch == 2 && out.type == FVM_VALUE_UNDEFINED && number(context, left, "receive", -7) &&
        idle(context, left) && text(context, right, counted, length) && idle(context, right);
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
        !alternatives(context, &left, &right, &second, counted, sizeof(counted)) || !idle(context, &alias) ||
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
        idle(context, &right) &&
        fvm_value_string_bytes(&value, counted, sizeof(counted)) == FVM_OK &&
        fvm_object_call(context, right.object, "send", 1, &value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean && text(context, &right, counted, sizeof(counted)) &&
        fvm_object_call(context, right.object, "close", 0, NULL, &reply) == FVM_OK &&
        idle(context, &second) && fvm_object_call(context, second.object, "close", 0, NULL, &reply) == FVM_OK;
}


/* [GC:] EN: Check aliases and nil in a received message and retain the message under roots.
 * RU: Проверить алиасы и nil принятого сообщения и удержать сообщение под roots. */
static int nested_fields(FVM_Context *context, FVM_GCRootScope *roots,
    const FVM_Value *message, FVM_Value *child)
{
    FVM_Value alias, nil;
    return message->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, message) == FVM_OK &&
        fvm_object_get(context, message->object, "0", child) == FVM_OK && child->type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, message->object, "1", &alias) == FVM_OK &&
        alias.type == FVM_VALUE_OBJECT && alias.object == child->object &&
        fvm_object_get(context, message->object, "2", &nil) == FVM_OK && nil.type == FVM_VALUE_NULL;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_nested_message(FVM_Context *context, FVM_GCRootScope *roots,
    const FVM_Value *outer, FVM_Value *message, FVM_Value *child)
{
    return outer->type == FVM_VALUE_OBJECT &&
        fvm_object_call(context, outer->object, "receive", 0, NULL, message) == FVM_OK &&
        nested_fields(context, roots, message, child);
}


/* [GC:] EN: Select a nested array on outer while the int child's send is blocked.
 * RU: Выбрать вложенный массив на outer, пока отправка через дочерний int заблокирована. */
static int select_nested(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *outer,
    const FVM_Value *child, FVM_Value *message, FVM_Value *received)
{
    FVM_Value ports[] = {*child, *outer}, blocked;
    const FVM_Value *operands[] = {&blocked, NULL};
    unsigned branch;
    fvm_value_number(&blocked, 99);
    return test_channel_select(context, child, "trySelect", 2, ports, operands, &branch, message) == FVM_OK &&
        branch == 2 && nested_fields(context, roots, message, received) && received->object == child->object;
}


/* EN: Drop a received wrapper, collect weak aliases and materialize the key a second time.
 * RU: Отбросить принятую обёртку, собрать слабые алиасы и повторно открыть тот же ключ. */
static int recollect(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *outer)
{
    FVM_GCRootScope *small;
    FVM_Value batch, child;
    unsigned long reclaimed;
    if (fvm_gc_roots_begin(context, &small) != FVM_OK)
    {
        return 0;
    }
    int ok = test_channel_nested_message(context, small, outer, &batch, &child);
    fvm_gc_roots_end(small);
    return ok && fvm_gc_collect(context, &reclaimed) == FVM_OK &&
        test_channel_nested_message(context, roots, outer, &batch, &child) &&
        number(context, &child, "receive", 123);
}


/* EN: Validate nested admission, exact lease identity and independent child lifetime after close.
 * RU: Проверить вложенный допуск, точную аренду и независимость дочернего канала после close. */
static int nested(FVM_Context *context, FVM_GCRootScope *roots, StyxRpcManagedTarget *target)
{
    const StyxRpcSchema schema = {"(i)", "(c[a[c[i]]])"};
    FVM_Value outer, other, batch, foreign, inner, sibling, again, sync, value, reply, dropped;
    unsigned long reclaimed;
    target->name = "nested";
    target->signature = NESTED_SIGNATURE;
    target->schema = schema;
    fvm_value_number(&value, 123);
    if (call(context, 1, &value, &outer) != FVM_OK || fvm_gc_roots_add(roots, &outer) != FVM_OK ||
        call(context, 1, &value, &other) != FVM_OK || fvm_gc_roots_add(roots, &other) != FVM_OK ||
        !test_channel_nested_message(context, roots, &outer, &batch, &inner) ||
        !test_channel_nested_message(context, roots, &other, &foreign, &sibling) ||
        inner.object == sibling.object ||
        fvm_object_get(context, batch.object, "3", &sync) != FVM_OK ||
        sync.type != FVM_VALUE_OBJECT || !idle(context, &sync) ||
        !select_nested(context, roots, &outer, &inner, &reply, &again))
    {
        return 0;
    }
    return fvm_object_call(context, outer.object, "trySend", 1, &foreign, &reply) == FVM_EXCEPTION &&
        fvm_exception_clear(context) == FVM_OK && idle(context, &outer) &&
        fvm_object_call(context, outer.object, "trySend", 1, &batch, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean &&
        test_channel_nested_message(context, roots, &outer, &reply, &again) && again.object == inner.object &&
        fvm_object_call(context, outer.object, "close", 0, NULL, &reply) == FVM_OK &&
        fvm_gc_collect(context, &reclaimed) == FVM_OK && number(context, &inner, "receive", 123) &&
        number(context, &sibling, "receive", 123) &&
        fvm_object_call(context, inner.object, "close", 0, NULL, &reply) == FVM_OK && closed(context, &again) &&
        call(context, 1, &value, &dropped) == FVM_OK && fvm_gc_roots_add(roots, &dropped) == FVM_OK &&
        recollect(context, roots, &dropped);
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
        ok = call(context, 0, NULL, &result) == FVM_OK && result.type == FVM_VALUE_NULL &&
            nested(context, roots, target);
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
