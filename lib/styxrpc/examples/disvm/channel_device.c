/* Copyright (c) 2026 bhgv
 * EN: Native C provider operating on real channels owned by a remote Dis process.
 * RU: C-провайдер работает с живыми каналами удалённого процесса Dis.
 */
#include "styxrpc/channel_call.h"
#include "styxrpc/graph.h"
#include "styxrpc/serve.h"
#include "client.h"
#include "drivers/posix/fd.h"
#include "mux/bidir.h"
#include "load_profile.h"
#include "owned_port.h"
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static const StyxRpcSchema schema = {"(c[s]c[s]c[s])", "(i)"}; /* Compiled Bridge.exchange types. */

enum
{
    Root,      /* Session root. */
    Mounted,   /* Published modules. */
    Module,    /* Bridge module. */
    Interface, /* Compiled offer records. */
    Fields,    /* Graph version. */
    Version,   /* UniBus channel version. */
    Exchange,  /* A function taking caller-owned channels. */
    Port,      /* A function returning a C-owned channel. */
    Bus        /* Module-scoped UniBus for result leases. */
};

typedef struct Device
{
    StyxClient *reverse; /* Exact reverse connection borrowed through all function fids. */
    unsigned char profile[192]; /* Immutable .interface bytes. */
    size_t profile_size;        /* Byte extent of the profile. */
    DevicePort port;             /* Lease and queue owned by the dispatcher only. */
} Device;

typedef struct Call
{
    Device *device;              /* Borrowed publication state. */
    uint64_t id;                /* Static file or the dynamic call function. */
    unsigned char request[256];  /* Accumulated scoped request. */
    unsigned char response[96];  /* Immutable completed reply. */
    size_t staged;               /* Bytes received. */
    size_t ready;                /* Worker-owned reply length, published with outcome. */
    int outcome;                 /* Atomic completion indicator, positive on success. */
    int started;                 /* True when the dispatcher owns a worker. */
    int refused;                 /* Sticky malformed-request failure before scheduling. */
    pthread_t worker;            /* Joined before releasing this fid. */
    StyxRpcResource bus;         /* Module bus callbacks on Bus fids. */
    void *bus_cookie;            /* Opened channel operation on Bus fids. */
    int owns_port;               /* Revoke this result channel on close. */
} Call;

static const StyxRpcEntry files[] = { /* Exact Qids and names on the forward connection. */
    {Root, "/", 0555, 1}, {Mounted, "mounted", 0555, 1}, {Module, "bridge.dis", 0555, 1},
    {Interface, ".interface", 0444, 0}, {Fields, ".ref-fields", 0444, 0},
    {Version, ".unibus-version", 0444, 0}, {Exchange, "exchange", 0777, 0},
    {Port, "port", 0777, 0}, {Bus, ".unibus", 0666, 0}
};
static const uint64_t parents[] = {Root, Root, Mounted, Module, Module, Module, Module, Module, Module};


/* EN: Return the requested immediate child, or NULL at end.
 * RU: Вернуть нужного потомка либо NULL в конце. */
static const StyxRpcEntry *enumerate(void *context, uint64_t parent, size_t index)
{
    (void)context;
    for (size_t i = 1; i < sizeof(files) / sizeof(*files); ++i)
    {
        if (parents[i] == parent && !index--)
        {
            return files + i;
        }
    }
    return NULL;
}


/* EN: Return metadata for a current node, including exact capability-marker extents.
 * RU: Вернуть метаданные узла, включая точные размеры маркеров. */
static int describe(void *context, uint64_t id, StyxRpcNode *node)
{
    Device *device = context;
    if (id > Bus)
    {
        return 0;
    }
    uint64_t length = id == Interface ? device->profile_size : id == Fields ? sizeof(STYXRPC_POSITIONAL_GRAPH) - 1 :
        id == Version ? sizeof(STYXRPC_CHANNEL_VERSION) - 1 : 0;
    *node = (StyxRpcNode){files[id], parents[id], length, 0};
    return 1;
}


/* EN: Allocate distinct application state for each successful open.
 * RU: Выделить отдельное состояние приложения на каждое успешное открытие. */
static int open_file(void *context, uint64_t id, unsigned access, void **cookie)
{
    unsigned required = id >= Exchange ? STYXRPC_READ | STYXRPC_WRITE : STYXRPC_READ;
    if (access != required)
    {
        return 0;
    }
    Call *call = calloc(1, sizeof(*call));
    if (!call)
    {
        return 0;
    }
    call->device = context;
    call->id = id;
    if (id == Bus)
    {
        call->bus = device_port_resource(&call->device->port);
        if (!call->bus.open(call->bus.context, access, &call->bus_cookie))
        {
            free(call);
            return 0;
        }
    }
    *cookie = call;
    return 1;
}


