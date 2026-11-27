/* Copyright (c) 2026 bhgv
 * EN: Check exact flush and neighboring RPC traffic for suspended channel and VFS requests.
 * RU: Проверить точный flush и соседний обмен для ожидающих запросов канала и VFS.
 */
#include <lib9.h>
#include <styx.h>
#include "client.h"
#include "styxrpc/channel_call.h"
#include "styxrpc/channel_route.h"
#include "styxrpc/named_channel.h"
#include "styxrpc/vfs_client.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define FRAME_SIZE 512U /* Negotiated maximum 9P frame size in this fixture. */

static char int_bytes[] = "\xff\xff\xff\xb7"; /* Signed big-endian -73. */

typedef enum Endpoint
{
    CallBus,      /* Non-token reverse PID bus. */
    ModuleBus,    /* Token-based module bus. */
    NamedChannel, /* Private enrollment file. */
    PlainFile,    /* Open ordinary file for a positioned transfer. */
    Int32File     /* The whole signed four-byte .var value. */
} Endpoint;


/* EN: The server owns wire counters. Only held and stopped cross thread boundaries.
 * RU: Счётчики протокола принадлежат серверу. Между потоками передаются held и stopped. */
typedef struct Wire
{
    int fd;              /* Server end of the socket pair, owned by peer. */
    Endpoint endpoint;   /* Expected client operation family. */
    int hold_type;       /* Original 9P request selected for suspension. */
    u32int target;       /* Fid of the bus or enrollment file being tested. */
    int reply_wins;      /* Emit the original data reply before Rflush. */
    int partial_file;    /* Acknowledge one prefix byte before suspending the suffix. */
    int held;            /* Atomic publication after saving the original tag. */
    int stopped;         /* Atomic stop flag set by the supervisory thread. */
    ushort tag;          /* Tag of the first suspended request. */
    u32int fid;          /* Fid of the first suspended request. */
    u32int survivor;     /* Independent open fid serving the control reads. */
    unsigned flushes;    /* Count of flushes addressing tag. */
    unsigned clunks;     /* Count of clunks retiring fid. */
    unsigned controls;   /* Count of confirmed control reads. */
    unsigned held_count; /* Count requested on a suspended transfer. */
    int clean;           /* Peer received EOF at a frame boundary. */
} Wire;


/* EN: Operation state belongs to the caller through join. Wire and fids are borrowed.
 * RU: Состояние операции принадлежит вызывающему до join. Wire и fid заимствуются. */
typedef struct Operation
{
    Wire *wire;           /* Shared protocol fixture. */
    StyxClient *client;   /* Borrowed channel session. */
    StyxRpcVfsFile *file; /* Optional open VFS file for bytes rather than a channel selection. */
    pthread_t caller;    /* Expected thread for every stop check. */
    int correct_thread;  /* All stop checks have run on caller. */
    int success;         /* Expected outcome was delivered without changing invalid outputs. */
} Operation;


/* EN: Encode and send a small reply on the blocking local fd. Return 1 if complete.
 * RU: Закодировать и отправить небольшой ответ через блокирующий fd. Вернуть 1 при успехе. */
static int send_reply(int fd, Fcall *reply)
{
    uchar frame[FRAME_SIZE];
    unsigned size = convS2M(reply, frame, sizeof(frame));
    return size && send(fd, frame, size, MSG_NOSIGNAL) == (ssize_t)size;
}


/* EN: Save the exact request tag and fid in wire before signaling the supervisor.
 * RU: Сохранить точный тег и fid в wire перед сигналом управляющему потоку. */
