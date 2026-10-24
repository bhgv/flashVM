/* FlashVM: C embedding API: channel.
 * File: src/capi/channel_c.c.
 */

#include "graph_constants.h"
#include "../../ports/channel_sync.h"
#include "../../ports/file_port.h"
#include "../core/channel_packet.h"
#include "flashvm/channel.h"
#include "flashvm/debug.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>
/* Define FVM_ChannelNode struct and its representation. */
typedef struct FVM_ChannelNode
{
    void *packet;
    void (*release_packet)(void *);
    FVM_Result (*retain_packet)(void *);
    FVM_Value value;
    char *owned_string;
    struct FVM_ChannelNode *next;
    int *receipt; /* Borrowed sender-stack acknowledgement, protected by channel sync. */
} FVM_ChannelNode;

/* Define FVM_Channel struct and its representation. */
struct FVM_Channel
{
    unsigned long identity;
    FVM_PortSync *sync;
    void *remote;
    FVM_RemoteChannelFn remote_request;
    void (*remote_release)(void *);
    const char *remote_address;
    char *file_path;
    FVM_PortFile *file;
    int file_channel;
    FVM_ChannelNode *head;
    FVM_ChannelNode *tail;
    char *last_string;
    int closed;
    int frozen;
    size_t pending;
    size_t capacity;
    size_t references;
    size_t waiting_senders;   /* Native waiters, including rendezvous receipt waits. */
    size_t waiting_receivers; /* Native waiters for data or debugger thaw. */
    int rendezvous;           /* Explicit zero-buffer handshake mode. */
};

/* Record a native sleep under the same mutex used by observer snapshots. */
static void channel_wait(FVM_Channel *channel, int sending)
{
    size_t *count = sending ? &channel->waiting_senders : &channel->waiting_receivers;
    ++*count;
    fvm_port_sync_wait(channel->sync);
    --*count;
}

/* Capacity is independent of rendezvous: an offer occupies its one transfer slot. */
static int channel_send_waits(FVM_Channel *channel, int blocking)
{
    if (channel->rendezvous)
        return channel->pending != 0 || (!blocking && (!channel->waiting_receivers || channel->frozen));
    return channel->capacity && channel->pending >= channel->capacity;
}

/* The receiver acknowledges before freeing the node while holding this mutex.
 * On close, withdraw an unreceived offer before its sender stack disappears. */
static FVM_Result channel_finish_send(FVM_Channel *channel, FVM_ChannelNode *node, int *delivered)
{
    FVM_ChannelNode *previous;
    FVM_ChannelNode *current;
    fvm_port_sync_signal_all(channel->sync);
    if (!node->receipt)
        return FVM_OK;
    while (!*delivered && !channel->closed)
        channel_wait(channel, 1);
    if (*delivered)
        return FVM_OK;
    previous = 0;
    current = channel->head;
    while (current && current != node)
    {
        previous = current;
        current = current->next;
    }
    if (current)
    {
        if (previous)
            previous->next = node->next;
        else
            channel->head = node->next;
        if (channel->tail == node)
            channel->tail = previous;
        --channel->pending;
    }
    fvm_port_sync_signal_all(channel->sync);
    return FVM_ERROR;
}

