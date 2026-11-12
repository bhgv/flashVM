# Copyright (c) 2026 bhgv
# EN: Channel ring reservations and transfer transactions.
# RU: Резервации кольца канала и транзакции передачи.


# EN: Protect ring geometry and reservation while keeping movers unlocked.
# source/body: borrowed immutable C text; return adapted text, raise on drift.
# No runtime ownership or locks; generation runs before compilation.
# RU: Защитить геометрию кольца резервацией передачи.
# source/body: неизменяемый текст C; вернуть адаптацию, при расхождении — исключение.
# Ресурсами VM не владеет; генерация выполняется до компиляции.
def adapt_buffer_reservation(source):
    """Protect ring geometry and reservation while keeping movers unlocked."""
    start = source.index("static int\ncgetb(Channel *c, void *v)\n{")
    end = source.index("/* Reject payload aliases", start)
    body = source[start:end]
    for required in (
        "dvm_runtime_heap_retain(&h->ref);",
        "c->front = buffer.front;",
        "c->size = buffer.size;",
        "kprotect(channelread, &read);",
        "DVM_R.d = buffer.slot;",
        "c->mover();",
    ):
        if required not in body:
            raise ValueError("channel buffer reservation drift: " + required)
    return source[:start] + BUFFER_RESERVATION + source[end:]