static int hold(Wire *wire, const Fcall *request, u32int fid)
{
    if (__atomic_load_n(&wire->held, __ATOMIC_RELAXED))
    {
        return 0;
    }
    wire->tag = request->tag;
    wire->fid = fid;
    wire->held_count = request->type == Tread || request->type == Twrite ? request->count : 0;
    __atomic_store_n(&wire->held, 1, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Process one decoded request on wire; return 0 on an unexpected frame or send failure.
 * RU: Обработать один запрос на wire. Вернуть 0 при неожиданном кадре или сбое отправки. */
static int answer(Wire *wire, const Fcall *request)
{
    Fcall reply = {.type = request->type + 1, .tag = request->tag};
    switch (request->type)
    {
    case Tversion:
        reply.msize = FRAME_SIZE;
        reply.version = VERSION9P;
        break;
    case Tattach:
        reply.qid = (Qid){.type = QTDIR, .path = 0};
        break;
    case Twalk:
        reply.nwqid = request->nwname;
        for (unsigned i = 0; i < request->nwname; ++i)
        {
            reply.wqid[i] = (Qid){.type = i + 1 < request->nwname ? QTDIR : QTFILE, .path = i + 1};
        }
        if (request->nwname == 1 && !strcmp(request->wname[0], "survivor"))
        {
            wire->survivor = request->newfid;
        }
        if (request->nwname && (!strcmp(request->wname[request->nwname - 1], ".unibus") ||
            !strcmp(request->wname[request->nwname - 1], STYXRPC_NAMED_SERVICE)))
        {
            wire->target = request->newfid;
            if (wire->hold_type == Twalk)
            {
                return hold(wire, request, wire->target);
            }
        }
        break;
    case Topen:
        if (wire->hold_type == Topen && request->fid == wire->target)
        {
            return hold(wire, request, request->fid);
        }
        reply.qid = (Qid){.type = QTFILE, .path = 1};
        break;
    case Tread:
    case Twrite:
        if (wire->endpoint == NamedChannel && request->fid == wire->target && request->type == Twrite)
        {
            if (request->offset == 0 && wire->hold_type == Twrite)
            {
                const unsigned char expected[] = {STYXRPC_NAMED_OPEN, 0, 0, 0, 5};
                if (request->count != sizeof(expected) || memcmp(request->data, expected, sizeof(expected)))
                {
                    return 0;
                }
                reply.count = request->count;
                break;
            }
            return wire->hold_type == Twrite && request->offset == STYXRPC_NAMED_HEADER &&
                request->count == 5 && !memcmp(request->data, "/test", 5) && hold(wire, request, request->fid);
        }
        if (wire->endpoint == ModuleBus && request->fid == wire->target && request->type == Twrite)
        {
            return wire->hold_type == Twrite && !request->offset &&
                request->count == STYXRPC_CHANNEL_TOKEN_SIZE && request->data[0] == 7 &&
                hold(wire, request, request->fid);
        }
        if (request->fid == wire->survivor && request->type == Tread)
        {
            ++wire->controls;
            reply.count = 1;
            reply.data = "x";
        }
        else if (wire->endpoint == Int32File)
        {
            return request->type == wire->hold_type && !request->offset && request->count == 4 &&
                (request->type == Tread || !memcmp(request->data, int_bytes, 4)) &&
                hold(wire, request, request->fid);
        }
        else if (wire->partial_file && request->type == wire->hold_type && !request->offset && request->count == 2)
        {
            if (request->type == Twrite && memcmp(request->data, "wz", 2))
            {
                return 0;
            }
            reply.count = 1;
            reply.data = "r";
        }
        else if (request->type == wire->hold_type && !__atomic_load_n(&wire->held, __ATOMIC_RELAXED) &&
            (!wire->partial_file || (request->offset == 1 && request->count == 1)))
        {
            if (request->type == Twrite && (wire->partial_file || wire->reply_wins) &&
                (request->count != 1 || request->data[0] != (wire->partial_file ? 'z' : 'w')))
            {
                return 0;
            }
            return hold(wire, request, request->fid);
        }
        else if (request->type == Twrite && wire->hold_type == Tread)
        {
            reply.count = request->count;
        }
        else
        {
            return 0;
        }
        break;
    case Tflush:
        if (!__atomic_load_n(&wire->held, __ATOMIC_RELAXED) || request->oldtag != wire->tag ||
            wire->flushes || wire->controls != 1)
        {
            return 0;
        }
        ++wire->flushes;
        if (wire->reply_wins)
        {
            Fcall original = {.type = wire->hold_type + 1, .tag = wire->tag,
                .count = wire->held_count, .data = wire->endpoint == Int32File ? int_bytes : "r"};
            if (!send_reply(wire->fd, &original))
            {
                return 0;
            }
        }
        break;
    case Tclunk:
        if (wire->fid && request->fid == wire->fid)
        {
            if (wire->flushes != 1)
            {
                return 0;
            }
            ++wire->clunks;
        }
        break;
    default:
        return 0;
    }
    return send_reply(wire->fd, &reply);
}


/* EN: Own the peer fd and serve complete frames until orderly client close.
 * RU: Владеть fd сервера и обслуживать полные кадры до закрытия клиента. */
static void *peer(void *state)
{
    Wire *wire = state;
    uchar frame[FRAME_SIZE];
    ssize_t received;
    while ((received = recv(wire->fd, frame, 4, MSG_WAITALL)) == 4)
    {
        Fcall request;
        unsigned size = GBIT32(frame);
        if (size < 7 || size > sizeof(frame) ||
            recv(wire->fd, frame + 4, size - 4, MSG_WAITALL) != (ssize_t)(size - 4) ||
            convM2S(frame, size, &request) != size || !answer(wire, &request))
        {
            break;
        }
    }
    wire->clean = received == 0;
    close(wire->fd);
    return NULL;
}


/* EN: Read the supervisor's stop flag and record which thread checked it.
 * RU: Прочитать флаг остановки и проверить поток, вызвавший колбэк. */
static int stop(void *state)
{
    Operation *operation = state;
    operation->correct_thread &= pthread_equal(pthread_self(), operation->caller) != 0;
    return __atomic_load_n(&operation->wire->stopped, __ATOMIC_ACQUIRE);
}


/* EN: Verify one signed Int32 transfer and preserve the read output after cancellation.
 * RU: Проверить передачу знакового Int32 и сохранить результат чтения при отмене. */
static int int_result(Operation *operation)
{
    if (operation->wire->hold_type == Twrite)
    {
        StyxRpcIoOutcome status = styxrpc_vfs_int_write_until(operation->file, -73, stop, operation);
        return status == (operation->wire->reply_wins ? STYXRPC_IO_OK : STYXRPC_IO_UNCERTAIN);
    }
    int32_t value = 123;
    StyxRpcIoOutcome status = styxrpc_vfs_int_read_until(operation->file, &value, stop, operation);
    return operation->wire->reply_wins ? status == STYXRPC_IO_OK && value == -73 :
        status == STYXRPC_IO_CANCELLED && value == 123;
}


/* EN: Wait on a data frame. The peer leaves it pending until the stop hook requests Tflush.
 * RU: Ждать кадр данных, который сервер удерживает до Tflush по колбэку. */
static void *call(void *state)
{
    Operation *operation = state;
    operation->caller = pthread_self();
    operation->correct_thread = 1;
    if (operation->wire->endpoint == Int32File)
    {
        operation->success = int_result(operation);
        return NULL;
    }
    if (operation->file)
    {
        char bytes[2] = {'w', 'z'};
        size_t used;
        int partial = operation->wire->partial_file;
        int writing = operation->wire->hold_type == Twrite;
        StyxRpcIoOutcome result = writing ?
            styxrpc_vfs_file_write_until(operation->file, 0, bytes, partial ? 2 : 1, &used, stop, operation) :
            styxrpc_vfs_file_read_until(operation->file, 0, bytes, partial ? 2 : 1, &used, stop, operation);
        operation->success = result == (partial ? STYXRPC_IO_CANCELLED : STYXRPC_IO_OK) &&
            used == 1 && bytes[0] == (writing ? 'w' : 'r') && bytes[1] == 'z';
        return NULL;
    }
    if (operation->wire->endpoint == NamedChannel)
    {
        StyxRpcVfsFile *file = NULL;
        StyxRpcIoOutcome status = styxrpc_named_open_until(operation->client, "/test", &file, stop, operation);
        int acknowledged = operation->wire->reply_wins;
        operation->success = acknowledged ? status == STYXRPC_IO_OK && file : status == STYXRPC_IO_CANCELLED && !file;
        styxrpc_vfs_close(file, 0);
        return NULL;
    }
    StyxRpcChannelCall channel = {.pid = 42, .count = 1, .grants = {{1, "(i)"}}};
    const StyxRpcSelection selection = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    StyxRpcChannelResult result = {.selected = 99};
    unsigned char request[96], reply[96];
    const StyxRpcChannelRoute route = {.kind = STYXRPC_ROUTE_MODULE, .token = {7}, .channel = {1, "(i)"}};
    int module = operation->wire->endpoint == ModuleBus;
    StyxRpcStatus status = module ?
        styxrpc_channel_route_exchange_until(operation->client, "module/.unibus", &route, &selection, 4000,
            NULL, 0, request, sizeof(request), reply, sizeof(reply), &result, stop, operation) :
        styxrpc_channel_exchange_until(operation->client, &channel, &selection, 4000,
            NULL, 0, request, sizeof(request), reply, sizeof(reply), &result, stop, operation);
    int stopped = __atomic_load_n(&operation->wire->stopped, __ATOMIC_ACQUIRE);
    StyxRpcStatus expected = STYXRPC_INVALID;
    if (stopped)
    {
        expected = module || operation->wire->hold_type == Twalk || operation->wire->hold_type == Topen ?
            STYXRPC_CANCELLED : STYXRPC_CANCELLED_UNCERTAIN;
    }
    operation->success = status == expected && result.selected == 99;
    return NULL;
}


/* EN: Open a borrowed session's file and return an owned fid or NULL.
 * RU: Открыть файл заимствованной сессии и вернуть собственный fid либо NULL. */
static StyxFid *opened(StyxClient *client, const char *path)
{
    StyxFid *fid = styxclientwalk(client, path);
    if (fid && styxclientopen(fid, 2))
    {
        return fid;
    }
    styxclientclunk(fid);
    return NULL;
}


/* EN: Confirm a one-byte ordinary RPC on a borrowed survivor fid.
 * RU: Подтвердить обычный однобайтовый запрос на заимствованном соседнем fid. */
static int control(StyxFid *fid)
{
    char byte = 0;
    StyxTransfer result;
    styxclientread(fid, 0, &byte, 1, &result);
    return result.success && result.count == 1 && byte == 'x';
}


/* EN: Hold a frame of type on endpoint with explicit request_stop or normal transport timeout.
 * reply_wins answers a held VFS request, partial_file confirms its first byte.
 * Keep one independent fid responsive before and after the exact flush.
 * RU: Удержать кадр type на endpoint до request_stop или обычного таймаута транспорта.
 * reply_wins отвечает на удержанный запрос VFS, partial_file подтверждает префикс.
 * Независимый fid должен отвечать до и после точного flush. */
static int exercise(Endpoint endpoint, int type, int reply_wins, int request_stop, int partial_file)
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 0;
    }
    Wire wire = {.fd = sockets[1], .endpoint = endpoint, .hold_type = type,
        .reply_wins = reply_wins, .partial_file = partial_file};
    pthread_t server, worker;
    if (pthread_create(&server, NULL, peer, &wire))
    {
        close(sockets[0]);
        close(sockets[1]);
        return 0;
    }
    StyxClient *client = styxclientmountfd(sockets[0], FRAME_SIZE, "test", "", request_stop ? 4000 : 500, 4);
    StyxFid *survivor = client ? opened(client, "survivor") : NULL;
    int file_transfer = endpoint == PlainFile || endpoint == Int32File;
    StyxRpcVfsFile *file = client && file_transfer ? opened(client, "data") : NULL;
    Operation operation = {.wire = &wire, .client = client, .file = file};
    int passed = survivor && (!file_transfer || file) && !pthread_create(&worker, NULL, call, &operation);
    if (passed)
    {
        const struct timespec pause = {0, 1000000};
        for (unsigned i = 0; i < 300 && !__atomic_load_n(&wire.held, __ATOMIC_ACQUIRE); ++i)
        {
            nanosleep(&pause, NULL);
        }
        passed = __atomic_load_n(&wire.held, __ATOMIC_ACQUIRE) && control(survivor);
        __atomic_store_n(&wire.stopped, request_stop, __ATOMIC_RELEASE);
        pthread_join(worker, NULL);
        passed = passed && operation.correct_thread && operation.success && styxclientalive(client) && control(survivor);
    }
    styxrpc_vfs_close(file, 0);
    styxclientclunk(survivor);
    styxclientfree(client);
    pthread_join(server, NULL);
    int ok = passed && wire.clean && wire.flushes == 1 && wire.clunks == 1 && wire.controls == 2;
    if (!ok)
    {
        fprintf(stderr, "endpoint %d frame %d: completed %d, clean %d, flush %u, clunk %u, control %u\n",
            endpoint, type, passed, wire.clean, wire.flushes, wire.clunks, wire.controls);
    }
    return ok;
}