/* Create channel. */
FVM_Result fvm_channel_create(FVM_Channel **out)
{
    FVM_Channel *c;
    if (!out)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    *out = 0;
    c = (FVM_Channel *)calloc(1, sizeof(*c));
    if (!c)
        return FVM_OUT_OF_MEMORY;
    c->sync = fvm_port_sync_create();
    if (!c->sync)
    {
        free(c);
        return FVM_OUT_OF_MEMORY;
    }
    c->identity = fvm_sync_next_identity();
    if (!c->identity)
    {
        fvm_port_sync_destroy(c->sync);
        free(c);
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    c->references = 1;
    *out = c;
    return FVM_OK;
}

/* Open channel file. */
FVM_Result fvm_channel_open_file(const char *path, const char *mode, FVM_Channel **out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Channel *channel;
    FVM_Result result;
    if (!out)
        return FVM_INVALID_ARGUMENT;
    *out = 0;
    result = fvm_channel_create(&channel);
    if (result != FVM_OK)
        return result;
    result = fvm_port_file_open(path, mode, &channel->file);
    if (result != FVM_OK)
    {
        fvm_channel_destroy(channel);
        return result;
    }
    channel->file_path = (char *)malloc(strlen(path) + 1);
    if (!channel->file_path)
    {
        fvm_channel_destroy(channel);
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    strcpy(channel->file_path, path);
    channel->file_channel = 1;
    *out = channel;
    return FVM_OK;
}

/* Test whether channel file. */
FVM_Result fvm_channel_is_file(FVM_Channel *channel, int *out)
{
    if (!channel || !out)
        return FVM_INVALID_ARGUMENT;
    *out = channel->file_channel == 1;
    return FVM_OK;
}

/* Read channel file. */
FVM_Result fvm_channel_file_read(FVM_Channel *c, void *data, size_t size, size_t *read)
{
    FVM_Result result;
    if (!c || !read || (!data && size))
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    *read = 0;
    fvm_port_sync_lock(c->sync);
    result = c->file_channel != 1 ? FVM_UNSUPPORTED
             : c->closed          ? FVM_ERROR
             : c->frozen          ? FVM_NOT_FOUND
                                  : fvm_port_file_read(c->file, data, size, read);
    fvm_port_sync_unlock(c->sync);
    return result;
}
/* Write channel file. */
FVM_Result fvm_channel_file_write(FVM_Channel *c, const void *data, size_t size)
{
    FVM_Result result;
    if (!c || (!data && size))
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    result = c->file_channel != 1 ? FVM_UNSUPPORTED
             : c->closed          ? FVM_ERROR
                                  : fvm_port_file_write(c->file, data, size);
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Send channel file record. */
FVM_Result fvm_channel_file_send_record(FVM_Channel *c, const unsigned char *data, size_t size)
{
    if (!data || size < 16 || size > 64UL * 1024 * 1024)
        return FVM_INVALID_ARGUMENT;
    if (c && c->remote_request)
        return fvm_channel_stream_send(0, c, data, size);
    return fvm_channel_file_write(c, data, size);
}
/* Lock held. Invalid/truncated records leave the read position unchanged;
   EOF is finite: blocking receive never waits for an external file writer. */
/* Compute and return file record. */
static FVM_Result file_record(FVM_Channel *c, unsigned char **data, size_t *size, int blocking,
                              int inspecting, size_t budget)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned char header[16], *buffer;
    const unsigned char magic[8] = {'F', 'V', 'M', 'G', 0, 0, 0, 1};
    size_t count, body;
    long position;
    FVM_Result result;

    /* Computation / Вычисление и обновление состояния. */
    *data = 0;
    *size = 0;
    while (c->frozen && !c->closed && blocking && !inspecting)
        channel_wait(c, 0);
    if (c->closed)
        return FVM_ERROR;
    if (c->frozen && !inspecting)
        return FVM_NOT_FOUND;

    /* Computation / Вычисление и обновление состояния. */
    position = fvm_port_file_position(c->file);
    result = fvm_port_file_read(c->file, header, sizeof(header), &count);
    if (result != FVM_OK)
        goto fail;
    if (!count)
        return blocking ? FVM_ERROR : FVM_NOT_FOUND;
    if (count != sizeof(header) || memcmp(header, magic, 7) || (header[FVM_GRAPH_VERSION_OFFSET] < FVM_GRAPH_MIN_WIRE_VERSION || header[FVM_GRAPH_VERSION_OFFSET] > FVM_GRAPH_WIRE_VERSION))
    {
        result = FVM_INVALID_ARGUMENT;
        goto fail;
    }
    body = ((size_t)header[8] << 24) | ((size_t)header[9] << 16) | ((size_t)header[10] << 8) |
           header[11];
    if (body > 64UL * 1024 * 1024 - 16)
    {
        result = FVM_UNSUPPORTED;
        goto fail;
    }
    /* Reject a record beyond the inspection budget before allocating or reading its body. */
    if (body + 16 > budget)
    {
        result = FVM_UNSUPPORTED;
        goto fail;
    }
    buffer = (unsigned char *)malloc(body + 16);
    if (!buffer)
    {
        result = FVM_OUT_OF_MEMORY;
        goto fail;
    }

    /* Computation / Вычисление и обновление состояния. */
    memcpy(buffer, header, 16);
    result = fvm_port_file_read(c->file, buffer + 16, body, &count);
    if (result != FVM_OK || count != body)
    {
        free(buffer);
        if (result == FVM_OK)
            result = FVM_INVALID_ARGUMENT;
        goto fail;
    }

    /* Computation / Вычисление и обновление состояния. */
    *data = buffer;
    *size = body + 16;
    return FVM_OK;
fail:
    fvm_port_file_restore(c->file, position);
    return result;
}

/* Copy the indexed unread file record under the channel lock, restoring the cursor on every
 * outcome. Inspection bypasses debugger freezing but never bypasses closure or read permissions. */
FVM_Result fvm_channel_file_peek_record(FVM_Channel *c, size_t index, unsigned char **data,
                                        size_t *size)
{
    long position;
    size_t scanned = 0;
    FVM_Result result;
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    if (c->file_channel != 1)
        return FVM_UNSUPPORTED;
    /* At most 65536 records may be inspected in a single debugger lookup. */
    if (index >= 65536UL)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_ERROR;
    }
    position = fvm_port_file_position(c->file);
    for (;;)
    {
        result = file_record(c, data, size, 0, 1, 64UL * 1024 * 1024 - scanned);
        if (result != FVM_OK || !index)
            break;
        /* Include skipped framing and bodies in the shared 64 MiB lookup budget. */
        scanned += *size;
        free(*data);
        *data = 0;
        *size = 0;
        --index;
    }
    fvm_port_file_restore(c->file, position);
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Receive channel file record. */
FVM_Result fvm_channel_file_receive_record(FVM_Channel *c, unsigned char **data, size_t *size,
                                           int blocking)
{
    FVM_Result result;
    if (c && c->remote_request)
        return fvm_channel_stream_receive(0, c, data, size, blocking);
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    *data = 0;
    *size = 0;
    fvm_port_sync_lock(c->sync);
    result = c->file_channel ? file_record(c, data, size, blocking, 0, 64UL * 1024 * 1024)
                             : FVM_UNSUPPORTED;
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Compute and return file primitive. */
static FVM_Result file_primitive(FVM_Channel *c, FVM_Value *out, int blocking, int borrowed)
{

    /* Initialization / Инициализация локального состояния. */
    unsigned char *data;
    size_t size;
    long position;
    char *previous;
    FVM_Result result;
    fvm_value_undefined(out);
    if (c->remote_request)
    {
        result = fvm_channel_stream_receive(0, c, &data, &size, blocking);
        if (result != FVM_OK)
            return result;
        result = fvm_serialization_decode_primitive(data, size, out);
        free(data);
        if (result == FVM_OK && borrowed)
        {
            fvm_port_sync_lock(c->sync);
            previous = c->last_string;
            c->last_string = out->type == FVM_VALUE_STRING ? (char *)out->string : 0;
            free(previous);
            fvm_port_sync_unlock(c->sync);
        }
        return result;
    }
    fvm_port_sync_lock(c->sync);
    /* Wait before capturing the rollback cursor: another resumed reader may run first. */
    while (c->frozen && !c->closed && blocking)
        channel_wait(c, 0);
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_ERROR;
    }

    /* Computation / Вычисление и обновление состояния. */
    position = fvm_port_file_position(c->file);
    result = file_record(c, &data, &size, blocking, 0, 64UL * 1024 * 1024);
    if (result == FVM_OK)
    {
        result = fvm_serialization_decode_primitive(data, size, out);
        free(data);
        if (result != FVM_OK)
            fvm_port_file_restore(c->file, position);
    }
    if (result == FVM_OK && borrowed)
    {
        previous = c->last_string;
        c->last_string = out->type == FVM_VALUE_STRING ? (char *)out->string : 0;
        free(previous);
    }
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Close channel. */
void fvm_channel_close(FVM_Channel *c)
{
    if (!c)
        return;
    if (c->remote_request)
        c->remote_request(c->remote, 0, 4, 0, 0, 0, 0, 0);

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    c->closed = 1;
    fvm_port_file_close(c->file);
    c->file = 0;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
}
/* Retain channel. */
FVM_Result fvm_channel_retain(FVM_Channel *c)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    if (c->references == (size_t)-1)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_INVALID_ARGUMENT;
    }

    /* Computation / Вычисление и обновление состояния. */
    ++c->references;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

/* Release channel. */
void fvm_channel_release(FVM_Channel *c)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n, *next;
    int last;
    if (!c)
        return;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    last = (--c->references == 0);
    if (last)
    {
        c->closed = 1;
        fvm_port_sync_signal_all(c->sync);
    }
    fvm_port_sync_unlock(c->sync);
    if (!last)
        return;
    n = c->head;
    while (n)
    {
        next = n->next;
        if (n->release_packet)
            n->release_packet(n->packet);
        free(n->owned_string);
        free(n);
        n = next;
    }
    fvm_port_file_close(c->file);
    if (c->remote_release)
        c->remote_release(c->remote);

    /* Cleanup / Освобождение ресурсов. */
    free(c->last_string);
    free(c->file_path);
    fvm_port_sync_destroy(c->sync);
    free(c);
}

/* Destroy channel. */
void fvm_channel_destroy(FVM_Channel *c)
{
    fvm_channel_release(c);
}

/* Send channel primitive. */
FVM_Result fvm_channel_send_primitive(FVM_Channel *c, const FVM_Value *v, int blocking)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    int delivered = 0;
    FVM_Result completion;
    size_t len;
    if (!c || !v || v->type < FVM_VALUE_UNDEFINED || v->type >= FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    if (v->metatable)
        return FVM_UNSUPPORTED;
    if (c->file_channel)
    {
        unsigned char *data;
        size_t size;
        FVM_Result result;
        result = fvm_serialization_encode_primitive(v, &data, &size);
        if (result != FVM_OK)
            return result;
        result = fvm_channel_file_send_record(c, data, size);
        free(data);
        return result;
    }
    n = (FVM_ChannelNode *)calloc(1, sizeof(*n));
    if (!n)
        return FVM_OUT_OF_MEMORY;
    n->value = *v;
    if (v->type == FVM_VALUE_STRING)
    {
        if (!v->string)
        {
            free(n);
            return FVM_INVALID_ARGUMENT;
        }
        len = fvm_value_string_size(v);
        n->owned_string = (char *)malloc(len + 1);
        if (!n->owned_string)
        {
            free(n);
            return FVM_OUT_OF_MEMORY;
        }
        memcpy(n->owned_string, v->string, len);
        n->owned_string[len] = 0;
        n->value.string = n->owned_string;
    }
    fvm_port_sync_lock(c->sync);
    while (!c->closed && channel_send_waits(c, blocking))
    {
        if (!blocking)
        {
            fvm_port_sync_unlock(c->sync);
            free(n->owned_string);
            free(n);
            return FVM_NOT_FOUND;
        }
        channel_wait(c, 1);
    }
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        free(n->owned_string);
        free(n);
        return FVM_ERROR;
    }
    if (c->rendezvous && blocking)
        n->receipt = &delivered;
    if (c->tail)
        c->tail->next = n;
    else
        c->head = n;

    /* Computation / Вычисление и обновление состояния. */
    c->tail = n;
    ++c->pending;
    completion = channel_finish_send(c, n, &delivered);
    fvm_port_sync_unlock(c->sync);
    if (completion != FVM_OK)
    {
        free(n->owned_string);
        free(n);
    }
    return completion;
}

