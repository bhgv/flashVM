/* Copyright (c) 2026 bhgv
 * EN: Check exact flush, output ownership and assignment outcomes for shared cells.
 * RU: Проверить точный flush, владение выходами и исходы присваивания общих ячеек.
 */
#include <lib9.h>
#include <styx.h>
#include "client.h"
#include "styxrpc/named_cell.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define FRAME_LIMIT 512U /* Negotiated 9P frame size in bytes. */

typedef enum Stage
{
    Create,     /* Withhold the token body after creation. */
    Attach,     /* Withhold the acknowledgement of a complete token. */
    Read,       /* Withhold the frame body after its length. */
    SetHeader,  /* Withhold the acknowledgement before any value write. */
    SetBody,    /* Withhold the acknowledgement of the submitted value. */
    SetTail,    /* Confirm a short prefix then withhold acknowledgement of its suffix. */
    SetAck      /* Deliver the initial acknowledgement before Rflush. */
} Stage;

static const unsigned char token[STYXRPC_CELL_TOKEN] = {0x42}; /* Opaque on this issuing attachment. */
static const unsigned char value[] = {STYXRPC_NAMED_BOOLEAN, 1}; /* Complete value frame. */

typedef struct Wire
{
    int fd;             /* Provider endpoint owned by serve. */
    Stage stage;        /* Case selected by the calling test. */
    u32int fid;         /* Temporary endpoint on .channels. */
    u32int neighbour;   /* Independent fid serving normal reads. */
    unsigned operation; /* Last complete request header sent over fid. */
    ushort tag;         /* Exact original tag selected for flush. */
    unsigned held_size; /* Submitted write count used for the acknowledgement race. */
    unsigned flushes;   /* Exact flush requests received by serve. */
    unsigned writes;    /* Value bodies accepted by serve. */
    unsigned controls;  /* Completed neighbour reads. */
    int prefix;         /* The provider confirmed the first byte in SetTail. */
    int clean;          /* Peer observed EOF at a frame boundary. */
    int held;           /* Atomic original request publication. */
    int stopped;        /* Atomic stop request from the supervisor. */
    int retired;        /* Atomic acknowledgement of the temporary fid's clunk. */
} Wire;

typedef struct Call
{
    Wire *wire;           /* Borrowed fixture through join. */
    StyxClient *client;   /* Borrowed session through join. */
    StyxFid *sentinel;    /* Borrowed output sentinel, distinct from the temporary fid. */
    StyxRpcIoOutcome result; /* Status reported by the public cell API. */
    int outputs;          /* Failure output storage kept its original value. */
} Call;


/* EN: Encode one borrowed reply and send it through wire's provider socket.
 * RU: Закодировать заимствованный ответ и послать через сокет провайдера wire. */
static int send_reply(Wire *wire, Fcall *reply)
{
    unsigned char bytes[FRAME_LIMIT];
    unsigned size = convS2M(reply, bytes, sizeof(bytes));
    return size && send(wire->fd, bytes, size, MSG_NOSIGNAL) == (ssize_t)size;
}


/* EN: Publish the exact original request without replying to it.
 * RU: Опубликовать исходный запрос, удержав ответ на него. */
static int hold(Wire *wire, const Fcall *request)
{
    if (__atomic_load_n(&wire->held, __ATOMIC_RELAXED))
    {
        return 0;
    }
    wire->tag = request->tag;
    wire->held_size = request->count;
    __atomic_store_n(&wire->held, 1, __ATOMIC_RELEASE);
    return 1;
}


/* EN: Accept the header and exact known body for this case. Track application
 * of a complete replacement even when its acknowledgement is withheld.
 * RU: Принять заголовок и точное тело этого случая. Учесть замену значения,
 * даже если подтверждение удерживается. */
static int write_cell(Wire *wire, const Fcall *request, Fcall *reply)
{
    if (request->fid != wire->fid)
    {
        return 0;
    }
    if (!request->offset && request->count == STYXRPC_NAMED_HEADER)
    {
        const unsigned char *header = (const unsigned char *)request->data;
        wire->operation = header[0];
        size_t size = wire->operation == STYXRPC_CELL_ATTACH ? sizeof(token) :
            wire->operation == STYXRPC_CELL_GET ? 0 : sizeof(value);
        if (header[1] || header[2] || header[3] || header[4] != size)
        {
            return 0;
        }
        if (wire->stage == SetHeader && wire->operation == STYXRPC_CELL_SET)
        {
            return hold(wire, request);
        }
    }
    else if (request->offset == STYXRPC_NAMED_HEADER && wire->operation == STYXRPC_CELL_ATTACH &&
        request->count == sizeof(token) && !memcmp(request->data, token, sizeof(token)))
    {
        if (wire->stage == Attach)
        {
            return hold(wire, request);
        }
    }
    else if (wire->stage == SetTail && wire->operation == STYXRPC_CELL_SET &&
        wire->prefix && request->offset == STYXRPC_NAMED_HEADER + 1U &&
        request->count == sizeof(value) - 1U && !memcmp(request->data, value + 1, sizeof(value) - 1U))
    {
        ++wire->writes;
        return hold(wire, request);
    }
    else if (request->offset == STYXRPC_NAMED_HEADER && request->count == sizeof(value) &&
        !memcmp(request->data, value, sizeof(value)))
    {
        if (wire->operation == STYXRPC_CELL_SET)
        {
            if (wire->stage == SetTail)
            {
                if (wire->prefix)
                {
                    return 0;
                }
                wire->prefix = 1;
                reply->count = 1;
                return send_reply(wire, reply);
            }
            ++wire->writes;
            return wire->stage != SetHeader && hold(wire, request);
        }
        if (wire->operation != STYXRPC_CELL_NEW || wire->stage != Create)
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }
    reply->count = request->count;
    return send_reply(wire, reply);
}


