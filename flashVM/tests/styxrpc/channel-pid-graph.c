/* Copyright (c) 2026 bhgv
 * EN: Exchange one leased graph of existing local queues with native Dis and restore it under GC.
 * RU: Обмен графом локальных очередей с нативной Dis в одной аренде и восстановление с GC.
 */
#include "channel-test.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include "styxrpc/program.h"
#include <pthread.h>
#include <stdio.h>

#define CALLER_PID 42U /* Process owning the reverse mount in this test. */
#define WAIT_MS 2500U /* Test budget for each native channel operation. */
#define OPEN_SIGNATURE UINT32_C(0x1303311e) /* Limbo open() with typed graph and completion channels. */


/* [GC:] EN: Create a queue with a compiled element shape and root the returned object.
 * RU: Создать очередь с компилированным типом и добавить объект под корни. */
static int queue(FVM_Context *context, FVM_GCRootScope *roots, const char *shape, unsigned capacity, FVM_Value *out)
{
    FVM_Value args[2];
    fvm_value_number(args + 1, capacity);
    return fvm_value_string(args, shape) == FVM_OK &&
        fvm_call_bytes(context, "newQueue", 8, 2, args, out) == FVM_OK &&
        out->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, out) == FVM_OK;
}


/* EN: Enqueue one value in queue and require its typed Boolean confirmation.
 * RU: Поставить значение в queue и сверить булево подтверждение. */
static int enqueue(FVM_Context *context, const FVM_Value *queue, const FVM_Value *value)
{
    FVM_Value reply;
    return fvm_object_call(context, queue->object, "trySend", 1, value, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean;
}


/* [GC:] EN: Seed two distinct nested FIFOs. Only root keeps their original objects alive.
 * RU: Заполнить два разных вложенных FIFO. Исходные объекты удерживает только root. */
static int seed(FVM_Context *context, const FVM_Value *root)
{
    FVM_GCRootScope *roots;
    FVM_Value first, second, fifo, rendezvous, number;
    if (fvm_gc_roots_begin(context, &roots) != FVM_OK)
    {
        return 0;
    }
    fvm_value_number(&number, 81);
    int ok = queue(context, roots, "c[i]", 4, &first) &&
        queue(context, roots, "c[i]", 1, &second) &&
        queue(context, roots, "i", 2, &fifo) && queue(context, roots, "i", 0, &rendezvous) &&
        enqueue(context, &fifo, &number) && enqueue(context, &first, &fifo) &&
        enqueue(context, &second, &rendezvous) && enqueue(context, root, &first) &&
        enqueue(context, root, &first) && enqueue(context, root, &second);
    fvm_gc_roots_end(roots);
    return ok;
}


/* [GC:] EN: A closed descendant rejects publication. The ancestor must still be usable.
 * RU: Закрытый потомок отклоняет публикацию. Предок должен оставаться доступным. */
static int rejects_closed(FVM_Context *context, FVM_GCRootScope *roots, StyxRpcServer *reverse, const FVM_Value *anchor)
{
    FVM_Value parent, child, result;
    return queue(context, roots, "c[i]", 1, &parent) &&
        queue(context, roots, "i", 1, &child) && enqueue(context, &parent, &child) &&
        fvm_object_call(context, child.object, "close", 0, NULL, &result) == FVM_OK &&
        fvm_styxrpc_channel_delegate_local(context, anchor, &parent, reverse, &result) == FVM_INVALID_ARGUMENT &&
        fvm_object_call(context, parent.object, "tryReceive", 0, NULL, &result) == FVM_OK &&
        result.type == FVM_VALUE_OBJECT && result.object == child.object;
}


/* [GC:] EN: Send one opaque reverse route from AVM1 and wait for native confirmation.
 * RU: Отправить встречный маршрут из AVM1 и дождаться нативного подтверждения. */
static int exchange(FVM_Context *context, FVM_GCRootScope *roots, StyxClient *client,
    StyxRpcServer *reverse, const FVM_Value *root)
{
    StyxRpcManagedTarget target = {
        client, "mounted/channel-pid-graph.dis", "open", OPEN_SIGNATURE,
        {"()", "((c[c[c[c[i]]]]c[i]))"}, CALLER_PID, WAIT_MS
    };
    FVM_Value pair, input, completion, args[2], port, result;
    if (fvm_styxrpc_channel_import(context, "openGraph", &target) != FVM_OK ||
        fvm_call_bytes(context, "openGraph", 9, 0, NULL, &pair) != FVM_OK ||
        pair.type != FVM_VALUE_OBJECT || fvm_gc_roots_add(roots, &pair) != FVM_OK ||
        fvm_object_get(context, pair.object, "0", &input) != FVM_OK || input.type != FVM_VALUE_OBJECT ||
        fvm_object_get(context, pair.object, "1", &completion) != FVM_OK || completion.type != FVM_VALUE_OBJECT)
    {
        fputs("Could not open the native graph and completion channels\n", stderr);
        return 0;
    }
    if (!rejects_closed(context, roots, reverse, &input))
    {
        fputs("Closed descendant did not roll back graph publication\n", stderr);
        return 0;
    }
    args[0] = input;
    args[1] = *root;
    if (fvm_call_bytes(context, "publishPort", 11, 2, args, &port) != FVM_OK ||
        fvm_gc_roots_add(roots, &port) != FVM_OK ||
        fvm_object_call(context, root->object, "tryReceive", 0, NULL, &result) != FVM_EXCEPTION ||
        fvm_exception_clear(context) != FVM_OK ||
        !test_channel_avm1_send(context, &input, &port))
    {
        fputs("Could not publish or send the nested reverse graph\n", stderr);
        return 0;
    }
    return fvm_object_call(context, completion.object, "receive", 0, NULL, &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 81 &&
        fvm_object_call(context, input.object, "close", 0, NULL, &result) == FVM_OK &&
        fvm_object_call(context, completion.object, "close", 0, NULL, &result) == FVM_OK;
}


/* [GC:] EN: Distinguish the peer's moved channel from the seeded FIFO. Collect after
 * dropping root's old edges so the new child must be traced through the retained graph.
 * RU: Отличить перенесённый канал от исходного FIFO. Собрать после удаления старых
 * ссылок root, чтобы новый дочерний канал удерживался общим графом. */
static int check_return(FVM_Context *context, FVM_GCRootScope *roots, const FVM_Value *root)
{
    FVM_Value first, rendezvous, alias, fifo, result;
    return fvm_object_call(context, root->object, "tryReceive", 0, NULL, &first) == FVM_OK &&
        first.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &first) == FVM_OK &&
        fvm_object_call(context, root->object, "tryReceive", 0, NULL, &result) == FVM_OK &&
        result.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(context, NULL) == FVM_OK &&
        fvm_object_call(context, first.object, "tryReceive", 0, NULL, &rendezvous) == FVM_OK &&
        rendezvous.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &rendezvous) == FVM_OK &&
        fvm_object_call(context, first.object, "tryReceive", 0, NULL, &fifo) == FVM_OK &&
        fifo.type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, &fifo) == FVM_OK &&
        fvm_object_call(context, first.object, "tryReceive", 0, NULL, &alias) == FVM_OK && alias.type == FVM_VALUE_OBJECT &&
        alias.object == rendezvous.object && fifo.object != rendezvous.object &&
        fvm_object_call(context, first.object, "tryReceive", 0, NULL, &result) == FVM_OK && result.type == FVM_VALUE_NULL &&
        fvm_object_call(context, rendezvous.object, "tryReceive", 0, NULL, &result) == FVM_OK &&
        result.type == FVM_VALUE_UNDEFINED &&
        fvm_object_call(context, fifo.object, "tryReceive", 0, NULL, &result) == FVM_OK &&
        result.type == FVM_VALUE_NUMBER && result.number == 43;
}