/* Receive channel. */
FVM_Result fvm_channel_receive(FVM_Channel *c, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    char *previous;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, 1, 1);
    fvm_port_sync_lock(c->sync);
    while ((!c->head || c->frozen) && !c->closed)
        channel_wait(c, 0);
    if (!c->head || c->frozen)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_ERROR;
    }
    n = c->head;
    if (n->packet)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_UNSUPPORTED;
    }
    c->head = n->next;
    if (!c->head)
        c->tail = 0;

    /* Computation / Вычисление и обновление состояния. */
    --c->pending;
    if (n->receipt)
        *n->receipt = 1;
    fvm_port_sync_signal_all(c->sync);
    *out = n->value;
    previous = c->last_string;
    c->last_string = n->owned_string;
    fvm_port_sync_unlock(c->sync);

    /* Cleanup / Освобождение ресурсов. */
    free(previous);
    free(n);
    return FVM_OK;
}
/* Attempt channel receive. */
FVM_Result fvm_channel_try_receive(FVM_Channel *c, FVM_Value *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    char *previous;
    FVM_Result result;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, 0, 1);
    fvm_port_sync_lock(c->sync);
    if (!c->head || c->frozen)
    {
        result = c->closed ? FVM_ERROR : FVM_NOT_FOUND;
        fvm_port_sync_unlock(c->sync);
        return result;
    }
    n = c->head;
    if (n->packet)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_UNSUPPORTED;
    }
    c->head = n->next;
    if (!c->head)
        c->tail = 0;

    /* Computation / Вычисление и обновление состояния. */
    --c->pending;
    if (n->receipt)
        *n->receipt = 1;
    fvm_port_sync_signal_all(c->sync);
    *out = n->value;
    previous = c->last_string;
    c->last_string = n->owned_string;
    fvm_port_sync_unlock(c->sync);

    /* Cleanup / Освобождение ресурсов. */
    free(previous);
    free(n);
    return FVM_OK;
}