/* EN: Read a slice of immutable metadata or this fid's reply.
 * RU: Прочитать часть неизменяемых метаданных или ответа этого fid. */
static int read_file(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    Call *call = cookie;
    if (call->id == Bus)
    {
        return call->bus.read(call->bus_cookie, offset, bytes, capacity, size);
    }
    int outcome = __atomic_load_n(&call->outcome, __ATOMIC_ACQUIRE);
    if (outcome < 0)
    {
        return 0;
    }
    const void *source = call->response;
    size_t length = outcome ? call->ready : 0;
    if (call->id == Interface)
    {
        source = call->device->profile;
        length = call->device->profile_size;
    }
    else if (call->id == Fields)
    {
        source = STYXRPC_POSITIONAL_GRAPH;
        length = sizeof(STYXRPC_POSITIONAL_GRAPH) - 1;
    }
    else if (call->id == Version)
    {
        source = STYXRPC_CHANNEL_VERSION;
        length = sizeof(STYXRPC_CHANNEL_VERSION) - 1;
    }
    size_t remain = offset < length ? length - (size_t)offset : 0;
    *size = capacity < remain ? capacity : remain;
    if (*size)
    {
        memcpy(bytes, (const unsigned char *)source + (size_t)offset, *size);
    }
    return 1;
}


/* EN: Run a guarded operation on the exact caller's generation using small fixture buffers.
 * RU: Выполнить ветвление на точном поколении клиента, используя малые буферы примера. */
static int operate(Call *call, const StyxRpcChannelCall *context, const StyxRpcSelection *selection,
    void *reply, size_t reply_size, StyxRpcChannelResult *result)
{
    unsigned char scratch[128], request[160];
    return styxrpc_channel_exchange(call->device->reverse, context, selection, 3000,
        scratch, sizeof(scratch), request, sizeof(request), reply, reply_size, result) == STYXRPC_OK;
}


/* EN: Try an idle channel, receive a live string through ALT, then send the response.
 * RU: Проверить пустой канал, принять строку через ALT, затем отправить ответ. */
static int run_exchange(Call *call, const StyxRpcChannelCall *context)
{
    const uint32_t input = context->grants[0].key, output = context->grants[1].key, idle = context->grants[2].key;
    const StyxRpcSelection try = {.mode = STYXRPC_CHANNEL_TRY, .count = 1, .guards = {{idle, 1, NULL, 0}}};
    const StyxRpcSelection alt = {.mode = STYXRPC_CHANNEL_ALT, .count = 2,
        .guards = {{idle, 1, NULL, 0}, {input, 1, NULL, 0}}};
    unsigned char reply[128], child[32], value[48];
    StyxRpcChannelResult result;
    size_t length, extent;
    FVM_SRecord outer, text;
    if (!operate(call, context, &try, reply, sizeof(reply), &result) || result.selected ||
        !operate(call, context, &alt, reply, sizeof(reply), &result) || result.selected != 2 ||
        fvm_srecord_read(result.value, result.size, sizeof(reply), &outer, &extent) != FVM_S_OK ||
        outer.tag != STYXRPC_CHANNEL_ELEMENT_TAG || extent != result.size ||
        fvm_srecord_read(outer.payload, outer.size, sizeof(reply), &text, &extent) != FVM_S_OK ||
        extent != outer.size || text.tag != STYXRPC_VALUE_STRING ||
        text.size != 8 || memcmp(text.payload, "from-dis", 8) ||
        fvm_srecord_write(STYXRPC_VALUE_STRING, "from-c", 6, child, sizeof(child), &length) != FVM_S_OK ||
        fvm_srecord_write(STYXRPC_CHANNEL_ELEMENT_TAG, child, length, value, sizeof(value), &extent) != FVM_S_OK)
    {
        return 0;
    }
    const StyxRpcSelection send = {.count = 1, .guards = {{output, 0, value, extent}}};
    return operate(call, context, &send, reply, sizeof(reply), &result) && result.selected == 1;
}


/* EN: Validate the complete call, use its channel grants, and emit an int result plus no updates.
 * RU: Проверить полный вызов, использовать полномочия и вернуть int с пустыми обновлениями. */
static int finish(Call *call)
{
    StyxRpcChannelCall context;
    if (styxrpc_channel_call(&schema, call->request, call->staged, &context) != STYXRPC_OK ||
        context.count != 3 || !run_exchange(call, &context))
    {
        return 0;
    }
    unsigned char child[24], result[32], updates[12];
    size_t length, result_size, update_size;
    return styxrpc_value_integer(STYXRPC_VALUE_INT, 42, child, sizeof(child), &length) == STYXRPC_OK &&
        fvm_srecord_write(STYX_DYNAMIC_REPLY_TAG, child, length, result, sizeof(result), &result_size) == FVM_S_OK &&
        fvm_srecord_write(STYXRPC_GRAPH_UPDATES_TAG, NULL, 0, updates, sizeof(updates), &update_size) == FVM_S_OK &&
        styxrpc_channel_call_reply(&schema, &context, result, result_size, updates, update_size,
            call->response, sizeof(call->response), &call->ready) == STYXRPC_OK;
}


