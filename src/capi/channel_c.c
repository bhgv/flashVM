#include "flashvm/channel.h"
#include "flashvm/debug.h"
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
    FVM_Result (*retain_packet)(void *);
    FVM_Value value;
    char *owned_string;
    struct FVM_ChannelNode *next;
} FVM_ChannelNode;
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
    c->identity = fvm_sync_next_identity();
    if (!c->identity)
    {
        fvm_port_sync_destroy(c->sync);
        free(c);
        return FVM_ERROR;
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
    channel->file_path = (char *)malloc(strlen(path) + 1);
    if (!channel->file_path)
    {
        fvm_channel_destroy(channel);
        return FVM_OUT_OF_MEMORY;
    }
    strcpy(channel->file_path, path);
    channel->file_channel = 1;
    *out = channel;
    return FVM_OK;
}
FVM_Result fvm_channel_is_file(FVM_Channel *channel, int *out)
{
    if (!channel || !out)
        return FVM_INVALID_ARGUMENT;
    *out = channel->file_channel == 1;
    return FVM_OK;
}
FVM_Result fvm_channel_file_read(FVM_Channel *c, void *data, size_t size, size_t *read)
{
    FVM_Result result;
    if (!c || !read || (!data && size))
        return FVM_INVALID_ARGUMENT;
    *read = 0;
    fvm_port_sync_lock(c->sync);
    result = c->file_channel != 1 ? FVM_UNSUPPORTED
             : c->closed          ? FVM_ERROR
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
    result = c->file_channel != 1 ? FVM_UNSUPPORTED
             : c->closed          ? FVM_ERROR
                                  : fvm_port_file_write(c->file, data, size);
    fvm_port_sync_unlock(c->sync);
    return result;
}
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
    if (c && c->remote_request)
        return fvm_channel_stream_receive(0, c, data, size, blocking);
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
    if (c->remote_request)
        c->remote_request(c->remote, 0, 4, 0, 0, 0, 0, 0);
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
    if (c->remote_release)
        c->remote_release(c->remote);
    free(c->last_string);
    free(c->file_path);
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
    while ((!c->head || c->frozen) && !c->closed)
        fvm_port_sync_wait(c->sync);
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
    while ((!c->head || c->frozen) && !c->closed && blocking)
        fvm_port_sync_wait(c->sync);
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
                                   FVM_Result (*retain)(void *), int blocking)
{
    FVM_ChannelNode *n;
    if (!c || !packet || !release || !retain)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    n = (FVM_ChannelNode *)calloc(1, sizeof(*n));
    if (!n)
        return FVM_OUT_OF_MEMORY;
    n->packet = packet;
    n->release_packet = release;
    n->retain_packet = retain;
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
    while ((!c->head || c->frozen) && !c->closed && blocking)
        fvm_port_sync_wait(c->sync);
    if (!c->head || c->frozen)
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

FVM_Result fvm_debug_channel_freeze(FVM_Channel *c, int frozen)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    c->frozen = frozen != 0;
    fvm_port_sync_signal_all(c->sync);
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_debug_channel_state(FVM_Channel *c, FVM_DebugChannel *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (c->file_channel)
        return FVM_UNSUPPORTED;
    fvm_port_sync_lock(c->sync);
    out->identity = c->identity;
    out->pending = c->pending;
    out->capacity = c->capacity;
    out->closed = c->closed;
    out->frozen = c->frozen;
    fvm_port_sync_unlock(c->sync);
    return FVM_OK;
}
FVM_Result fvm_channel_peek_mixed(FVM_Channel *c, size_t index, FVM_Value *out, void **packet)
{
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
            size = strlen(out->string) + 1;
            copy = (char *)malloc(size);
            if (!copy)
            {
                fvm_value_undefined(out);
                result = FVM_OUT_OF_MEMORY;
            }
            else
            {
                memcpy(copy, out->string, size);
                out->string = copy;
            }
        }
    }
    fvm_port_sync_unlock(c->sync);
    return result;
}

unsigned long fvm_channel_identity(FVM_Channel *c)
{
    return c ? c->identity : 0;
}

FVM_Result fvm_channel_create_remote(void *remote, FVM_RemoteChannelFn request,
                                     void (*release)(void *), const char *address,
                                     FVM_Channel **out)
{
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
const char *fvm_channel_remote_address(FVM_Channel *c)
{
    return c ? c->remote_address : 0;
}
FVM_Result fvm_channel_is_stream(FVM_Channel *c, int *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    *out = c->file_channel != 0;
    return FVM_OK;
}
FVM_Result fvm_channel_is_closed(FVM_Channel *c, int *out)
{
    FVM_Result r;
    unsigned char *data;
    size_t size;
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
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
FVM_Result fvm_channel_stream_send(FVM_Context *context, FVM_Channel *c, const unsigned char *data,
                                   size_t size)
{
    int closed;
    if (!c || !data || size < 16 || size > 64UL * 1024 * 1024)
        return FVM_INVALID_ARGUMENT;
    if (!c->remote_request)
        return fvm_channel_file_send_record(c, data, size);
    fvm_port_sync_lock(c->sync);
    closed = c->closed;
    fvm_port_sync_unlock(c->sync);
    return closed ? FVM_ERROR : c->remote_request(c->remote, context, 5, data, size, 0, 0, 0);
}
FVM_Result fvm_channel_stream_receive(FVM_Context *context, FVM_Channel *c, unsigned char **data,
                                      size_t *size, int blocking)
{
    int closed;
    if (!c || !data || !size)
        return FVM_INVALID_ARGUMENT;
    *data = 0;
    *size = 0;
    if (!c->remote_request)
        return fvm_channel_file_receive_record(c, data, size, blocking);
    fvm_port_sync_lock(c->sync);
    closed = c->closed;
    fvm_port_sync_unlock(c->sync);
    return closed ? FVM_ERROR
                  : c->remote_request(c->remote, context, 6, 0, 0, data, size, blocking);
}

const char *fvm_channel_file_path(FVM_Channel *c)
{
    return c ? c->file_path : 0;
}