/* Compute and return channel available. */
FVM_Result fvm_channel_available(FVM_Channel *c, int *available)
{
    if (!c || !available)
        return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    if (c->file_channel)
    {
        FVM_Result result;
        *available = 0;
        result = c->closed ? FVM_OK : fvm_port_file_available(c->file, available);
        fvm_port_sync_unlock(c->sync);
        return result;
    }

    /* Computation / Вычисление и обновление состояния. */
    *available = (c->head != 0);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

/* Update channel capacity. */
FVM_Result fvm_channel_set_capacity(FVM_Channel *c, size_t capacity)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    c->capacity = capacity;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

/* Compute and return channel pending. */
FVM_Result fvm_channel_pending(FVM_Channel *c, size_t *pending)
{
    if (!c || !pending)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    *pending = c->pending;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

/* Send channel. */
FVM_Result fvm_channel_send(FVM_Channel *c, const FVM_Value *v)
{
    return fvm_channel_send_primitive(c, v, 1);
}
/* Attempt channel send. */
FVM_Result fvm_channel_try_send(FVM_Channel *c, const FVM_Value *v)
{
    return fvm_channel_send_primitive(c, v, 0);
}

/* Receive channel owned. */
FVM_Result fvm_channel_receive_owned(FVM_Channel *c, FVM_Value *out, int blocking)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    FVM_Result r;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, blocking, 0);
    fvm_port_sync_lock(c->sync);
    while ((!c->head || c->frozen) && !c->closed && blocking)
        channel_wait(c, 0);
    if (!c->head || c->frozen)
    {
        r = c->closed ? FVM_ERROR : FVM_NOT_FOUND;
        fvm_port_sync_unlock(c->sync);
        return r;
    }
    n = c->head;
    if (n->packet)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_UNSUPPORTED;
    }
    c->head = n->next;
    if (!c->head)
        c->tail = 0;

    /* Computation / Вычисление и обновление состояния. */
    --c->pending;
    if (n->receipt)
        *n->receipt = 1;
    *out = n->value;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    free(n);
    return FVM_OK;
}