/* [GC:] EN: Retire the family through GC while only the root local queue remains rooted.
 * RU: Отозвать семейство через GC, оставив среди явных корней только локальную очередь. */
static int run(FVM_Context *context, StyxClient *client, StyxRpcServer *reverse)
{
    FVM_GCRootScope *outer, *inner;
    FVM_Value root, cleared;
    if (fvm_gc_roots_begin(context, &outer) != FVM_OK)
    {
        return 0;
    }
    int ok = fvm_styxrpc_channel_factory(context, "newQueue") == FVM_OK &&
        fvm_styxrpc_channel_publisher(context, "publishPort", reverse) == FVM_OK &&
        queue(context, outer, "c[c[i]]", 3, &root) && seed(context, &root) &&
        fvm_gc_collect(context, NULL) == FVM_OK && fvm_gc_roots_begin(context, &inner) == FVM_OK;
    if (ok)
    {
        ok = exchange(context, inner, client, reverse, &root);
        fvm_gc_roots_end(inner);
    }
    if (!ok && fvm_exception_get(context, &cleared) == FVM_OK && cleared.type == FVM_VALUE_STRING)
    {
        fprintf(stderr, "%s\n", cleared.string);
    }
    fvm_value_undefined(&cleared);
    int cleaned = fvm_set_global(context, "c", &cleared) == FVM_OK &&
        fvm_set_global(context, "p", &cleared) == FVM_OK && fvm_gc_collect(context, NULL) == FVM_OK;
    ok = ok && cleaned && check_return(context, outer, &root);
    fvm_gc_roots_end(outer);
    return ok;
}


/* EN: Serve reverse requests on their worker until transport shutdown.
 * RU: Обслуживать встречные запросы в рабочем потоке до остановки транспорта. */
static void *dispatch(void *server)
{
    while (styxrpc_server_step(server))
    {
    }
    return NULL;
}


/* [GC:] EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_pid_graph(FVM_Context *context, int fd)
{
    StyxRpcRegistration nodes[10];
    StyxRpcPrograms programs;
    StyxRpcServer *server = NULL;
    pthread_t thread;
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (mux && styxrpc_programs_init(&programs, nodes, sizeof(nodes) / sizeof(*nodes)) &&
        styxrpc_program_add(&programs, CALLER_PID))
    {
        server = styxrpc_program_server_create(phybidirchannel(mux, 1), &programs);
    }
    int started = server && !pthread_create(&thread, NULL, dispatch, server);
    int ok = 0;
    if (started)
    {
        StyxClient *client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "flash", "", WAIT_MS, 8);
        ok = client && run(context, client, server);
        styxclientshutdown(client);
        styxclientfree(client);
    }
    if (mux)
    {
        phybidirfree(mux);
    }
    if (started)
    {
        pthread_join(thread, NULL);
    }
    styxrpc_server_destroy(server);
    if (!ok || styxrpc_program_tree.files.enumerate(&programs, 1, 1))
    {
        fputs("Reverse PID graph did not round trip or retire\n", stderr);
        return 0;
    }
    return 1;
}
