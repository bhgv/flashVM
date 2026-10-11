#include "flashvm/channel.h"
#include "../core/channel_packet.h"
#include "../../ports/channel_sync.h"
#include "../../ports/file_port.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
typedef struct FVM_ChannelNode
{
    void *packet;
    void (*release_packet)(void *);
    FVM_Value value;
    char *owned_string;
    struct FVM_ChannelNode *next;
} FVM_ChannelNode;
struct FVM_Channel
{
    FVM_PortSync *sync;
    FVM_PortFile *file;
    int file_channel;
    FVM_ChannelNode *head;
    FVM_ChannelNode *tail;
    char *last_string;
    int closed;
    size_t pending;
    size_t capacity;
    size_t references;
};
FVM_Result fvm_channel_create(FVM_Channel **out)
{
    FVM_Channel *c;
    if (!out)
        return FVM_INVALID_ARGUMENT;
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
    c->references = 1;
    *out = c;
    return FVM_OK;
}
FVM_Result fvm_channel_open_file(const char *path, const char *mode, FVM_Channel **out)
{
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
    channel->file_channel = 1;
    *out = channel;
    return FVM_OK;
}
FVM_Result fvm_channel_is_file(FVM_Channel *channel, int *out)
{
    if (!channel || !out)
        return FVM_INVALID_ARGUMENT;
    *out = channel->file_channel;
    return FVM_OK;
}
FVM_Result fvm_channel_file_read(FVM_Channel *c, void *data, size_t size, size_t *read)
{
    FVM_Result result;
    if (!c || !read || (!data && size))
        return FVM_INVALID_ARGUMENT;
    *read = 0;
    fvm_port_sync_lock(c->sync);
    result = !c->file_channel ? FVM_UNSUPPORTED
             : c->closed      ? FVM_ERROR
                              : fvm_port_file_read(c->file, data, size, read);
    fvm_port_sync_unlock(c->sync);
    return result;
}
FVM_Result fvm_channel_file_write(FVM_Channel *c, const void *data, size_t size)
{
    FVM_Result result;
    if (!c || (!data && size))
        return FVM_INVALID_ARGUMENT;
    fvm_port_sync_lock(c->sync);
    result = !c->file_channel ? FVM_UNSUPPORTED
             : c->closed      ? FVM_ERROR
                              : fvm_port_file_write(c->file, data, size);
    fvm_port_sync_unlock(c->sync);
    return result;
}
FVM_Result fvm_channel_file_send_record(FVM_Channel *c, const unsigned char *data, size_t size)
{
    if (!data || size < 16 || size > 64UL * 1024 * 1024)
        return FVM_INVALID_ARGUMENT;
    return fvm_channel_file_write(c, data, size);
}
/* Lock held. Invalid/truncated records leave the read position unchanged;
   EOF is finite: blocking receive never waits for an external file writer. */
static FVM_Result file_record(FVM_Channel *c, unsigned char **data, size_t *size, int blocking)
{
    unsigned char header[16], *buffer;
    const unsigned char magic[8] = {'F', 'V', 'M', 'G', 0, 0, 0, 1};
    size_t count, body;
    long position;
    FVM_Result result;
    *data = 0;
    *size = 0;
    if (c->closed)
        return FVM_ERROR;
    position = fvm_port_file_position(c->file);
    result = fvm_port_file_read(c->file, header, sizeof(header), &count);
    if (result != FVM_OK)
        goto fail;
    if (!count)
        return blocking ? FVM_ERROR : FVM_NOT_FOUND;
    if (count != sizeof(header) || memcmp(header, magic, sizeof(magic)))
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
    buffer = (unsigned char *)malloc(body + 16);
    if (!buffer)
    {
        result = FVM_OUT_OF_MEMORY;
        goto fail;
    }
    memcpy(buffer, header, 16);
    result = fvm_port_file_read(c->file, buffer + 16, body, &count);
    if (result != FVM_OK || count != body)
    {
        free(buffer);
        if (result == FVM_OK)
            result = FVM_INVALID_ARGUMENT;
        goto fail;
    }
    *data = buffer;
    *size = body + 16;
    return FVM_OK;
fail:
    fvm_port_file_restore(c->file, position);
    return result;
}
FVM_Result fvm_channel_file_receive_record(FVM_Channel *c, unsigned char **data, size_t *size,
                                           int blocking)
{
    FVM_Result result;
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    fvm_port_sync_lock(c->sync);
    result = c->file_channel ? file_record(c, data, size, blocking) : FVM_UNSUPPORTED;
    fvm_port_sync_unlock(c->sync);
    return result;
}
static FVM_Result file_primitive(FVM_Channel *c, FVM_Value *out, int blocking, int borrowed)
{
    unsigned char *data;
    size_t size;
    long position;
    char *previous;
    FVM_Result result;
    fvm_value_undefined(out);
    fvm_port_sync_lock(c->sync);
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        return FVM_ERROR;
    }
    position = fvm_port_file_position(c->file);
    result = file_record(c, &data, &size, blocking);
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
void fvm_channel_close(FVM_Channel *c)
{
    if (!c)
        return;
    fvm_port_sync_lock(c->sync);
    c->closed = 1;
    fvm_port_file_close(c->file);
    c->file = 0;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
}
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
    ++c->references;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