/* Release channel message. */
void fvm_channel_message_release(FVM_Value *v)
{
    if (!v)
        return;
    if (v->type == FVM_VALUE_STRING)
        free((void *)v->string);
    fvm_value_undefined(v);
}

/* Send channel packet. */
FVM_Result fvm_channel_packet_send(FVM_Channel *c, void *packet, void (*release)(void *),
                                   FVM_Result (*retain)(void *), int blocking)
{
    FVM_ChannelNode *n;
    int delivered = 0;
    FVM_Result completion;
    if (!c || !packet || !release || !retain)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    n = (FVM_ChannelNode *)calloc(1, sizeof(*n));
    if (!n)
        return FVM_OUT_OF_MEMORY;
    n->packet = packet;
    n->release_packet = release;

    /* Computation / Вычисление и обновление состояния. */
    n->retain_packet = retain;
    fvm_port_sync_lock(c->sync);
    while (!c->closed && channel_send_waits(c, blocking))
    {
        if (!blocking)
        {
            fvm_port_sync_unlock(c->sync);
            free(n);
            return FVM_NOT_FOUND;
        }
        channel_wait(c, 1);
    }
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        free(n);
        return FVM_ERROR;
    }
    if (c->rendezvous && blocking)
        n->receipt = &delivered;
    if (c->tail)
        c->tail->next = n;
    else
        c->head = n;

    /* Computation / Вычисление и обновление состояния. */
    c->tail = n;
    ++c->pending;
    completion = channel_finish_send(c, n, &delivered);
    fvm_port_sync_unlock(c->sync);
    if (completion != FVM_OK)
    {
        free(n);
    }
    return completion;
}

