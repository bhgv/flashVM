/* Copyright (c) 2026 bhgv
 * EN: Run FlashVM script channels over native reverse PID and module grants on one carrier.
 * RU: Проверка каналов FlashVM с нативными встречными PID и токенами модуля на одном соединении.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel-test.h"
#include "flashvm/exception.h"
#include "flashvm/avm1.h"
#include "flashvm/gc.h"
#include "styxrpc/program.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAME 128U   /* Complete frames of the small recursive fixture. */
#define WAIT_MS 2000U /* Bounded waits for native operations and the reverse hub. */
#define CALL_PID 42U /* Stable hosted process identity for the scoped offer. */

#define ACTION_END 0x00            /* Finish the script operation. */
#define ACTION_GET_VARIABLE 0x1c   /* Load a fixture global. */
#define ACTION_SET_VARIABLE 0x1d   /* Store the send result in a fixture global. */
#define ACTION_CALL_METHOD 0x52    /* Invoke send on the script channel. */
#define ACTION_PUSH 0x96           /* Push a string or signed integer action value. */
#define PUSH_STRING 0U            /* NUL-terminated action string. */
#define PUSH_INT 7U               /* Four-byte little-endian action integer. */

static const StyxRpcChannelGrant hub = {1, "(c[c[i]])"}; /* Receives a module child and echoes it. */

typedef struct TestRoutes
{
    StyxRpcPrograms programs;          /* Reverse registrations and stable process tree. */
    StyxRpcRegistration nodes[20];    /* All slots used by the reverse tree. */
    StyxRpcServer *server;            /* Served on the worker and destroyed after join. */
    uint32_t hub_pid;                  /* Published PID for the module-message hub. */
    unsigned seen;                     /* Release/acquire count of exact module deliveries. */
    size_t extent;                     /* Immutable frame length after the first delivery. */
    unsigned char message[FRAME];      /* Captured exact module message from the native peer. */
} TestRoutes;

typedef struct HubOpen
{
    StyxRpcChannelFid fid;         /* One reverse operation on the test hub. */
    unsigned char request[FRAME]; /* Disjoint borrowed incoming bytes. */
    unsigned char reply[FRAME];   /* Disjoint immutable reply after completion. */
} HubOpen;


/* EN: Accept only a typed module grant for the hub's exact channel element.
 * RU: Допустить только полномочие модуля с точным типом элемента встречной шины. */
static int module_only(const void *unused, const StyxRpcChannelRoute *route)
{
    (void)unused;
    return route->kind == STYXRPC_ROUTE_MODULE && route->unbuffered &&
        !strcmp(route->channel.shape, "(c[i])");
}

static const StyxRpcChannelPolicy grants = {module_only, NULL}; /* Exact incoming module-grant profile. */


/* EN: Record two identical module deliveries, then echo them back to native Dis.
 * RU: Записать две одинаковые выдачи модуля и вернуть их в нативную Dis. */