/* EN: Check stalled channel frames, VFS reply precedence and confirmed prefix, and transport timeout.
 * RU: Проверить ожидание кадров канала, приоритет ответа и префикс VFS, таймаут транспорта. */
int main(void)
{
    if (!exercise(CallBus, Tread, 0, 1, 0) || !exercise(CallBus, Twrite, 0, 1, 0) ||
        !exercise(CallBus, Tread, 0, 0, 0) || !exercise(CallBus, Twalk, 0, 1, 0) ||
        !exercise(CallBus, Topen, 0, 1, 0) || !exercise(ModuleBus, Twrite, 0, 1, 0) ||
        !exercise(ModuleBus, Twrite, 1, 1, 0) || !exercise(NamedChannel, Twalk, 0, 1, 0) ||
        !exercise(NamedChannel, Topen, 0, 1, 0) || !exercise(NamedChannel, Twrite, 0, 1, 0) ||
        !exercise(NamedChannel, Twrite, 1, 1, 0) || !exercise(PlainFile, Tread, 1, 1, 0) ||
        !exercise(PlainFile, Tread, 0, 1, 1) || !exercise(PlainFile, Twrite, 1, 1, 0) ||
        !exercise(PlainFile, Twrite, 0, 1, 1) ||
        !exercise(Int32File, Tread, 0, 1, 0) || !exercise(Int32File, Tread, 1, 1, 0) ||
        !exercise(Int32File, Twrite, 0, 1, 0) || !exercise(Int32File, Twrite, 1, 1, 0))
    {
        fprintf(stderr, "channel stop wire: held RPC, flush or neighboring RPC failed\n");
        return 1;
    }
    return 0;
}