/* Receive channel mixed. */
FVM_Result fvm_channel_receive_mixed(FVM_Channel *c, FVM_Value *out, void **packet, int blocking)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    FVM_Result r;
    if (!c || !out || !packet)
        return FVM_INVALID_ARGUMENT;
    *packet = 0;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    while ((!c->head || c->frozen) && !c->closed && blocking)
        channel_wait(c, 0);
    if (!c->head || c->frozen)
    {
        r = c->closed ? FVM_ERROR : FVM_NOT_FOUND;
        fvm_port_sync_unlock(c->sync);
        return r;
    }

    /* Computation / Вычисление и обновление состояния. */
    n = c->head;
    c->head = n->next;
    if (!c->head)
        c->tail = 0;

    /* Computation / Вычисление и обновление состояния. */
    --c->pending;
    if (n->receipt)
        *n->receipt = 1;
    *packet = n->packet;
    *out = n->value;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    free(n);
    return FVM_OK;
}

/* Suspend local queue delivery or file reads under the channel lock and wake resumed waiters. */
FVM_Result fvm_debug_channel_freeze(FVM_Channel *c, int frozen)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (c->remote_request)
    {
        unsigned char input = frozen != 0;
        unsigned char *response = 0;
        size_t size = 0;
        FVM_Result result = c->remote_request(c->remote, 0, 7, &input, 1, &response, &size, 0);
        if (result == FVM_OK && size)
            result = FVM_ERROR;
        free(response);
        return result;
    }

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    c->frozen = frozen != 0;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

/* Decode a portable unsigned 32-bit control field without host endian or alignment assumptions. */
static unsigned long debug_word(const unsigned char *data)
{
    return ((unsigned long)data[0] << 24) | ((unsigned long)data[1] << 16) |
           ((unsigned long)data[2] << 8) | data[3];
}

/* Capture queue metadata or count framed unread local file records without consuming them.
 * File scanning runs under the channel lock; malformed data leaves both cursor and output intact.
 */