BUFFER_RESERVATION = r"""
extern void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument);

extern void dvm_runtime_channel_buffer_notify(void);
#include "disvm/native_context.h"

enum { CHANNEL_ALT_TRY = -1 };

typedef struct ChannelReservation
{
    ChannelRead read; /* Receive retains the array through its mover. */
    int receive;
    int available;
    int publish_after_move;
    int next_size;
    int next_front;
    Type *transaction_type; /* Validated layout for a guarded built-in transfer. */
    int committed;
    int completed;
    struct ChannelReservation *next;
    DVM_NativeContext *owner;
    int replay_opcode; /* Only direct buffered SEND or RECV may replay. */
    int deferred; /* BUSY before value or ring metadata changed. */
    Prog *refill_sender; /* Pinned and claimed for the full refill callback. */
} ChannelReservation;


/* Stack ledgers are linked only while their operations own a reservation.
 * The queue mutex protects membership. No mover runs with the mutex held. */
#include "channel_match.h"

static ChannelReservation *active_channel_reservations;

int dvm_runtime_channel_buffer_busy(Channel *channel)
{
    ChannelReservation *entry;
    if (dvm_channel_match_blocked(channel, dvm_native_context_current()))
    {
        return 1;
    }
    for (entry = active_channel_reservations; entry; entry = entry->next)
    {
        if (entry->read.channel == channel)
            return 1;
    }
    return 0;
}


/* With the queue mutex or NativeWorld held, classify a reservation.
 * A mover must never park behind another mover. */
int dvm_runtime_channel_wait_state(Channel *channel)
{
    ChannelReservation *entry;
    DVM_NativeContext *current = dvm_native_context_current();
    int busy = 0;
    int owns_reservation = 0;
    for (entry = active_channel_reservations; entry; entry = entry->next)
    {
        busy |= entry->read.channel == channel;
        owns_reservation |= entry->owner == current;
    }
    if (busy)
    {
        return owns_reservation ? -1 : 1;
    }
    return dvm_channel_match_blocked(channel, current);
}


/* An unoccupied mapped slot must own no pointers. That makes the guarded
 * send transaction incapable of raising from displaced-owner retirement. */
static int channel_slot_is_empty(ChannelBuffer *buffer)
{
    int byte, bit;
    for (byte = 0; byte < buffer->type->np; byte++)
    {
        for (bit = 0; bit < 8; bit++)
        {
            void *value;
            if (!(buffer->type->map[byte] & (0x80U >> bit)))
                continue;
            memmove(&value, (uchar *)buffer->slot + (byte*8+bit)*sizeof(void *),
                sizeof(value));
            if (value != H)
                return 0;
        }
    }
    return 1;
}


/* Capture the next state. Pointer-free transfers publish it after the copy.
 * NativeWorld still excludes another reservation. */
static int reserve_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    ChannelRead *read = &reservation->read;
    Channel *channel = read->channel;
    ChannelBuffer buffer;
    int status;
    int busy = dvm_runtime_channel_wait_state(channel);
    if (busy < 0)
    {
        error("channel buffer transfer already active");
    }
    if (busy)
    {
        reservation->available = reservation->replay_opcode == CHANNEL_ALT_TRY ? 0 : -1;
        return 1;
    }
    if (reservation->replay_opcode == CHANNEL_ALT_TRY &&
        channelpeer(channel, reservation->receive))
        return 1;
    if (dvm_runtime_guest_unbounded && reservation->replay_opcode > 0 &&
        channelpeer(channel, reservation->receive))
    {
        if (!dvm_runtime_scalar_retry(reservation->replay_opcode))
        {
            error("channel peer changed repeatedly before reserving buffer");
        }
        reservation->available = 2;
        return 1;
    }
    status = channelbuffer(channel, reservation->receive, 0, &buffer);
    if (status < 0)
        error(buffer.error);
    if (!status)
        return 1;
    if (buffer.width && !channelspan(read->destination, buffer.width))
        error(read->destination == nil || read->destination == H ? exNilref : exBounds);
    /* [GC:] EN: Pointer rings need the same guarded consume as mapped records:
     * clearing a source owner is a write observed by concurrent graph capture.
     * RU: Кольцо указателей требует того же защищённого consume, что mapped:
     * очистка исходного владельца — запись, видимая конкурентному обходу графа. */
    if (dvm_runtime_guarded_copies &&
        ((buffer.type->np && channel->mover == movtmp) ||
         (buffer.type == &Tptr && buffer.width == sizeof(void *) && channel->mover == movp)))
    {
        if ((channel->mover == movtmp && channel->mid.t != buffer.type) ||
            (!reservation->receive && !channel_slot_is_empty(&buffer)))
            error(exHeap);
        reservation->transaction_type = buffer.type;
    }
    else if (reservation->replay_opcode && dvm_runtime_guarded_copies && !buffer.type->np &&
        ((channel->mover == movb && buffer.width == sizeof(BYTE)) ||
         (channel->mover == movw && buffer.width == sizeof(WORD)) ||
         (channel->mover == movl && buffer.width == sizeof(LONG)) ||
         (channel->mover == movf && buffer.width == sizeof(REAL)) ||
         (channel->mover == movm && channel->mid.w == buffer.width)))
    {
        reservation->transaction_type = buffer.type;
    }
    if (reservation->receive)
    {
        Heap *header = D2H(buffer.array);
        ulong refs = dvm_refcount_load_u(&header->ref);
        if (!refs || refs == (ulong)-1)
            error(exHeap);
        dvm_runtime_heap_retain(&header->ref);
        read->array = buffer.array;
    }
    read->slot = buffer.slot;
    reservation->next_size = buffer.size;
    reservation->next_front = buffer.front;
    reservation->publish_after_move = !buffer.type->np || reservation->transaction_type != nil;
    if (!reservation->publish_after_move)
    {
        if (reservation->receive)
            channel->front = buffer.front;
        channel->size = buffer.size;
    }
    reservation->owner = dvm_native_context_current();
    reservation->next = active_channel_reservations;
    active_channel_reservations = reservation;
    reservation->available = 1;
    return 1;
}


/* EN: Wait outside the queue mutex for a foreign buffer reservation.
 * RU: Дождаться чужой резервации буфера вне мьютекса очереди. */
static int claim_channel_buffer(ChannelReservation *reservation)
{
    do
    {
        reservation->available = 0;
        dvm_runtime_channel_queue_guard(reserve_channel_buffer, reservation);
        if (reservation->available >= 0)
        {
            return reservation->available;
        }
        dvm_runtime_channel_require_wait(dvm_runtime_channel_wait_guest(reservation->read.channel));
    } while (1);
}


/* Publish a committed transfer and remove its reservation on every exit. */
static int finish_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    Channel *channel = reservation->read.channel;
    ChannelReservation **link = &active_channel_reservations;
    while (*link && *link != reservation)
        link = &(*link)->next;
    if (!*link)
        abort();
    if (reservation->committed || (reservation->completed && !reservation->deferred && reservation->publish_after_move))
    {
        if (reservation->receive)
            channel->front = reservation->next_front;
        channel->size = reservation->next_size;
    }
    *link = reservation->next;
    dvm_runtime_channel_buffer_notify();
    return 1;
}


/* Move a built-in value out of the ring and clear any mapped source owners. */
static int consume_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    reservation->deferred = !dvm_runtime_mapped_buffer_transfer(reservation->read.slot,
        reservation->read.destination, reservation->transaction_type,
        reservation->transaction_type->np != 0, &reservation->committed,
        reservation->replay_opcode == CHANNEL_ALT_TRY ? 0 : reservation->replay_opcode, NULL);
    return 1;
}


/* Consume the captured receive-array owner exactly once, outside the queue guard. */
static int release_channel_read_array(void *array)
{
    destroy(array);
    return 1;
}


/* Keep the receive array live through the mover and its mapped cleanup. */
static int read_buffer_owned(Channel *channel, void *destination, int replay_opcode)
{
    ChannelReservation reservation = {{channel, nil, nil, destination}, 1, 0, 0, 0, 0, nil, 0, 0, nil, nil, replay_opcode, 0, nil};
    int available = claim_channel_buffer(&reservation);
    if (available != 1)
    {
        return available != 0;
    }
    reservation.completed = reservation.transaction_type ? kprotect(consume_channel_buffer, &reservation) :
        kprotect(channelread, &reservation.read);
    dvm_runtime_channel_queue_guard(finish_channel_buffer, &reservation);
    if (!reservation.completed)
    {
        char first_error[ERRMAX];
        kgerrstr(first_error, sizeof(first_error));
        kprotect(release_channel_read_array, reservation.read.array);
        error(first_error);
    }
    destroy(reservation.read.array);
    return 1;
}


/* Invoke the mover with the captured geometry outside the queue mutex. */
static int write_channel_buffer(void *argument)
{
    ChannelReservation *reservation = argument;
    ChannelRead *read = &reservation->read;
    Channel *channel = read->channel;
    DVM_R.s = read->destination;
    DVM_R.m = &channel->mid;
    DVM_R.d = read->slot;
    if (reservation->transaction_type)
    {
        reservation->deferred = !dvm_runtime_mapped_buffer_transfer(read->destination,
            read->slot, reservation->transaction_type, 0, &reservation->committed,
            reservation->replay_opcode == CHANNEL_ALT_TRY ? 0 : reservation->replay_opcode,
            reservation->refill_sender);
    }
    else
    {
        channel->mover();
    }
    return 1;
}


/* Capture send geometry under the mutex and transfer outside it. */
static int write_buffer_owned(Channel *channel, void *source, int replay_opcode, int *committed,
    Prog *refill_sender)
{
    ChannelReservation reservation = {{channel, nil, nil, source}, 0, 0, 0, 0, 0, nil, 0, 0, nil, nil, replay_opcode, 0, refill_sender};
    int available = claim_channel_buffer(&reservation);
    if (available != 1)
    {
        return available != 0;
    }
    reservation.completed = kprotect(write_channel_buffer, &reservation);
    dvm_runtime_channel_queue_guard(finish_channel_buffer, &reservation);
    if (committed)
        *committed = reservation.committed;
    if (!reservation.completed)
        krethrow();
    return 1;
}



typedef struct RootedBufferTransfer
{
    Channel *channel;
    void *operand;
    int receive;
    int transferred;
    int replay_opcode; /* Zero for refill after claiming a peer. */
    int *committed; /* Optional publication result for a queued sender. */
    Prog *refill_sender;
} RootedBufferTransfer;


/* Report availability separately from the VM checkpoint result. */
static int run_rooted_buffer_transfer(void *argument)
{
    RootedBufferTransfer *transfer = argument;
    transfer->transferred = transfer->receive ?
        read_buffer_owned(transfer->channel, transfer->operand, transfer->replay_opcode) :
        write_buffer_owned(transfer->channel, transfer->operand, transfer->replay_opcode,
            transfer->committed, transfer->refill_sender);
    return 1;
}


/* Root the channel outside the queue mutex. Its owning buffer and mover
 * descriptor then survive callbacks through reservation completion. */
static int transfer_channel_buffer(Channel *channel, void *operand, int receive, int replay_opcode,
    int *committed, Prog *refill_sender)
{
    RootedBufferTransfer transfer = {channel, operand, receive, 0, replay_opcode, committed, refill_sender};
    DVM_HeapHold owner = {0};
    const char *problem;
    int completed;
    if (!guarded_channel_selected())
    {
        run_rooted_buffer_transfer(&transfer);
        return transfer.transferred;
    }
    problem = dvm_runtime_heap_hold(&channel, &Tchannel, 0, &owner);
    if (problem)
        error((char *)problem);
    completed = kprotect(run_rooted_buffer_transfer, &transfer);
    dvm_runtime_heap_finish(&owner, completed);
    return transfer.transferred;
}


static int cgetb(Channel *channel, void *destination)
{
    return transfer_channel_buffer(channel, destination, 1, 0, NULL, NULL);
}

static int cputb(Channel *channel, void *source)
{
    return transfer_channel_buffer(channel, source, 0, 0, NULL, NULL);
}

static int refill_put(Channel *channel, void *source, int *committed, Prog *sender)
{
    return transfer_channel_buffer(channel, source, 0, 0, committed, sender);
}


"""
