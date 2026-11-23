/* Copyright (c) 2026 bhgv
 * EN: Run a native module-grant round trip using the production ActionScript mailbox.
 * RU: Проверка обмена полномочием модуля через штатную очередь ActionScript.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel-test.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include <stdio.h>
#include <time.h>

#define CALL_PID 42U /* Reverse process already used by the recursive fixture. */
#define WAIT_MS 2000U /* Operation or local polling budget in milliseconds. */


/* EN: Poll for one authenticated module channel, yielding between empty results.
 * RU: Дождаться одного проверенного канала модуля, уступая поток при пустом результате. */
static int poll(FVM_Context *context, const FVM_Value *mailbox, FVM_Value *out)
{
    const struct timespec pause = {0, 1000000};
    for (unsigned i = 0; i < WAIT_MS; ++i)
    {
        if (fvm_object_call(context, mailbox->object, "tryReceive", 0, NULL, out) != FVM_OK)
        {
            return 0;
        }
        if (out->type != FVM_VALUE_UNDEFINED)
        {
            return out->type == FVM_VALUE_OBJECT;
        }
        nanosleep(&pause, NULL);
    }
    return 0;
}


/* [GC:] EN: Create a rooted local FIFO with the native fixture's initial integer.
 * RU: Создать удерживаемый локальный FIFO с начальным числом нативной проверки. */
static int seed(FVM_Context *context, FVM_GCRootScope *roots, FVM_Value *local)
{
    FVM_Value args[2], value, reply;
    fvm_value_number(args + 1, 2);
    fvm_value_number(&value, 81);
    return fvm_value_string(args, "i") == FVM_OK &&
        fvm_call_bytes(context, "newRpcChannel", 13, 2, args, local) == FVM_OK &&
        local->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, local) == FVM_OK &&
        fvm_object_call(context, local->object, "trySend", 1, &value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean;
}


/* [GC:] EN: Connect one mailbox to the exact result lease, then echo its admitted module.
 * RU: Связать очередь с точной арендой результата, затем вернуть принятый модуль. */
static int exchange(FVM_Context *context, FVM_GCRootScope *roots, struct StyxClient *client,
    StyxRpcServer *reverse, const FVM_Value *local)
{
    StyxRpcManagedTarget target = {
        client, "mounted/channel-lifetime.dis", "open", UINT32_C(0x1303311e),
        {"()", "((c[c[c[c[i]]]]c[i]))"}, CALL_PID, WAIT_MS
    };
    FVM_Value tuple, input, completion, args[3], mailbox, value, channel, alias, offer;
    if (fvm_styxrpc_channel_import(context, "openMailboxDis", &target) != FVM_OK ||
        fvm_styxrpc_channel_mailbox_factory(context, "openMailbox", reverse) != FVM_OK ||
        fvm_call_bytes(context, "openMailboxDis", 14, 0, NULL, &tuple) != FVM_OK ||
        tuple.type != FVM_VALUE_OBJECT || fvm_gc_roots_add(roots, &tuple) != FVM_OK ||
        fvm_object_get(context, tuple.object, "0", &input) != FVM_OK || input.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, tuple.object, "1", &completion) != FVM_OK || completion.type != FVM_VALUE_OBJECT)
    {
        return 0;
    }
    args[0] = input;
    fvm_value_number(args + 2, 1);
    if (fvm_value_string(args + 1, "(c[c[i]])") != FVM_OK ||
        fvm_call_bytes(context, "openMailbox", 11, 3, args, &mailbox) != FVM_OK ||
        mailbox.type != FVM_VALUE_OBJECT || fvm_gc_roots_add(roots, &mailbox) != FVM_OK ||
        fvm_object_call(context, mailbox.object, "tryReceive", 0, NULL, &value) != FVM_OK ||
        value.type != FVM_VALUE_UNDEFINED || !test_channel_avm1_send(context, &input, &mailbox) ||
        fvm_object_call(context, input.object, "close", 0, NULL, &value) != FVM_OK ||
        !poll(context, &mailbox, &channel) || fvm_gc_roots_add(roots, &channel) != FVM_OK ||
        !poll(context, &mailbox, &alias) || alias.object != channel.object ||
        fvm_object_call(context, mailbox.object, "trySend", 1, &channel, &value) != FVM_OK ||
        value.type != FVM_VALUE_BOOLEAN || !value.boolean)
    {
        fputs("Mailbox could not receive and echo the native module grant\n", stderr);
        return 0;
    }
    return fvm_styxrpc_channel_delegate_local(context, &channel, local, reverse, &offer) == FVM_OK &&
        fvm_gc_roots_add(roots, &offer) == FVM_OK && test_channel_avm1_send(context, &channel, &offer) &&
        fvm_object_call(context, completion.object, "receive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 81 &&
        fvm_object_call(context, mailbox.object, "close", 0, NULL, &value) == FVM_OK &&
        fvm_object_call(context, mailbox.object, "close", 0, NULL, &value) == FVM_OK &&
        fvm_object_call(context, mailbox.object, "tryReceive", 0, NULL, &value) == FVM_EXCEPTION &&
        fvm_exception_clear(context) == FVM_OK &&
        fvm_object_call(context, channel.object, "close", 0, NULL, &value) == FVM_OK &&
        fvm_object_call(context, completion.object, "close", 0, NULL, &value) == FVM_OK;
}


/* [GC:] EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_mailbox(FVM_Context *context, struct StyxClient *client, StyxRpcServer *reverse)
{
    FVM_GCRootScope *outer, *inner;
    FVM_Value local, value;
    if (fvm_gc_roots_begin(context, &outer) != FVM_OK)
    {
        return 0;
    }
    int ok = seed(context, outer, &local) && fvm_gc_roots_begin(context, &inner) == FVM_OK;
    if (ok)
    {
        ok = exchange(context, inner, client, reverse, &local);
        fvm_gc_roots_end(inner);
    }
    if (!ok && fvm_exception_get(context, &value) == FVM_OK && value.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "%s\n", value.string);
    }
    fvm_value_undefined(&value);
    int retired = fvm_set_global(context, "p", &value) == FVM_OK &&
        fvm_set_global(context, "c", &value) == FVM_OK && fvm_gc_collect(context, NULL) == FVM_OK;
    ok = ok && retired && fvm_object_call(context, local.object, "tryReceive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 43;
    fvm_gc_roots_end(outer);
    return ok;
}