FVM_Result fvm_debug_channel_state(FVM_Channel *c, FVM_DebugChannel *out)
{
    FVM_DebugChannel state;
    FVM_Result result;
    unsigned char *data;
    size_t size, scanned;
    long position;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->remote_request)
    {
        unsigned char *response = 0;
        size_t length = 0;
        result = c->remote_request(c->remote, 0, 8, 0, 0, &response, &length, 0);
        if (result == FVM_OK)
        {
            if (length != 12 || !response || debug_word(response + 8) > 3)
                result = FVM_ERROR;
            else
            {
                state.identity = c->identity;
                state.pending = (size_t)debug_word(response);
                state.capacity = (size_t)debug_word(response + 4);
                state.closed = (debug_word(response + 8) & 1) != 0;
                state.frozen = (debug_word(response + 8) & 2) != 0;
                *out = state;
            }
        }
        free(response);
        return result;
    }

    fvm_port_sync_lock(c->sync);
    state.identity = c->identity;
    state.pending = c->pending;
    state.capacity = c->capacity;
    state.closed = c->closed;
    state.frozen = c->frozen;
    result = FVM_OK;
    if (c->file_channel)
    {
        state.pending = 0;
        state.capacity = 0;
        if (!c->closed)
        {
            position = fvm_port_file_position(c->file);
            scanned = 0;
            for (;;)
            {
                result = file_record(c, &data, &size, 0, 1,
                                     state.pending >= 65536 ? 0 : 64UL * 1024 * 1024 - scanned);
                if (result != FVM_OK)
                    break;
                free(data);
                /* Bound inspection even if another process continuously appends records. */
                if (state.pending >= 65536 || size > 64UL * 1024 * 1024 - scanned)
                {
                    result = FVM_UNSUPPORTED;
                    break;
                }
                scanned += size;
                ++state.pending;
            }
            fvm_port_file_restore(c->file, position);
            if (result == FVM_NOT_FOUND)
                result = FVM_OK;
        }
    }
    if (result == FVM_OK)
        *out = state;
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Compute and return channel peek mixed. */
FVM_Result fvm_channel_peek_mixed(FVM_Channel *c, size_t index, FVM_Value *out, void **packet)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_ChannelNode *n;
    FVM_Result result;
    char *copy;
    size_t size;
    if (!c || !out || !packet)
        return FVM_INVALID_ARGUMENT;
    *packet = 0;
    fvm_value_undefined(out);
    if (c->file_channel)
        return FVM_UNSUPPORTED;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    n = c->head;
    while (n && index)
    {
        n = n->next;
        --index;
    }
    if (!n)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_NOT_FOUND;
    }
    result = FVM_OK;
    if (n->packet)
    {
        result = n->retain_packet(n->packet);
        if (result == FVM_OK)
            *packet = n->packet;
    }
    else
    {
        *out = n->value;
        if (out->type == FVM_VALUE_STRING)
        {
            size = fvm_value_string_size(out) + 1;
            copy = (char *)malloc(size);
            if (!copy)
            {
                fvm_value_undefined(out);
                result = FVM_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(copy, out->string, size - 1);
                copy[size - 1] = 0;
                out->string = copy;
            }
        }
    }
    fvm_port_sync_unlock(c->sync);
    return result;
}

/* Compute and return channel identity. */
unsigned long fvm_channel_identity(FVM_Channel *c)
{
    return c ? c->identity : 0;
}

/* Create channel remote. */
FVM_Result fvm_channel_create_remote(void *remote, FVM_RemoteChannelFn request,
                                     void (*release)(void *), const char *address,
                                     FVM_Channel **out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Result r;
    r = fvm_channel_create(out);
    if (r == FVM_OK)
    {
        (*out)->remote = remote;
        (*out)->remote_request = request;
        (*out)->remote_release = release;
        (*out)->remote_address = address;
        (*out)->file_channel = 2;
    }
    return r;
}

/* Compute and return channel remote address. */
const char *fvm_channel_remote_address(FVM_Channel *c)
{
    return c ? c->remote_address : 0;
}

/* Test whether channel stream. */
FVM_Result fvm_channel_is_stream(FVM_Channel *c, int *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    *out = c->file_channel != 0;
    return FVM_OK;
}

/* Test whether channel closed. */
FVM_Result fvm_channel_is_closed(FVM_Channel *c, int *out)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Result r;
    unsigned char *data;
    size_t size;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    *out = c->closed;
    fvm_port_sync_unlock(c->sync);
    if (!*out && c->remote_request)
    {
        data = 0;
        size = 0;
        r = c->remote_request(c->remote, 0, 0, 0, 0, &data, &size, 0);
        if (r == FVM_OK && size == 1)
            *out = data[0] != 0;
        else if (r == FVM_OK)
            r = FVM_ERROR;
        free(data);
        return r;
    }
    return FVM_OK;
}

/* Send channel stream. */
FVM_Result fvm_channel_stream_send(FVM_Context *context, FVM_Channel *c, const unsigned char *data,
                                   size_t size)
{
    int closed;
    if (!c || !data || size < 16 || size > 64UL * 1024 * 1024)
        return FVM_INVALID_ARGUMENT;
    if (!c->remote_request)
        return fvm_channel_file_send_record(c, data, size);

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    closed = c->closed;
    fvm_port_sync_unlock(c->sync);
    return closed ? FVM_ERROR : c->remote_request(c->remote, context, 5, data, size, 0, 0, 0);
}

