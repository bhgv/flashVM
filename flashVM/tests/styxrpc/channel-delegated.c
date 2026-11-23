/* Copyright (c) 2026 bhgv
 * EN: Run FlashVM script channels over native reverse PID and module grants on one carrier.
 * RU: Проверка каналов FlashVM с нативными встречными PID и токенами модуля на одном соединении.
 */
#define _POSIX_C_SOURCE 200809L
#include "channel-test.h"
#include "flashvm/exception.h"
#include "flashvm/avm1.h"
#include "styxrpc/channel_group.h"
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
static const StyxRpcChannelGrant leaf = {2, "(i)"};       /* Final exchange through a second PID. */

typedef struct TestRoutes
{
    StyxRpcPrograms programs;          /* Reverse registrations and stable process tree. */
    StyxRpcRegistration nodes[20];    /* All slots used by the reverse tree. */
    StyxRpcChannelOutput endpoint;    /* Owned reverse FIFO kept through server retirement. */
    StyxRpcServer *server;             /* Owned by the dispatcher after launch. */
    uint32_t hub_pid;                  /* Published PID for the module-message hub. */
    uint32_t leaf_pid;                 /* Published PID for the independent integer channel. */
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


/* EN: Publish both PID routes before starting the serialized dispatcher.
 * RU: Опубликовать оба маршрута PID до запуска сериализованного диспетчера. */
static int publish(TestRoutes *routes)
{
    unsigned char arena[2][FRAME];
    size_t lengths[2];
    const StyxRpcChannelQueue queue = {.grant = leaf, .bytes = arena[0],
        .sizes = lengths, .capacity = 2, .stride = FRAME, .count = 1};
    const StyxRpcResource resource = {routes, hub_open, hub_read, hub_write, hub_close};
    return styxrpc_value_integer(STYXRPC_VALUE_INT, 81, arena[0], sizeof(arena[0]), lengths) == STYXRPC_OK &&
        styxrpc_channel_group_reverse_output(&queue, 1, 0, &routes->endpoint) == STYXRPC_OK &&
        styxrpc_programs_init(&routes->programs, routes->nodes, sizeof(routes->nodes) / sizeof(*routes->nodes)) &&
        styxrpc_program_add(&routes->programs, CALL_PID) &&
        (routes->hub_pid = styxrpc_program_unibus_grant(&routes->programs, &resource)) &&
        (routes->leaf_pid = styxrpc_program_unibus_grant(&routes->programs, &routes->endpoint.resource));
}


/* EN: Serialize reverse requests and close every fid before returning ownership.
 * RU: Сериализовать встречные запросы и закрыть все fid перед возвратом владения. */
static void *serve(void *state)
{
    TestRoutes *routes = state;
    while (styxrpc_server_step(routes->server))
    {
    }
    styxrpc_server_destroy(routes->server);
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
static int send(FVM_Context *context, const FVM_Value *object, const FVM_Value *value)
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


/* [GC:] EN: Exercise two reverse PIDs and a separate module token through AS channel methods.
 * RU: Проверить два встречных PID и отдельный токен модуля через методы каналов AS. */
static int exchange(FVM_Context *context, FVM_GCRootScope *roots, StyxClient *client, const TestRoutes *routes)
{
    StyxRpcManagedTarget target = {
        client, "mounted/channel-lifetime.dis", "open", UINT32_C(0x1303311e),
        {"()", "((c[c[c[c[i]]]]c[i]))"}, CALL_PID, WAIT_MS
    };
    const StyxRpcChannelRoute first = {.kind = STYXRPC_ROUTE_PID, .pid = routes->hub_pid, .channel = hub, .unbuffered = 1};
    const StyxRpcChannelRoute second = {.kind = STYXRPC_ROUTE_PID, .pid = routes->leaf_pid, .channel = leaf};
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
    if (!send(context, &input, &offered) || !module_route(routes, &module))
    {
        fputs("The first reverse PID did not return a module grant\n", stderr);
        return 0;
    }
    if (fvm_styxrpc_channel_admit_module(context, &input, &module, &channel) != FVM_OK ||
        fvm_gc_roots_add(roots, &channel) != FVM_OK ||
        fvm_styxrpc_channel_admit_module(context, &result, &module, &alias) != FVM_OK ||
        alias.object != channel.object || fvm_object_call(context, input.object, "close", 0, NULL, &value) != FVM_OK)
    {
        fputs("Could not canonicalize the separate module grant on its anchor\n", stderr);
        return 0;
    }
    module.unbuffered = 0;
    if (fvm_styxrpc_channel_admit_module(context, &result, &module, &value) != FVM_INVALID_ARGUMENT ||
        fvm_styxrpc_channel_delegate_pid(context, &channel, &second, &offered) != FVM_OK ||
        !send(context, &alias, &offered))
    {
        fputs("The second reverse PID could not be forwarded via the separate module token\n", stderr);
        return 0;
    }
    return fvm_object_call(context, result.object, "receive", 0, NULL, &value) == FVM_OK &&
        value.type == FVM_VALUE_NUMBER && value.number == 81;
}


/* EN: Verify the integer Dis put through the independent PID, after joining the dispatcher.
 * RU: Проверить int, отправленный Dis через отдельный PID, после остановки диспетчера. */
static int leaf_received(const TestRoutes *routes)
{
    const StyxRpcResource *resource = &routes->endpoint.resource;
    const StyxRpcSelection selection = {.count = 1, .guards = {{2, 1, NULL, 0}}};
    unsigned char frame[FRAME], reply[FRAME];
    size_t length, extent;
    StyxRpcChannelResult result;
    void *cookie = NULL;
    int valid = styxrpc_channel_request(&leaf, 1, &selection, NULL, 0, frame, sizeof(frame), &length) == STYXRPC_OK &&
        resource->open(resource->context, STYXRPC_READ | STYXRPC_WRITE, &cookie) &&
        resource->write(cookie, 0, frame, length, &extent) && extent == length &&
        resource->read(cookie, length, reply, sizeof(reply), &extent) &&
        styxrpc_channel_result(&leaf, 1, &selection, reply, extent, &result) == STYXRPC_OK && result.integer == 43;
    if (cookie)
    {
        resource->close(cookie);
    }
    return valid;
}


/* EN/RU: See channel-test.h. / См. channel-test.h. */
int test_channel_delegated(FVM_Context *context, int fd)
{
    TestRoutes routes = {0};
    pthread_t thread;
    PhyBidir *mux = phybidircreate(phyposixfd(fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (mux && publish(&routes))
    {
        routes.server = styxrpc_server_create(phybidirchannel(mux, 1), &styxrpc_program_tree, &routes.programs);
    }
    int started = routes.server && !pthread_create(&thread, NULL, serve, &routes);
    int ok = 0;
    if (started)
    {
        StyxClient *client = styxclientmountphy(phybidirchannel(mux, 0), 4096, "flash", "", WAIT_MS, 8);
        FVM_GCRootScope *roots;
        if (client && fvm_gc_roots_begin(context, &roots) == FVM_OK)
        {
            ok = exchange(context, roots, client, &routes);
            FVM_Value error;
            if (!ok && fvm_exception_get(context, &error) == FVM_OK && error.type == FVM_VALUE_STRING)
            {
                fprintf(stderr, "%s\n", error.string);
            }
            fvm_gc_roots_end(roots);
        }
        styxclientshutdown(client);
        styxclientfree(client);
        phybidirfree(mux);
        pthread_join(thread, NULL);
        ok = ok && leaf_received(&routes);
    }
    else
    {
        styxrpc_server_destroy(routes.server);
        if (mux)
        {
            phybidirfree(mux);
        }
    }
    if (routes.endpoint.release)
    {
        routes.endpoint.release(routes.endpoint.resource.context);
    }
    if (!ok)
    {
        fputs("FlashVM delegated channel round trip failed\n", stderr);
    }
    return ok;
}