/* EN: Serve the independent fid or the cell reply length. Hold the complete
 * token or value body at the original offset for the controller.
 * RU: Обслужить независимый fid или длину ответа. Удержать тело жетона или
 * значения на его исходном смещении для управляющего потока. */
static int read_cell(Wire *wire, const Fcall *request, Fcall *reply)
{
    if (request->fid == wire->neighbour)
    {
        ++wire->controls;
        reply->count = 1;
        reply->data = "v";
        return send_reply(wire, reply);
    }
    if (request->fid != wire->fid ||
        (wire->operation != STYXRPC_CELL_NEW && wire->operation != STYXRPC_CELL_GET))
    {
        return 0;
    }
    size_t size = wire->operation == STYXRPC_CELL_NEW ? sizeof(token) : sizeof(value);
    if (!request->offset && request->count == STYXRPC_NAMED_REPLY)
    {
        char length[STYXRPC_NAMED_REPLY] = {0, 0, 0, (char)size};
        reply->count = sizeof(length);
        reply->data = length;
        return send_reply(wire, reply);
    }
    return request->offset == STYXRPC_NAMED_REPLY && request->count == size && hold(wire, request);
}


/* EN: Handle one original request. Verify the flush addresses tag and publish
 * endpoint retirement only after the provider acknowledged its clunk.
 * RU: Обработать запрос, проверить тег flush и отметить закрытие временного
 * fid после подтверждения clunk провайдером. */
static int answer(Wire *wire, const Fcall *request)
{
    Fcall reply = {.type = request->type + 1, .tag = request->tag};
    switch (request->type)
    {
    case Tversion:
        reply.msize = FRAME_LIMIT;
        reply.version = VERSION9P;
        break;
    case Tattach:
        reply.qid = (Qid){.type = QTDIR, .path = 0};
        break;
    case Twalk:
        if (request->nwname != 1)
        {
            return 0;
        }
        if (!strcmp(request->wname[0], STYXRPC_NAMED_SERVICE))
        {
            wire->fid = request->newfid;
        }
        else if (!strcmp(request->wname[0], "neighbour"))
        {
            wire->neighbour = request->newfid;
        }
        else
        {
            return 0;
        }
        reply.nwqid = 1;
        reply.wqid[0] = (Qid){.type = QTFILE, .path = request->newfid};
        break;
    case Topen:
        reply.qid = (Qid){.type = QTFILE, .path = request->fid};
        break;
    case Twrite:
        return write_cell(wire, request, &reply);
    case Tread:
        return read_cell(wire, request, &reply);
    case Tflush:
        if (!__atomic_load_n(&wire->held, __ATOMIC_RELAXED) ||
            request->oldtag != wire->tag || wire->flushes++ || wire->controls != 1)
        {
            return 0;
        }
        if (wire->stage == SetAck)
        {
            Fcall original = {.type = Rwrite, .tag = wire->tag, .count = wire->held_size};
            if (!send_reply(wire, &original))
            {
                return 0;
            }
        }
        break;
    case Tclunk:
        if (wire->fid && request->fid == wire->fid)
        {
            if (wire->flushes != 1 || !send_reply(wire, &reply))
            {
                return 0;
            }
            __atomic_store_n(&wire->retired, 1, __ATOMIC_RELEASE);
            return 1;
        }
        break;
    default:
        return 0;
    }
    return send_reply(wire, &reply);
}


/* EN: Own the provider socket and dispatch complete frames until client EOF.
 * RU: Владеть сокетом провайдера и обрабатывать полные кадры до EOF клиента. */
static void *serve(void *state)
{
    Wire *wire = state;
    unsigned char bytes[FRAME_LIMIT];
    ssize_t count;
    while ((count = recv(wire->fd, bytes, 4, MSG_WAITALL)) == 4)
    {
        Fcall request;
        unsigned size = GBIT32(bytes);
        if (size < 7 || size > sizeof(bytes) ||
            recv(wire->fd, bytes + 4, size - 4, MSG_WAITALL) != (ssize_t)(size - 4) ||
            convM2S(bytes, size, &request) != size || !answer(wire, &request))
        {
            break;
        }
    }
    wire->clean = count == 0;
    close(wire->fd);
    return NULL;
}


/* EN: Read the supervisor's atomic stop flag from the caller thread.
 * RU: Прочитать флаг остановки контроллера из потока вызова. */