void fvm_channel_release(FVM_Channel *c)
{
    FVM_ChannelNode *n, *next;
    int last;
    if (!c)
        return;
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
    free(c->last_string);
    fvm_port_sync_destroy(c->sync);
    free(c);
}
void fvm_channel_destroy(FVM_Channel *c)
{
    fvm_channel_release(c);
}
FVM_Result fvm_channel_send_primitive(FVM_Channel *c, const FVM_Value *v, int blocking)
{
    FVM_ChannelNode *n;
    size_t len;
    if (!c || !v || v->type < FVM_VALUE_UNDEFINED || v->type >= FVM_VALUE_OBJECT)
        return FVM_INVALID_ARGUMENT;
    if (v->metatable) return FVM_UNSUPPORTED;
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
        len = strlen(v->string);
        n->owned_string = (char *)malloc(len + 1);
        if (!n->owned_string)
        {
            free(n);
            return FVM_OUT_OF_MEMORY;
        }
        memcpy(n->owned_string, v->string, len + 1);
        n->value.string = n->owned_string;
    }
    fvm_port_sync_lock(c->sync);
    while (!c->closed && c->capacity && c->pending >= c->capacity)
    {
        if (!blocking)
        {
            fvm_port_sync_unlock(c->sync);
            free(n->owned_string);
            free(n);
            return FVM_NOT_FOUND;
        }
        fvm_port_sync_wait(c->sync);
    }
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        free(n->owned_string);
        free(n);
        return FVM_ERROR;
    }
    if (c->tail)
        c->tail->next = n;
    else
        c->head = n;
    c->tail = n;
    ++c->pending;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_receive(FVM_Channel *c, FVM_Value *out)
{
    FVM_ChannelNode *n;
    char *previous;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, 1, 1);
    fvm_port_sync_lock(c->sync);
    while (!c->head && !c->closed)
        fvm_port_sync_wait(c->sync);
    if (!c->head)
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
    --c->pending;
    fvm_port_sync_signal_all(c->sync);
    *out = n->value;
    previous = c->last_string;
    c->last_string = n->owned_string;
    fvm_port_sync_unlock(c->sync);
    free(previous);
    free(n);
    return FVM_OK;
}
FVM_Result fvm_channel_try_receive(FVM_Channel *c, FVM_Value *out)
{
    FVM_ChannelNode *n;
    char *previous;
    FVM_Result result;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, 0, 1);
    fvm_port_sync_lock(c->sync);
    if (!c->head)
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
    --c->pending;
    fvm_port_sync_signal_all(c->sync);
    *out = n->value;
    previous = c->last_string;
    c->last_string = n->owned_string;
    fvm_port_sync_unlock(c->sync);
    free(previous);
    free(n);
    return FVM_OK;
}
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
    *available = (c->head != 0);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

FVM_Result fvm_channel_set_capacity(FVM_Channel *c, size_t capacity)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    c->capacity = capacity;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_pending(FVM_Channel *c, size_t *pending)
{
    if (!c || !pending)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    *pending = c->pending;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}

FVM_Result fvm_channel_send(FVM_Channel *c, const FVM_Value *v)
{
    return fvm_channel_send_primitive(c, v, 1);
}
FVM_Result fvm_channel_try_send(FVM_Channel *c, const FVM_Value *v)
{
    return fvm_channel_send_primitive(c, v, 0);
}
FVM_Result fvm_channel_receive_owned(FVM_Channel *c, FVM_Value *out, int blocking)
{
    FVM_ChannelNode *n;
    FVM_Result r;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return file_primitive(c, out, blocking, 0);
    fvm_port_sync_lock(c->sync);
    while (!c->head && !c->closed && blocking)
        fvm_port_sync_wait(c->sync);
    if (!c->head)
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
    --c->pending;
    *out = n->value;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    free(n);
    return FVM_OK;
}
void fvm_channel_message_release(FVM_Value *v)
{
    if (!v)
        return;
    if (v->type == FVM_VALUE_STRING)
        free((void *)v->string);
    fvm_value_undefined(v);
}

FVM_Result fvm_channel_packet_send(FVM_Channel *c, void *packet, void (*release)(void *),
                                   int blocking)
{
    FVM_ChannelNode *n;
    if (!c || !packet || !release)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    n = (FVM_ChannelNode *)calloc(1, sizeof(*n));
    if (!n)
        return FVM_OUT_OF_MEMORY;
    n->packet = packet;
    n->release_packet = release;
    fvm_port_sync_lock(c->sync);
    while (!c->closed && c->capacity && c->pending >= c->capacity)
    {
        if (!blocking)
        {
            fvm_port_sync_unlock(c->sync);
            free(n);
            return FVM_NOT_FOUND;
        }
        fvm_port_sync_wait(c->sync);
    }
    if (c->closed)
    {
        fvm_port_sync_unlock(c->sync);
        free(n);
        return FVM_ERROR;
    }
    if (c->tail)
        c->tail->next = n;
    else
        c->head = n;
    c->tail = n;
    ++c->pending;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_receive_mixed(FVM_Channel *c, FVM_Value *out, void **packet, int blocking)
{
    FVM_ChannelNode *n;
    FVM_Result r;
    if (!c || !out || !packet)
        return FVM_INVALID_ARGUMENT;
    *packet = 0;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    while (!c->head && !c->closed && blocking)
        fvm_port_sync_wait(c->sync);
    if (!c->head)
    {
        r = c->closed ? FVM_ERROR : FVM_NOT_FOUND;
        fvm_port_sync_unlock(c->sync);
        return r;
    }
    n = c->head;
    c->head = n->next;
    if (!c->head)
        c->tail = 0;
    --c->pending;
    *packet = n->packet;
    *out = n->value;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    free(n);
    return FVM_OK;
}
