/* Copyright (c) 2026 bhgv
 * EN: Bind separate native send and receive queues to a PID or module result.
 * RU: Привязка отдельных очередей отправки и приёма к PID или результату модуля.
 */
#define _POSIX_C_SOURCE 200809L
#include "styxrpc/channel_mailbox.h"
#include "channel_scheduler_internal.h"
#include "load_profile.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define FRAME STYXRPC_MANAGED_LIMIT /* Maximum slot and fid buffer length. */
#define MAILBOX_KEY 1U /* Single wire key on this reverse PID. */
#define MAX_REPLY_OVERHEAD (2U * FVM_SRECORD_INTEGER_MAX + 5U) /* Length, tag, checksum and ordinal. */


/* EN: FIFO and scheduler for one direction. All access takes the mailbox lock.
 * RU: FIFO и планировщик одного направления. Доступ под мьютексом очереди. */
typedef struct MailboxLane
{
    StyxRpcChannelQueue queue;          /* Cursor and borrowed slot arrays. */
    StyxRpcChannelScheduler scheduler;  /* Pending single-guard operations. */
    size_t sizes[STYXRPC_CHANNEL_GROUP_CAPACITY]; /* Frame lengths by slot. */
} MailboxLane;


struct StyxRpcChannelMailbox
{
    pthread_mutex_t lock;       /* Serializes dispatcher and owning host. */
    MailboxLane incoming;       /* Peer sends; host receives. */
    MailboxLane outgoing;       /* Host sends; peer receives. */
    StyxRpcChannelGrant grant;  /* Immutable key and element type. */
    StyxRpcChannelPolicy policy; /* Immutable matching route profile. */
    int result;                  /* Module-result wire offsets and PID leaves if set. */
    unsigned references;        /* Resource and optional independent result owner. */
    int active;                  /* Resource remains pending publication or live. */
    unsigned char arena[];      /* Disjoint frame slots for the two directions. */
};


/* EN: Own one operation on a temporary dispatcher fid.
 * RU: Владеть одной операцией на временном fid диспетчера. */
typedef struct MailboxOpen
{
    StyxRpcChannelMailbox *mailbox; /* Borrowed until all fids retire. */
    MailboxLane *lane;              /* Set on admission of the sole guard. */
    StyxRpcChannelFid fid;         /* Validated native request and reply. */
    unsigned char storage[];       /* Separate input and output buffers. */
} MailboxOpen;


/* EN: Return monotonic milliseconds for WAIT expiry, zero on clock failure.
 * RU: Вернуть монотонные миллисекунды для WAIT, ноль при ошибке часов. */
static uint64_t clock_ms(void *unused)
{
    struct timespec now;
    (void)unused;
    return !clock_gettime(CLOCK_MONOTONIC, &now) ?
        UINT64_C(1000) * (uint64_t)now.tv_sec + (uint64_t)now.tv_nsec / 1000000 : 0;
}


/* EN: Require the mailbox's exact address kind for every non-nil leaf.
 * RU: Требовать точный вид адреса очереди для каждой ссылки кроме nil. */
static int allow_route(const void *context, const StyxRpcChannelRoute *route)
{
    const StyxRpcChannelMailbox *mailbox = context;
    return route->kind == (mailbox->result ? STYXRPC_ROUTE_PID : STYXRPC_ROUTE_MODULE);
}


/* EN: Admit one peer guard to the corresponding directional scheduler under lock.
 * RU: Передать одну ветку партнёра планировщику нужного направления под мьютексом. */
static int admit(void *context, StyxRpcChannelFid *fid)
{
    MailboxOpen *opened = context;
    if (fid->selection.count != 1)
    {
        return 0;
    }
    opened->lane = fid->selection.guards[0].receive ? &opened->mailbox->outgoing : &opened->mailbox->incoming;
    return styxrpc_scheduler_admit(&opened->lane->scheduler, fid);
}


/* EN: Remove the retiring peer operation from its scheduler under lock.
 * RU: Снять закрываемую операцию партнёра из её планировщика под мьютексом. */
static void cancel(void *context, StyxRpcChannelFid *fid)
{
    MailboxOpen *opened = context;
    styxrpc_scheduler_cancel(&opened->lane->scheduler, fid);
}


/* EN: Allocate a private duplex fid which borrows the mailbox in context.
 * RU: Выделить отдельный двусторонний fid, заимствующий очередь context. */