static int stopped(void *state)
{
    const Wire *wire = state;
    return __atomic_load_n(&wire->stopped, __ATOMIC_ACQUIRE);
}


/* EN: Execute the public API on the caller and verify untouched failure outputs.
 * RU: Вызвать открытый API и проверить сохранение выходов при отказе. */
static void *call(void *state)
{
    Call *test = state;
    Wire *wire = test->wire;
    StyxFid *file = test->sentinel;
    unsigned char created[STYXRPC_CELL_TOKEN] = {0};
    unsigned char sentinel = 9, *frame = &sentinel;
    size_t size = 17;
    switch (wire->stage)
    {
    case Create:
        test->result = styxrpc_cell_create_until(test->client, value, sizeof(value),
            created, &file, stopped, wire);
        test->outputs = file == test->sentinel && !memcmp(created, (unsigned char[STYXRPC_CELL_TOKEN]){0}, sizeof(created));
        break;
    case Attach:
        test->result = styxrpc_cell_attach_until(test->client, token, &file, stopped, wire);
        test->outputs = file == test->sentinel;
        break;
    case Read:
        test->result = styxrpc_cell_read_until(test->client, token, &frame, &size, stopped, wire);
        test->outputs = frame == &sentinel && size == 17;
        break;
    case SetHeader:
    case SetBody:
    case SetTail:
    case SetAck:
        test->result = styxrpc_cell_write_until(test->client, token, value, sizeof(value), stopped, wire);
        test->outputs = 1;
        break;
    }
    if (file != test->sentinel)
    {
        styxclientclunk(file);
    }
    if (frame != &sentinel)
    {
        free(frame);
    }
    return NULL;
}


/* EN: Read one confirmed byte from the borrowed independent fid.
 * RU: Прочитать подтверждённый байт из заимствованного независимого fid. */
static int read_control(StyxFid *file)
{
    StyxTransfer read;
    char byte;
    styxclientread(file, 0, &byte, 1, &read);
    return read.success && read.count == 1 && byte == 'v';
}


/* EN: Wait up to one second for a provider observation published under acquire/release.
 * RU: Ждать до секунды наблюдения провайдера с синхронизацией acquire/release. */
static int observed(const int *flag)
{
    const struct timespec pause = {0, 1000000};
    for (unsigned i = 0; i < 1000 && !__atomic_load_n(flag, __ATOMIC_ACQUIRE); ++i)
    {
        nanosleep(&pause, NULL);
    }
    return __atomic_load_n(flag, __ATOMIC_ACQUIRE);
}


/* EN: Exercise one scenario on fresh sockets, check two live reads and exact teardown.
 * RU: Выполнить сценарий на новых сокетах, проверить два чтения и точное освобождение. */
static int exercise(Stage stage, StyxRpcIoOutcome expected)
{
    int fd[2];
    pthread_t provider, caller;
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd))
    {
        return 0;
    }
    Wire wire = {.fd = fd[1], .stage = stage};
    if (pthread_create(&provider, NULL, serve, &wire))
    {
        close(fd[0]);
        close(fd[1]);
        return 0;
    }
    StyxClient *client = styxclientmountfd(fd[0], FRAME_LIMIT, "test", "", 2500, 8);
    StyxFid *neighbour = client ? styxclientwalk(client, "neighbour") : NULL;
    Call test = {.wire = &wire, .client = client, .sentinel = neighbour};
    int ok = neighbour && styxclientopen(neighbour, 0) && !pthread_create(&caller, NULL, call, &test);
    if (ok)
    {
        ok = observed(&wire.held) && read_control(neighbour);
        __atomic_store_n(&wire.stopped, 1, __ATOMIC_RELEASE);
        pthread_join(caller, NULL);
        ok = ok && expected == test.result && test.outputs && observed(&wire.retired) && read_control(neighbour);
    }
    styxclientclunk(neighbour);
    styxclientfree(client);
    pthread_join(provider, NULL);
    ok = ok && wire.writes == (stage == SetBody || stage == SetTail || stage == SetAck) && wire.clean &&
        wire.flushes == 1 && wire.controls == 2;
    if (!ok)
    {
        fprintf(stderr, "cell phase %d result %d/%d outputs %d flushes %u writes %u controls %u\n",
            stage, test.result, expected, test.outputs, wire.flushes, wire.writes, wire.controls);
    }
    return ok;
}


/* EN: Cover creation, attachment, reading, staged and submitted mutation, and reply precedence.
 * RU: Проверить создание, присоединение, чтение, незавершённую и отправленную запись и приоритет ответа. */
int main(void)
{
    return exercise(Create, STYXRPC_IO_CANCELLED) && exercise(Attach, STYXRPC_IO_CANCELLED) &&
        exercise(Read, STYXRPC_IO_CANCELLED) && exercise(SetHeader, STYXRPC_IO_CANCELLED) &&
        exercise(SetBody, STYXRPC_IO_UNCERTAIN) && exercise(SetTail, STYXRPC_IO_UNCERTAIN) &&
        exercise(SetAck, STYXRPC_IO_OK) ? 0 : 1;
}