/* Receive channel stream. */
FVM_Result fvm_channel_stream_receive(FVM_Context *context, FVM_Channel *c, unsigned char **data,
                                      size_t *size, int blocking)
{
    int closed;
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    *data = 0;
    *size = 0;
    if (!c->remote_request)
        return fvm_channel_file_receive_record(c, data, size, blocking);

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_sync_lock(c->sync);
    closed = c->closed;
    fvm_port_sync_unlock(c->sync);
    return closed ? FVM_ERROR
                  : c->remote_request(c->remote, context, 6, 0, 0, data, size, blocking);
}

/* Copy an indexed remote or file message without consuming it or changing its freeze state. */
FVM_Result fvm_channel_stream_peek(FVM_Context *context, FVM_Channel *c, size_t index,
                                   unsigned char **data, size_t *size)
{
    int closed;
    unsigned char request[4];
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    if (!c->remote_request)
        return fvm_channel_file_peek_record(c, index, data, size);
    if (index > 4294967295UL)
        return FVM_UNSUPPORTED;
    request[0] = (unsigned char)(index >> 24);
    request[1] = (unsigned char)(index >> 16);
    request[2] = (unsigned char)(index >> 8);
    request[3] = (unsigned char)index;
    fvm_port_sync_lock(c->sync);
    closed = c->closed;
    fvm_port_sync_unlock(c->sync);
    return closed
               ? FVM_ERROR
               : c->remote_request(c->remote, context, 9, request, sizeof(request), data, size, 0);
}

/* Compute and return channel file path. */
const char *fvm_channel_file_path(FVM_Channel *c)
{
    return c ? c->file_path : 0;
}

/* Observe native waiters without waiting for a message, peer or free capacity. */
FVM_Result fvm_channel_wait_state(FVM_Channel *channel, FVM_ChannelWaitState *out)
{
    FVM_ChannelWaitState state;
    if (!channel || !out)
        return FVM_INVALID_ARGUMENT;
    if (channel->file_channel || channel->remote_request)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(channel->sync);
    state.pending = channel->pending;
    state.waiting_senders = channel->waiting_senders;
    state.waiting_receivers = channel->waiting_receivers;
    state.directions = 0;
    if (state.waiting_senders)
        state.directions |= FVM_CHANNEL_WAIT_SEND;
    if (state.waiting_receivers)
        state.directions |= FVM_CHANNEL_WAIT_RECEIVE;
    state.closed = channel->closed;
    state.frozen = channel->frozen;
    state.rendezvous = channel->rendezvous;
    *out = state;
    fvm_port_sync_unlock(channel->sync);
    return FVM_OK;
}

/* Mode changes require exclusive idle ownership, preserving existing queued data. */
FVM_Result fvm_channel_set_rendezvous(FVM_Channel *channel, int enabled)
{
    if (!channel || (enabled != 0 && enabled != 1))
        return FVM_INVALID_ARGUMENT;
    if (channel->file_channel || channel->remote_request)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(channel->sync);
    if (channel->closed || channel->pending || channel->waiting_senders || channel->waiting_receivers)
    {
        fvm_port_sync_unlock(channel->sync);
        return FVM_INVALID_ARGUMENT;
    }
    channel->rendezvous = enabled;
    fvm_port_sync_unlock(channel->sync);
    return FVM_OK;
}

/* Register a cancellable script wait without blocking its owning context. */
void fvm_channel_poll_wait_begin(FVM_Channel *channel, int sending)
{
    if (!channel || channel->file_channel || channel->remote_request)
        return;
    fvm_port_sync_lock(channel->sync);
    if (sending)
        ++channel->waiting_senders;
    else
        ++channel->waiting_receivers;
    fvm_port_sync_signal_all(channel->sync);
    fvm_port_sync_unlock(channel->sync);
}

/* End exactly one polling scope after success, cancellation, close or error. */
void fvm_channel_poll_wait_end(FVM_Channel *channel, int sending)
{
    if (!channel || channel->file_channel || channel->remote_request)
        return;
    fvm_port_sync_lock(channel->sync);
    if (sending)
        --channel->waiting_senders;
    else
        --channel->waiting_receivers;
    fvm_port_sync_unlock(channel->sync);
}