static int open_bus(void *context, unsigned access, void **cookie)
{
    StyxRpcChannelMailbox *mailbox = context;
    if (access != (STYXRPC_READ | STYXRPC_WRITE))
    {
        return 0;
    }
    MailboxOpen *opened = calloc(1, sizeof(*opened) + 2U * FRAME);
    if (!opened)
    {
        return 0;
    }
    opened->mailbox = mailbox;
    opened->fid = (StyxRpcChannelFid){.grants = &mailbox->grant, .grant_count = 1,
        .request = opened->storage, .request_capacity = FRAME, .reply = opened->storage + FRAME,
        .reply_capacity = FRAME, .admit = admit, .cancel = cancel, .context = opened, .policy = &mailbox->policy};
    *cookie = opened;
    return 1;
}


/* EN: Confirm one validated request fragment through the exact open cookie.
 * RU: Подтвердить проверенный фрагмент запроса через точный cookie. */
static int write_bus(void *cookie, uint64_t offset, const void *bytes, size_t count, size_t *size)
{
    MailboxOpen *opened = cookie;
    pthread_mutex_lock(&opened->mailbox->lock);
    int accepted = offset <= SIZE_MAX &&
        styxrpc_channel_fid_write(&opened->fid, (size_t)offset, bytes, count) != STYXRPC_INVALID;
    pthread_mutex_unlock(&opened->mailbox->lock);
    if (accepted)
    {
        *size = count;
    }
    return accepted;
}


/* EN: Poll and read the confirmed reply at the native post-request offset.
 * RU: Проверить готовность и прочитать ответ со штатного смещения после запроса. */
static int read_bus(void *cookie, uint64_t offset, void *bytes, size_t capacity, size_t *size)
{
    MailboxOpen *opened = cookie;
    pthread_mutex_lock(&opened->mailbox->lock);
    if (opened->lane)
    {
        styxrpc_scheduler_progress(&opened->lane->scheduler);
    }
    size_t origin = opened->mailbox->result ? 0 : opened->fid.staged;
    int accepted = offset >= origin && offset - origin <= SIZE_MAX &&
        styxrpc_channel_fid_read(&opened->fid, (size_t)(offset - origin), bytes, capacity, size) != STYXRPC_INVALID;
    pthread_mutex_unlock(&opened->mailbox->lock);
    return accepted;
}


/* EN: Cancel the exact operation before freeing its per-fid buffers.
 * RU: Отменить точную операцию до освобождения буферов fid. */
static void close_bus(void *cookie)
{
    MailboxOpen *opened = cookie;
    pthread_mutex_lock(&opened->mailbox->lock);
    styxrpc_channel_fid_close(&opened->fid);
    pthread_mutex_unlock(&opened->mailbox->lock);
    free(opened);
}


/* EN: Drop one reference, marking the resource retired when requested.
 * RU: Снять одну ссылку и при необходимости пометить ресурс отозванным. */
static void drop(StyxRpcChannelMailbox *mailbox, int retired)
{
    pthread_mutex_lock(&mailbox->lock);
    if (retired)
    {
        mailbox->active = 0;
    }
    unsigned references = --mailbox->references;
    pthread_mutex_unlock(&mailbox->lock);
    if (!references)
    {
        pthread_mutex_destroy(&mailbox->lock);
        free(mailbox);
    }
}


/* EN: Retire the exported resource after all operation fids have closed.
 * RU: Отозвать выданный ресурс после закрытия всех fid операций. */
static void release(void *context)
{
    drop(context, 1);
}


/* EN: Assign slots and initialize the native scheduler for one lane.
 * RU: Назначить слоты и подготовить нативный планировщик одного направления. */
static int init_lane(MailboxLane *lane, const StyxRpcChannelGrant *grant, unsigned slots, unsigned char *bytes)
{
    lane->queue = (StyxRpcChannelQueue){.grant = *grant, .bytes = bytes, .sizes = lane->sizes,
        .capacity = slots, .stride = FRAME};
    return styxrpc_scheduler_init(&lane->scheduler, &lane->queue, 1, clock_ms, NULL);
}


/* EN: Create a mailbox for a reverse PID or a module result with both output owners.
 * RU: Создать очередь встречного PID или результата модуля с двумя владельцами. */