static int hub_admit(void *state, StyxRpcChannelFid *fid)
{
    TestRoutes *routes = state;
    const StyxRpcGuard *guard = fid->selection.guards;
    unsigned count = __atomic_load_n(&routes->seen, __ATOMIC_RELAXED);
    if (fid->selection.count != 1)
    {
        return 0;
    }
    if (guard->receive)
    {
        return count == 2 &&
            styxrpc_channel_fid_finish(fid, 1, routes->message, routes->extent) == STYXRPC_OK;
    }
    if (count >= 2 || guard->size > sizeof(routes->message) ||
        (count && (guard->size != routes->extent || memcmp(guard->value, routes->message, guard->size))) ||
        styxrpc_channel_fid_finish(fid, 1, NULL, 0) != STYXRPC_OK)
    {
        return 0;
    }
    if (!count)
    {
        memcpy(routes->message, guard->value, guard->size);
        routes->extent = guard->size;
    }
    __atomic_store_n(&routes->seen, count + 1, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Own one distinct reverse hub fid and its separate buffers.
 * RU: Владеть отдельным встречным fid и раздельными буферами. */
static int hub_open(void *state, unsigned access, void **cookie)
{
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    HubOpen *opened = calloc(1, sizeof(*opened));
    if (!opened)
    {
        return 0;
    }
    opened->fid = (StyxRpcChannelFid){.grants = &hub, .grant_count = 1,
        .request = opened->request, .request_capacity = sizeof(opened->request),
        .reply = opened->reply, .reply_capacity = sizeof(opened->reply),
        .context = state, .admit = hub_admit, .policy = &grants};
    *cookie = opened;
    return 1;
}


/* EN: Accept one checked write prefix from the published PID.
 * RU: Принять один проверенный префикс записи с опубликованного PID. */
static int hub_write(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *used)
{
    HubOpen *opened = cookie;
    if (offset > SIZE_MAX || styxrpc_channel_fid_write(&opened->fid, (size_t)offset, bytes, count) == STYXRPC_INVALID)
    {
        return 0;
    }
    *used = count;
    return 1;
}


/* EN: Read a confirmed reverse reply after the request extent.
 * RU: Прочитать подтверждённый встречный ответ после длины запроса. */
static int hub_read(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *used)
{
    HubOpen *opened = cookie;
    size_t origin = opened->fid.staged;
    return offset >= origin && offset - origin <= SIZE_MAX &&
        styxrpc_channel_fid_read(&opened->fid, (size_t)(offset - origin), bytes, capacity, used) != STYXRPC_INVALID;
}


/* EN: Retire the exact hub fid before its buffers disappear.
 * RU: Отозвать точный fid встречной шины до освобождения буферов. */
static void hub_close(void *cookie)
{
    HubOpen *opened = cookie;
    styxrpc_channel_fid_close(&opened->fid);
    free(opened);
}


/* EN: Publish the fixed hub on routes before starting the reverse dispatcher.
 * RU: Зарегистрировать постоянную шину routes до запуска встречного диспетчера. */
static int publish(TestRoutes *routes)
{
    const StyxRpcResource resource = {routes, hub_open, hub_read, hub_write, hub_close};
    return styxrpc_programs_init(&routes->programs, routes->nodes, sizeof(routes->nodes) / sizeof(*routes->nodes)) &&
        styxrpc_program_add(&routes->programs, CALL_PID) &&
        (routes->hub_pid = styxrpc_program_unibus_grant(&routes->programs, &resource));
}


/* EN: Serve reverse requests for routes until disconnect, then return server ownership.
 * RU: Обслуживать встречные запросы routes до обрыва и вернуть владение сервером. */
static void *serve(void *state)
{
    TestRoutes *routes = state;
    while (styxrpc_server_step(routes->server))
    {
    }
    return NULL;
}


/* EN: Wait for the native peer's two verified module messages and decode the exact route.
 * RU: Дождаться двух проверенных сообщений партнёра и декодировать точный маршрут. */
static int module_route(const TestRoutes *routes, StyxRpcChannelRoute *route)
{
    const struct timespec pause = {0, 1000000};
    for (unsigned i = 0; i < WAIT_MS && __atomic_load_n(&routes->seen, __ATOMIC_ACQUIRE) != 2; ++i)
    {
        nanosleep(&pause, NULL);
    }
    StyxRpcChannelRoutes received;
    if (__atomic_load_n(&routes->seen, __ATOMIC_ACQUIRE) != 2 ||
        styxrpc_channel_element_routes(hub.shape, routes->message, routes->extent, &received) != STYXRPC_OK ||
        received.count != 1 || !module_only(NULL, received.entries))
    {
        return 0;
    }
    *route = received.entries[0];
    return 1;
}


/* [GC:] EN: Decode two references together, or reject a bad second token without a result.
 * RU: Декодировать две ссылки вместе либо отклонить неверный второй токен без результата. */
static int check_pair(FVM_Context *context, const FVM_Value *anchor, const StyxRpcChannelRoute *route,
    const FVM_Value *expected)
{
    unsigned char left[FRAME], right[FRAME], pair[FRAME], frame[FRAME];
    size_t first, second, length, extent;
    FVM_Value result, alias;
    StyxRpcChannelRoute other = *route;
    if (!expected)
    {
        other.token[0] ^= 128U;
    }
    fvm_value_number(&result, 19);
    if (styxrpc_channel_route_value(route, left, sizeof(left), &first) != STYXRPC_OK ||
        styxrpc_channel_route_value(&other, right, sizeof(right), &second) != STYXRPC_OK ||
        fvm_srecord_write_pair(STYXRPC_VALUE_TUPLE, left, first, right, second, pair, sizeof(pair), &length) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, pair, length, frame, sizeof(frame), &extent) != FVM_S_OK)
    {
        return 0;
    }
    FVM_Result status = fvm_styxrpc_channel_admit_message(context, anchor, "((c[c[i]]c[c[i]]))", frame, extent, &result);
    if (!expected)
    {
        return status == FVM_INVALID_ARGUMENT && result.type == FVM_VALUE_NUMBER && result.number == 19;
    }
    return status == FVM_OK && result.type == FVM_VALUE_OBJECT &&
        fvm_object_get(context, result.object, "0", &alias) == FVM_OK && alias.type == FVM_VALUE_OBJECT &&
        alias.object == expected->object &&
        fvm_object_get(context, result.object, "1", &alias) == FVM_OK && alias.type == FVM_VALUE_OBJECT &&
        alias.object == expected->object;
}


/* [GC:] EN: Load a live tuple and root it through the complete recursive scenario.
 * RU: Загрузить живой кортеж и удерживать его на весь рекурсивный сценарий. */
static int open_ports(FVM_Context *context, FVM_GCRootScope *roots, FVM_Value *tuple)
{
    return fvm_call_bytes(context, "openDis", 7, 0, NULL, tuple) == FVM_OK &&
        tuple->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, tuple) == FVM_OK;
}


