/* Copyright (c) 2026 bhgv
 * EN: Check exact flush and neighboring RPC traffic for suspended channel and VFS requests.
 * RU: Проверить точный flush и соседний обмен для ожидающих запросов канала и VFS.
 */
#include <lib9.h>
#include <styx.h>
#include "client.h"
#include "styxrpc/channel_call.h"
#include "styxrpc/vfs_client.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define FRAME_SIZE 512U /* Negotiated maximum 9P frame size in this fixture. */


/* EN: The server owns wire counters. Only held and stopped cross thread boundaries.
 * RU: Счётчики протокола принадлежат серверу. Между потоками передаются held и stopped. */
typedef struct Wire
{
    int fd;              /* Server end of the socket pair, owned by peer. */
    int hold_type;       /* Tread or Twrite selected for suspension. */
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
        break;
    case Topen:
        reply.qid = (Qid){.type = QTFILE, .path = 1};
        break;
    case Tread:
    case Twrite:
        if (request->fid == wire->survivor && request->type == Tread)
        {
            ++wire->controls;
            reply.count = 1;
            reply.data = "x";
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
            wire->tag = request->tag;
            wire->fid = request->fid;
            __atomic_store_n(&wire->held, 1, __ATOMIC_RELEASE);
            return 1;
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
            Fcall original = {.type = wire->hold_type + 1, .tag = wire->tag, .count = 1, .data = "r"};
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


/* EN: Wait on a data frame. The peer leaves it pending until the stop hook requests Tflush.
 * RU: Ждать кадр данных, который сервер удерживает до Tflush по колбэку. */
static void *call(void *state)
{
    Operation *operation = state;
    operation->caller = pthread_self();
    operation->correct_thread = 1;
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
    StyxRpcChannelCall channel = {.pid = 42, .count = 1, .grants = {{1, "(i)"}}};
    const StyxRpcSelection selection = {.count = 1, .guards = {{1, 1, NULL, 0}}};
    StyxRpcChannelResult result = {.selected = 99};
    unsigned char request[96], reply[96];
    StyxRpcStatus status = styxrpc_channel_exchange_until(operation->client, &channel, &selection, 4000,
        NULL, 0, request, sizeof(request), reply, sizeof(reply), &result, stop, operation);
    int stopped = __atomic_load_n(&operation->wire->stopped, __ATOMIC_ACQUIRE);
    operation->success = status == (stopped ? STYXRPC_CANCELLED_UNCERTAIN : STYXRPC_INVALID) && result.selected == 99;
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


/* EN: Hold a frame of type with explicit request_stop or normal transport timeout.
 * reply_wins answers a held VFS request, partial_file confirms its first byte.
 * Keep one independent fid responsive before and after the exact flush.
 * RU: Удержать кадр type до request_stop или обычного таймаута транспорта.
 * reply_wins отвечает на удержанный запрос VFS, partial_file подтверждает префикс.
 * Независимый fid должен отвечать до и после точного flush. */
static int exercise(int type, int reply_wins, int request_stop, int partial_file)
{
    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets))
    {
        return 0;
    }
    Wire wire = {.fd = sockets[1], .hold_type = type, .reply_wins = reply_wins, .partial_file = partial_file};
    pthread_t server, worker;
    if (pthread_create(&server, NULL, peer, &wire))
    {
        close(sockets[0]);
        close(sockets[1]);
        return 0;
    }
    StyxClient *client = styxclientmountfd(sockets[0], FRAME_SIZE, "test", "", request_stop ? 4000 : 500, 4);
    StyxFid *survivor = client ? opened(client, "survivor") : NULL;
    int file_transfer = reply_wins || partial_file;
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
    return passed && wire.clean && wire.flushes == 1 && wire.clunks == 1 && wire.controls == 2;
}


/* EN: Check stalled channel frames, VFS reply precedence and confirmed prefix, and transport timeout.
 * RU: Проверить ожидание кадров канала, приоритет ответа и префикс VFS, таймаут транспорта. */
int main(void)
{
    if (!exercise(Tread, 0, 1, 0) || !exercise(Twrite, 0, 1, 0) ||
        !exercise(Tread, 1, 1, 0) || !exercise(Tread, 0, 0, 0) || !exercise(Tread, 0, 1, 1) ||
        !exercise(Twrite, 1, 1, 0) || !exercise(Twrite, 0, 1, 1))
    {
        fprintf(stderr, "channel stop wire: held RPC, flush or neighboring RPC failed\n");
        return 1;
    }
    return 0;
}