static StyxRpcStatus create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output, int result)
{
    const char *end = shape && *shape == '(' && strlen(shape) < STYXRPC_MANAGED_SHAPE ?
        styxshape_end(shape + 1) : NULL;
    if (!end || *end != ')' || end[1] || strpbrk(shape, "rh") ||
        (!result && (!slots || !strchr(shape, 'c'))) || slots > STYXRPC_CHANNEL_GROUP_CAPACITY || !mailbox || !output)
    {
        return STYXRPC_INVALID;
    }
    size_t extent = (size_t)slots * FRAME;
    StyxRpcChannelMailbox *state = calloc(1, sizeof(*state) + 2 * extent);
    if (!state)
    {
        return STYXRPC_INVALID;
    }
    state->grant.key = MAILBOX_KEY;
    strcpy(state->grant.shape, shape);
    state->result = result;
    state->policy = (StyxRpcChannelPolicy){allow_route, state};
    state->references = result ? 2 : 1;
    state->active = 1;
    if (!init_lane(&state->incoming, &state->grant, slots, state->arena) ||
        !init_lane(&state->outgoing, &state->grant, slots, state->arena + extent) ||
        pthread_mutex_init(&state->lock, NULL))
    {
        free(state);
        return STYXRPC_INVALID;
    }
    *mailbox = state;
    *output = (StyxRpcChannelOutput){.resource = {state, open_bus, read_bus, write_bus, close_bus},
        .key = MAILBOX_KEY, .unbuffered = !slots, .release = release};
    return STYXRPC_OK;
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
StyxRpcStatus styxrpc_channel_mailbox_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output)
{
    return create(shape, slots, mailbox, output, 0);
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
StyxRpcStatus styxrpc_channel_mailbox_result_create(const char *shape, unsigned slots,
    StyxRpcChannelMailbox **mailbox, StyxRpcChannelOutput *output)
{
    return create(shape, slots, mailbox, output, 1);
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
int styxrpc_channel_mailbox_active(StyxRpcChannelMailbox *mailbox)
{
    if (!mailbox)
    {
        return 0;
    }
    pthread_mutex_lock(&mailbox->lock);
    int active = mailbox->active;
    pthread_mutex_unlock(&mailbox->lock);
    return active;
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
void styxrpc_channel_mailbox_owner_release(StyxRpcChannelMailbox *mailbox)
{
    if (mailbox && mailbox->result)
    {
        drop(mailbox, 0);
    }
}


/* EN: Check that a near-limit frame fits its real native receive reply.
 * RU: Проверить, что почти предельный кадр помещается в штатный ответ приёма. */
static int fits_reply(const StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size)
{
    if (size <= FRAME - MAX_REPLY_OVERHEAD)
    {
        return 1;
    }
    unsigned char *reply = malloc(FRAME);
    const StyxRpcSelection receive = {.count = 1, .guards = {{MAILBOX_KEY, 1, NULL, 0}}, .policy = &mailbox->policy};
    size_t used;
    int fits = reply && styxrpc_channel_reply(&mailbox->grant, 1, &receive, 1, bytes, size, reply, FRAME, &used) == STYXRPC_OK;
    free(reply);
    return fits;
}


/* EN: Validate a copied or routed owner frame using the mailbox's exact type.
 * RU: Проверить кадр владельца по точному типу почтовой очереди. */
static int valid_frame(const StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size)
{
    const char *shape = mailbox->grant.shape;
    if (!strcmp(shape, "(i)"))
    {
        int64_t value;
        return styxrpc_value_get_integer(STYXRPC_VALUE_INT, bytes, size, &value) == STYXRPC_OK;
    }
    StyxRpcChannelRoutes routes;
    if (styxrpc_channel_element_routes(shape, bytes, size, &routes) != STYXRPC_OK)
    {
        return 0;
    }
    for (size_t i = 0; i < routes.count; ++i)
    {
        if (!allow_route(mailbox, routes.entries + i))
        {
            return 0;
        }
    }
    return fits_reply(mailbox, bytes, size);
}


/* EN: Execute an immediate owner rendezvous with a previously checked guard.
 * RU: Немедленно выполнить rendezvous владельца с уже проверенной веткой. */
static StyxRpcStatus try_owner(StyxRpcChannelMailbox *mailbox, MailboxLane *lane, const StyxRpcGuard *guard,
    unsigned char *reply, size_t capacity, StyxRpcChannelResult *result)
{
    StyxRpcChannelFid fid = {.grants = &mailbox->grant, .grant_count = 1,
        .reply = reply, .reply_capacity = capacity, .pending = 1};
    fid.selection = (StyxRpcSelection){.mode = STYXRPC_CHANNEL_TRY, .count = 1, .policy = &mailbox->policy};
    fid.selection.guards[0] = *guard;
    styxrpc_scheduler_try_owner(&lane->scheduler, &fid);
    StyxRpcStatus status = styxrpc_channel_result(fid.grants, fid.grant_count, &fid.selection, reply, fid.ready, result);
    return status == STYXRPC_OK && !result->selected ? STYXRPC_MORE : status;
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
StyxRpcStatus styxrpc_channel_mailbox_send(StyxRpcChannelMailbox *mailbox, const void *bytes, size_t size)
{
    if (!mailbox || !valid_frame(mailbox, bytes, size))
    {
        return STYXRPC_INVALID;
    }
    MailboxLane *lane = &mailbox->outgoing;
    StyxRpcChannelQueue *queue = &lane->queue;
    pthread_mutex_lock(&mailbox->lock);
    StyxRpcStatus status = mailbox->active ? STYXRPC_MORE : STYXRPC_INVALID;
    if (mailbox->active && !queue->capacity)
    {
        unsigned char reply[STYXRPC_FRAME_SIZE];
        const StyxRpcGuard guard = {MAILBOX_KEY, 0, bytes, size};
        StyxRpcChannelResult result;
        status = try_owner(mailbox, lane, &guard, reply, sizeof(reply), &result);
    }
    else if (mailbox->active && queue->count < queue->capacity)
    {
        size_t tail = (queue->head + queue->count) % queue->capacity;
        memcpy(queue->bytes + tail * queue->stride, bytes, size);
        queue->sizes[tail] = size;
        ++queue->count;
        styxrpc_scheduler_progress(&lane->scheduler);
        status = STYXRPC_OK;
    }
    pthread_mutex_unlock(&mailbox->lock);
    return status;
}


/* EN: Receive one rendezvous value only if the caller can hold any valid frame.
 * RU: Принять rendezvous лишь если буфер вызывающего вмещает любой допустимый кадр. */
static StyxRpcStatus receive_direct(StyxRpcChannelMailbox *mailbox, void *bytes, size_t capacity, size_t *size)
{
    if (capacity < FRAME)
    {
        return STYXRPC_INVALID;
    }
    unsigned char *reply = malloc(FRAME);
    if (!reply)
    {
        return STYXRPC_INVALID;
    }
    const StyxRpcGuard guard = {MAILBOX_KEY, 1, NULL, 0};
    StyxRpcChannelResult result;
    StyxRpcStatus status = try_owner(mailbox, &mailbox->incoming, &guard, reply, FRAME, &result);
    if (status == STYXRPC_OK)
    {
        if (!strcmp(mailbox->grant.shape, "(i)"))
        {
            status = styxrpc_value_integer(STYXRPC_VALUE_INT, result.integer, bytes, capacity, size);
        }
        else
        {
            memcpy(bytes, result.value, result.size);
            *size = result.size;
        }
    }
    free(reply);
    return status;
}


/* EN/RU: See channel_mailbox.h. / См. channel_mailbox.h. */
StyxRpcStatus styxrpc_channel_mailbox_receive(StyxRpcChannelMailbox *mailbox, void *bytes, size_t capacity, size_t *size)
{
    if (!mailbox || !bytes || !size)
    {
        return STYXRPC_INVALID;
    }
    MailboxLane *lane = &mailbox->incoming;
    StyxRpcChannelQueue *queue = &lane->queue;
    pthread_mutex_lock(&mailbox->lock);
    StyxRpcStatus status = mailbox->active ? STYXRPC_MORE : STYXRPC_INVALID;
    if (mailbox->active && !queue->capacity)
    {
        status = receive_direct(mailbox, bytes, capacity, size);
    }
    else if (mailbox->active && queue->count)
    {
        size_t length = queue->sizes[queue->head];
        status = STYXRPC_INVALID;
        if (length <= capacity)
        {
            memcpy(bytes, queue->bytes + queue->head * queue->stride, length);
            *size = length;
            queue->head = (queue->head + 1) % queue->capacity;
            --queue->count;
            styxrpc_scheduler_progress(&lane->scheduler);
            status = STYXRPC_OK;
        }
    }
    pthread_mutex_unlock(&mailbox->lock);
    return status;
}