/* EN: Read a bound channel or an idle error via the public AS method surface.
 * RU: Получить канал из кортежа либо ошибку через открытые методы AS. */
static int port(FVM_Context *context, const FVM_Value *tuple, const char *index, FVM_Value *value)
{
    return fvm_object_get(context, tuple->object, index, value) == FVM_OK && value->type == FVM_VALUE_OBJECT;
}


/* [GC:] EN: Execute s = c.send(p) using AVM1 and verify the confirmed Boolean.
 * RU: Выполнить s = c.send(p) в AVM1 и проверить подтверждённое булево значение. */
int test_channel_avm1_send(FVM_Context *context, const FVM_Value *object, const FVM_Value *value)
{
    static const unsigned char actions[] = {
        ACTION_PUSH, 3, 0, PUSH_STRING, 's', 0,
        ACTION_PUSH, 3, 0, PUSH_STRING, 'p', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 5, 0, PUSH_INT, 1, 0, 0, 0,
        ACTION_PUSH, 3, 0, PUSH_STRING, 'c', 0, ACTION_GET_VARIABLE,
        ACTION_PUSH, 6, 0, PUSH_STRING, 's', 'e', 'n', 'd', 0, ACTION_CALL_METHOD,
        ACTION_SET_VARIABLE, ACTION_END
    };
    FVM_Value result;
    return fvm_set_global(context, "c", object) == FVM_OK &&
        fvm_set_global(context, "p", value) == FVM_OK &&
        fvm_avm1_execute(context, actions, sizeof(actions)) == FVM_OK &&
        fvm_get_global(context, "s", &result) == FVM_OK &&
        result.type == FVM_VALUE_BOOLEAN && result.boolean;
}