/* EN: Service the reverse route separately so Dis can match its channel operations.
 * RU: Обслужить встречный канал отдельно, чтобы Dis успевал сопоставлять операции. */
static void *work(void *state)
{
    Call *call = state;
    int outcome = finish(call) ? 1 : -1;
    __atomic_store_n(&call->outcome, outcome, __ATOMIC_RELEASE);
    return NULL;
}


/* EN: Retire the worker before releasing its buffers and the call's reverse context.
 * RU: Дождаться рабочего потока перед освобождением буферов и контекста. */
static void close_file(void *state)
{
    Call *call = state;
    if (call->started)
    {
        pthread_join(call->worker, NULL);
    }
    if (call->owns_port)
    {
        device_port_revoke(&call->device->port);
    }
    if (call->id == Bus)
    {
        call->bus.close(call->bus_cookie);
    }
    free(call);
}


/* EN: Publish a returned channel from the exact currently opened function request.
 * RU: Опубликовать возвращаемый канал из точного запроса текущего fid. */
static int publish_port(Call *call)
{
    StyxRpcChannelCall context;
    if (styxrpc_channel_call(&port_schema, call->request, call->staged, &context) != STYXRPC_OK ||
        !device_port_publish(&call->device->port, &context,
            call->response, sizeof(call->response), &call->ready))
    {
        return 0;
    }
    call->owns_port = 1;
    __atomic_store_n(&call->outcome, 1, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Acknowledge one ordered request fragment; commit only a complete request.
 * RU: Подтвердить один последовательный фрагмент, выполнить только полный запрос. */
static int write_file(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *written)
{
    Call *call = cookie;
    if (call->id == Bus)
    {
        return call->bus.write(call->bus_cookie, offset, bytes, count, written);
    }
    if (call->id < Exchange || call->started || call->owns_port || call->refused || call->staged != offset ||
        !count || count > sizeof(call->request) - call->staged)
    {
        return 0;
    }
    memcpy(call->request + call->staged, bytes, count);
    call->staged += count;
    FVM_SRecord record;
    size_t extent;
    FVM_SResult status = fvm_srecord_read(call->request, call->staged, sizeof(call->request), &record, &extent);
    if (status == FVM_S_OK && extent == call->staged)
    {
        int done = call->id == Port ? publish_port(call) : !pthread_create(&call->worker, NULL, work, call);
        call->started = done && call->id == Exchange;
        call->refused = !done;
    }
    else
    {
        call->refused = status != FVM_S_MORE || call->staged == sizeof(call->request);
    }
    if (call->refused)
    {
        return 0;
    }
    *written = count;
    return 1;
}


/* EN: Consume one connected socket and serve through independent forward/reverse Styx sessions.
 * RU: Принять подключённый сокет и обслуживать прямую и встречную сессии Styx. */
int main(int argc, char **argv)
{
    char *end;
    long fd = argc == 2 ? strtol(argv[1], &end, 10) : -1;
    Device device = {0};
    if (fd < 3 || fd > INT_MAX || *end ||
        styxrpc_channel_profile("exchange", UINT32_C(0x0c61e66a), &schema,
            device.profile, sizeof(device.profile), &device.profile_size) != STYXRPC_OK)
    {
        return 2;
    }
    size_t extent;
    if (styxrpc_channel_profile("port", PORT_SIGNATURE, &port_schema,
        device.profile + device.profile_size, sizeof(device.profile) - device.profile_size, &extent) != STYXRPC_OK)
    {
        return 2;
    }
    device.profile_size += extent;
    PhyBidir *mux = phybidircreate(phyposixfd((int)fd, PHY_STREAM | PHY_ORDERED | PHY_RELIABLE), 65536);
    if (!mux)
    {
        return 1;
    }
    device.reverse = styxclientmountphy(phybidirchannel(mux, 1), 4096, "device", "", 1000, 8);
    static const StyxRpcTree tree = {{enumerate, open_file, read_file, write_file, close_file}, describe};
    StyxRpcServer *server = device.reverse ? styxrpc_server_create(phybidirchannel(mux, 0), &tree, &device) : NULL;
    while (styxrpc_server_step(server))
    {
    }
    int attached = server != NULL;
    styxrpc_server_destroy(server);
    styxclientshutdown(device.reverse);
    styxclientfree(device.reverse);
    phybidirfree(mux);
    return attached ? 0 : 1;
}