/* [GC:] EN: On context, exercise two PIDs and a module token across client and routes.
 * Root temporary values in roots. Lend the caller's already rooted local FIFO.
 * RU: В context проверить два PID и токен модуля по client и routes.
 * Удержать значения в roots, заимствовать уже удерживаемую очередь local. */
static int exchange(FVM_Context *context, FVM_GCRootScope *roots, StyxClient *client,
    const TestRoutes *routes, const FVM_Value *local)
{
    StyxRpcManagedTarget target = {
        client, "mounted/channel-lifetime.dis", "open", UINT32_C(0x1303311e),
        {"()", "((c[c[c[c[i]]]]c[i]))"}, CALL_PID, WAIT_MS
    };
    const StyxRpcChannelRoute first = {.kind = STYXRPC_ROUTE_PID, .pid = routes->hub_pid, .channel = hub, .unbuffered = 1};
    StyxRpcChannelRoute module;
    FVM_Value tuple, other, input, result, unrelated, offered, channel, alias, value;
    if (fvm_styxrpc_channel_import(context, "openDis", &target) != FVM_OK ||
        !open_ports(context, roots, &tuple) ||
        !port(context, &tuple, "0", &input) || !port(context, &tuple, "1", &result))
    {
        fputs("Could not open the recursive channel lease\n", stderr);
        return 0;
    }
    target.name = "idle";
    if (!open_ports(context, roots, &other) ||
        !port(context, &other, "0", &unrelated) ||
        fvm_styxrpc_channel_delegate_pid(context, &input, &first, &offered) != FVM_OK ||
        fvm_gc_roots_add(roots, &offered) != FVM_OK)
    {
        fputs("Could not bind the hosted reverse PID to an open channel lease\n", stderr);
        return 0;
    }
    if (fvm_object_call(context, unrelated.object, "send", 1, &offered, &value) != FVM_EXCEPTION ||
        fvm_exception_clear(context) != FVM_OK)
    {
        fputs("A different lease did not reject the PID grant\n", stderr);
        return 0;
    }
    if (!test_channel_avm1_send(context, &input, &offered) || !module_route(routes, &module))
    {
        fputs("The first reverse PID did not return a module grant\n", stderr);
        return 0;
    }
    if (!check_pair(context, &input, &module, NULL) ||
        fvm_styxrpc_channel_admit_message(context, &input, hub.shape, routes->message, routes->extent, &channel) != FVM_OK ||
        fvm_gc_roots_add(roots, &channel) != FVM_OK ||
        fvm_styxrpc_channel_admit_message(context, &result, hub.shape, routes->message, routes->extent, &alias) != FVM_OK ||
        alias.object != channel.object || !check_pair(context, &input, &module, &channel) ||
        fvm_object_call(context, input.object, "close", 0, NULL, &value) != FVM_OK)
    {
        fputs("Could not canonicalize the separate module grant on its anchor\n", stderr);
        return 0;
    }
    module.unbuffered = 0;
    FVM_Value args[] = {channel, *local};
    if (fvm_styxrpc_channel_admit_module(context, &result, &module, &value) != FVM_INVALID_ARGUMENT ||
        fvm_call_bytes(context, "publishPort", 11, 2, args, &offered) != FVM_OK ||
        fvm_gc_roots_add(roots, &offered) != FVM_OK ||
        fvm_call_bytes(context, "publishPort", 11, 2, args, &value) != FVM_EXCEPTION ||
        fvm_exception_clear(context) != FVM_OK ||
        fvm_object_call(context, local->object, "tryReceive", 0, NULL, &value) != FVM_EXCEPTION ||
        fvm_exception_clear(context) != FVM_OK || !test_channel_avm1_send(context, &alias, &offered))
    {
        fputs("The second reverse PID could not be forwarded via the separate module token\n", stderr);
        return 0;
    }
    return fvm_object_call(context, result.object, "receive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 81 &&
        fvm_object_call(context, channel.object, "close", 0, NULL, &value) == FVM_OK &&
        fvm_object_call(context, result.object, "close", 0, NULL, &value) == FVM_OK;
}


/* [GC:] EN: Create a rooted two-slot FIFO in context and enqueue the value Dis must read.
 * RU: Создать удерживаемый в roots FIFO на два элемента и записать значение для Dis. */
static int seed(FVM_Context *context, FVM_GCRootScope *roots, FVM_Value *local)
{
    FVM_Value args[2], integer, reply;
    fvm_value_number(args + 1, 2);
    fvm_value_number(&integer, 81);
    return fvm_styxrpc_channel_factory(context, "newRpcChannel") == FVM_OK &&
        fvm_value_string_bytes(args, "i", 1) == FVM_OK &&
        fvm_call_bytes(context, "newRpcChannel", 13, 2, args, local) == FVM_OK &&
        local->type == FVM_VALUE_OBJECT && fvm_gc_roots_add(roots, local) == FVM_OK &&
        fvm_object_call(context, local->object, "trySend", 1, &integer, &reply) == FVM_OK &&
        reply.type == FVM_VALUE_BOOLEAN && reply.boolean;
}


/* [GC:] EN: Drop the issuing family while retaining the original queue, then read
 * the native peer's value from the exact same ActionScript object.
 * RU: Освободить семейство, сохранив исходную очередь, и прочитать значение
 * нативного партнёра из того же объекта ActionScript. */
static int run_exchange(FVM_Context *context, StyxClient *client, const TestRoutes *routes)
{
    FVM_GCRootScope *outer, *inner;
    FVM_Value local, value;
    unsigned long reclaimed;
    if (fvm_gc_roots_begin(context, &outer) != FVM_OK)
    {
        return 0;
    }
    int ok = seed(context, outer, &local) &&
        fvm_styxrpc_channel_publisher(context, "publishPort", routes->server) == FVM_OK &&
        fvm_gc_roots_begin(context, &inner) == FVM_OK;
    if (ok)
    {
        ok = exchange(context, inner, client, routes, &local);
        if (!ok && fvm_exception_get(context, &value) == FVM_OK && value.type == FVM_VALUE_STRING)
        {
            fprintf(stderr, "%s\n", value.string);
        }
        fvm_gc_roots_end(inner);
    }
    fvm_value_undefined(&value);
    int cleared = fvm_set_global(context, "c", &value) == FVM_OK &&
        fvm_set_global(context, "p", &value) == FVM_OK &&
        fvm_set_global(context, "openDis", &value) == FVM_OK &&
        fvm_set_global(context, "publishPort", &value) == FVM_OK;
    int collected = fvm_gc_collect(context, &reclaimed) == FVM_OK;
    ok = ok && cleared && collected &&
        fvm_object_call(context, local.object, "tryReceive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 43 &&
        fvm_object_call(context, local.object, "tryReceive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_UNDEFINED;
    fvm_gc_roots_end(outer);
    return ok;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_delegated(FVM_Context *context, int fd)
{
    TestRoutes routes = {0};
    pthread_t thread;
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (mux && publish(&routes))
    {
        routes.server = styxrpc_program_server_create(phybidirchannel(mux, 1), &routes.programs);
    }
    int started = routes.server && !pthread_create(&thread, NULL, serve, &routes);
    int ok = 0;
    if (started)
    {
        StyxClient *client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "flash", "", WAIT_MS, 8);
        ok = client && run_exchange(context, client, &routes) && test_channel_mailbox(context, client, routes.server);
        styxclientshutdown(client);
        styxclientfree(client);
        phybidirfree(mux);
        pthread_join(thread, NULL);
    }
    else
    {
        if (mux)
        {
            phybidirfree(mux);
        }
    }
    styxrpc_server_destroy(routes.server);
    if (!ok)
    {
        fputs("FlashVM delegated channel round trip failed\n", stderr);
    }
    return ok;
}
