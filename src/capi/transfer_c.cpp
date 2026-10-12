/* FlashVM: C embedding API: transfer.
 * File: src/capi/transfer_c.cpp.
 */

#include "../core/string_internal.hpp"
#include "../core/runtime.hpp"
#include "../core/exception_internal.hpp"
#include "../core/channel_packet.h"
#include "../core/transfer_resource.hpp"
#include "../core/capability_internal.hpp"
#include "flashvm/extensions.h"
#include "flashvm/mutex.h"
#include "flashvm/debug.h"
#include "../../ports/channel_sync.h"
#include "../../ports/thread_port.h"
#include "flashvm/worker.h"
#include <climits>
#include "flashvm/transfer.h"
#include "flashvm/serialization.h"
#include <new>
#include <cstring>
#include <cstdlib>
namespace
{
/* Define Resource struct and its representation. */
struct Resource
{
    int kind;
    void *handle;
    unsigned long origin;
};
struct Packet;
Packet *queued_packets = 0;
/* Unregister packet. */
void unregister_packet(Packet *);

/* Define Packet struct and its representation. */
struct Packet
{
    Packet *next;
    FVM_Channel *target;
    fvm_detail::vector<Resource> resources;
    unsigned long references;
    unsigned char *data;
    size_t size;
    /* Initialize Packet and its owned state. */
    Packet() : next(0), target(0), references(1), data(0), size(0)
    {
    }
    /* Release the resources owned by this Packet instance. */
    ~Packet()
    {

        /* Computation / Вычисление и обновление состояния. */
        unregister_packet(this);
        fvm_serialized_free(data);
        for (size_t i = 0; i < resources.size(); ++i)
            if (resources[i].kind == 8)
                fvm_mutex_release((FVM_Mutex *)resources[i].handle);
            else
                fvm_channel_release((FVM_Channel *)resources[i].handle);
    }
};

/* Define RegistryScope struct and its representation. */
struct RegistryScope
{
    /* Initialize Registry Scope and its owned state. */
    RegistryScope()
    {
        fvm_port_registry_lock();
    }
    /* Release the resources owned by this RegistryScope instance. */
    ~RegistryScope()
    {
        fvm_port_registry_unlock();
    }
};
/* Unregister packet. */
void unregister_packet(Packet *p)
{
    RegistryScope guard;
    if (!p->target)
        return;
    Packet **link = &queued_packets;
    while (*link != p)
        link = &(*link)->next;

    /* Computation / Вычисление и обновление состояния. */
    *link = p->next;
    p->target = 0;
    p->next = 0;
}
/* Queue-owned capability cycles otherwise retain channels forever. Reserve an
   edge before publishing the packet so concurrent opposite sends cannot race.
   Only native handles are compared; no channel locks are taken under this lock. */
/* Register packet. */
FVM_Result register_packet(Packet *packet, FVM_Channel *target)
{
    RegistryScope guard;
    try
    {
        fvm_detail::vector<FVM_Channel *> reachable;
        for (size_t i = 0; i < packet->resources.size(); ++i)
            if (packet->resources[i].kind == 1)
                reachable.push_back((FVM_Channel *)packet->resources[i].handle);
        for (size_t i = 0; i < reachable.size(); ++i)
        {
            if (reachable[i] == target)
                return FVM_UNSUPPORTED;
            for (Packet *p = queued_packets; p; p = p->next)
                if (p->target == reachable[i])
                    for (size_t j = 0; j < p->resources.size(); ++j)
                        if (p->resources[j].kind == 1)
                        {
                            FVM_Channel *next = (FVM_Channel *)p->resources[j].handle;
                            bool seen = false;
                            for (size_t k = 0; k < reachable.size(); ++k)
                                if (reachable[k] == next)
                                    seen = true;
                            if (!seen)
                            {
                                if (reachable.size() >= 65536)
                                    return FVM_UNSUPPORTED;
                                reachable.push_back(next);
                            }
                        }
        }
        packet->target = target;
        packet->next = queued_packets;
        queued_packets = packet;
        return FVM_OK;
    }
    catch (...)
    {
        return FVM_OUT_OF_MEMORY;
    }
}

/* Release the current value. */
void release(void *packet)
{
    Packet *p = (Packet *)packet;
    fvm_port_registry_lock();
    bool last = --p->references == 0;
    fvm_port_registry_unlock();
    if (last)
        delete p;
}
/* Retain packet. */
FVM_Result retain_packet(void *packet)
{
    Packet *p = (Packet *)packet;
    fvm_port_registry_lock();
    FVM_Result r = p->references == ULONG_MAX ? FVM_ERROR : FVM_OK;
    if (r == FVM_OK)
        ++p->references;
    fvm_port_registry_unlock();
    return r;
}

/* Define ResourceScope struct and its representation. */
struct ResourceScope
{
    FVM_Context *context;
    void *previous;
    /* Initialize Resource Scope and its owned state. */
    ResourceScope(FVM_Context *c, Packet *p) : context(c), previous(c->transfer_resources)
    {
        c->transfer_resources = p;
    }
    /* Release the resources owned by this ResourceScope instance. */
    ~ResourceScope()
    {
        context->transfer_resources = previous;
    }
};
} // namespace
/* Capture transfer resource. */
FVM_Result fvm_transfer_capture_resource(FVM_Context *c, FVM_Object *o, unsigned long *token)
{
    if (!c->transfer_resources || !o->resource || (o->resource_kind != 1 && o->resource_kind != 8))
        return FVM_UNSUPPORTED;
    Packet *p = (Packet *)c->transfer_resources;
    for (size_t i = 0; i < p->resources.size(); ++i)
        if (p->resources[i].kind == o->resource_kind && p->resources[i].handle == o->resource)
        {
            *token = (unsigned long)i;
            return FVM_OK;
        }

    /* Initialization / Инициализация локального состояния. */
    int closed = 0;
    FVM_Result status = o->resource_kind == 8
                            ? fvm_mutex_is_closed((FVM_Mutex *)o->resource, &closed)
                            : fvm_channel_is_closed((FVM_Channel *)o->resource, &closed);
    if (status != FVM_OK || closed)
    {
        fvm_resource_fault(c, closed ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_DISCONNECTED, 0,
                           "capability.send", 0);
        fvm_resource_fault_resource(c, o->resource_kind, o->resource, o->resource_origin);
        fvm_debug_resource_error(c, 0, "capability.send", 0);
        return FVM_ERROR;
    }
    Resource resource;

    /* Computation / Вычисление и обновление состояния. */
    resource.kind = o->resource_kind;
    resource.handle = o->resource;
    resource.origin = o->resource_origin;
    FVM_Result r = resource.kind == 8 ? fvm_mutex_retain((FVM_Mutex *)resource.handle)
                                      : fvm_channel_retain((FVM_Channel *)resource.handle);
    if (r != FVM_OK)
        return r;
    try
    {
        p->resources.push_back(resource);
    }
    catch (...)
    {
        if (resource.kind == 8)
            fvm_mutex_release((FVM_Mutex *)resource.handle);
        else
            fvm_channel_release((FVM_Channel *)resource.handle);
        return FVM_OUT_OF_MEMORY;
    }
    *token = (unsigned long)p->resources.size() - 1;
    return FVM_OK;
}

/* Restore transfer resource. */
FVM_Result fvm_transfer_restore_resource(FVM_Context *c, unsigned long kind, unsigned long token,
                                         FVM_Object **out)
{
    if (!c->transfer_resources)
        return FVM_UNSUPPORTED;
    Packet *p = (Packet *)c->transfer_resources;
    if (token >= p->resources.size() || kind != (unsigned long)p->resources[token].kind)
        return FVM_INVALID_ARGUMENT;

    /* Initialization / Инициализация локального состояния. */
    Resource &r = p->resources[token];
    int closed = 0;
    FVM_Result status = kind == 8 ? fvm_mutex_is_closed((FVM_Mutex *)r.handle, &closed)
                                  : fvm_channel_is_closed((FVM_Channel *)r.handle, &closed);
    if (status != FVM_OK || closed)
    {
        fvm_resource_fault(c, closed ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_DISCONNECTED, 0,
                           "capability.receive", 0);
        fvm_resource_fault_resource(c, kind, r.handle, r.origin);
        fvm_debug_resource_error(c, 0, "capability.receive", 0);
        return FVM_ERROR;
    }
    FVM_Result result = kind == 8 ? fvm_extensions_wrap_mutex(c, (FVM_Mutex *)r.handle, out)
                                  : fvm_extensions_wrap_channel(c, (FVM_Channel *)r.handle, out);
    if (result == FVM_OK && kind == 1)
        (*out)->resource_origin = r.origin;
    return result;
}

/* Send channel value. */
extern "C" FVM_Result fvm_channel_send_value(FVM_Context *context, FVM_Channel *channel,
                                             const FVM_Value *value, int blocking)
{
    int file = 0;
    if (!context || !channel || !value || context->destroying)
        return FVM_INVALID_ARGUMENT;
    if (fvm_validate_value(context, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    fvm_channel_is_stream(channel, &file);
    if (!file && value->type != FVM_VALUE_OBJECT && !value->metatable)
    {
        FVM_Result result = fvm_channel_send_primitive(channel, value, blocking);
        if (result == FVM_ERROR)
        {
            int closed = 0;
            fvm_channel_is_closed(channel, &closed);
            fvm_resource_fault(context, closed ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_FAILURE,
                               fvm_channel_identity(channel), "channel.send", 0);
            fvm_debug_resource_error(context, fvm_channel_identity(channel), "channel.send", 0);
        }
        return result;
    }
    Packet *packet = new (std::nothrow) Packet;
    if (!packet)
        return FVM_OUT_OF_MEMORY;
    FVM_Result result;
    {
        ResourceScope scope(context, file ? 0 : packet);
        result = fvm_value_serialize(context, value, &packet->data, &packet->size);
    }
    if (result == FVM_OK)
    {
        if (file)
        {
            do
            {
                result = fvm_channel_stream_send(context, channel, packet->data, packet->size);
                if (result != FVM_NOT_FOUND || !blocking)
                    break;
                int stop = 0;
                if (context->worker)
                    fvm_worker_stop_requested(context->worker, &stop);
                if (stop)
                {
                    result = FVM_ERROR;
                    break;
                }
                fvm_port_thread_sleep_us(1000);
            } while (true);
        }
        else
        {
            result = register_packet(packet, channel);
            if (result == FVM_OK)
                result = fvm_channel_packet_send(channel, packet, release, retain_packet, blocking);
        }
    }
    if (file || result != FVM_OK)
        delete packet;
    if (result == FVM_ERROR)
    {
        int closed = 0;
        if (!fvm_channel_remote_address(channel))
            fvm_channel_is_closed(channel, &closed);
        fvm_resource_fault(context,
                           closed ? FVM_RESOURCE_CLOSED
                           : file ? FVM_RESOURCE_IO
                                  : FVM_RESOURCE_FAILURE,
                           fvm_channel_identity(channel), "channel.send",
                           fvm_channel_remote_address(channel));
        fvm_debug_resource_error(context, fvm_channel_identity(channel), "channel.send",
                                 fvm_channel_remote_address(channel));
    }
    return result;
}

/* Receive value. */
static FVM_Result receive_value(FVM_Context *context, FVM_Channel *channel, FVM_Value *out,
                                int blocking, bool peek, size_t index)
{

    /* Initialization / Инициализация локального состояния. */
    int file = 0;
    void *raw = 0;
    FVM_Value value;
    FVM_Result result;
    if (!context || !channel || !out || context->destroying)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    fvm_channel_is_stream(channel, &file);
    if (file)
    {
        if (peek)
            return FVM_UNSUPPORTED;
        unsigned char *data = 0;
        size_t size = 0;
        try
        {
            /* Define PathScope struct and its representation. */
            struct PathScope
            {
                FVM_Context *context;
                fvm_detail::string previous;
                /* Initialize Path Scope and its owned state. */
                PathScope(FVM_Context *c, const char *path)
                    : context(c), previous(c->resource_receive_path)
                {
                    if (path)
                        c->resource_receive_path = path;
                }
                /* Release the resources owned by this PathScope instance. */
                ~PathScope()
                {
                    context->resource_receive_path.swap(previous);
                }
            } provenance(context, fvm_channel_file_path(channel));
            result = fvm_channel_stream_receive(context, channel, &data, &size, blocking);
            if (result == FVM_OK)
                result = fvm_value_deserialize(context, data, size, out);
            fvm_serialized_free(data);
            if (result == FVM_ERROR)
            {
                int closed = 0;
                if (!fvm_channel_remote_address(channel))
                    fvm_channel_is_closed(channel, &closed);
                fvm_resource_fault(context, closed ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_IO,
                                   fvm_channel_identity(channel), "channel.receive",
                                   fvm_channel_remote_address(channel));
                fvm_debug_resource_error(context, fvm_channel_identity(channel), "channel.receive",
                                         fvm_channel_remote_address(channel));
            }
            return result;
        }
        catch (...)
        {
            fvm_serialized_free(data);
            return FVM_OUT_OF_MEMORY;
        }
    }
    result = peek ? fvm_channel_peek_mixed(channel, index, &value, &raw)
                  : fvm_channel_receive_mixed(channel, &value, &raw, blocking);
    if (result != FVM_OK)
    {
        if (result == FVM_ERROR)
        {
            int closed = 0;
            fvm_channel_is_closed(channel, &closed);
            fvm_resource_fault(context, closed ? FVM_RESOURCE_CLOSED : FVM_RESOURCE_FAILURE,
                               fvm_channel_identity(channel), "channel.receive", 0);
            fvm_debug_resource_error(context, fvm_channel_identity(channel), "channel.receive", 0);
        }
        return result;
    }
    if (raw)
    {
        Packet *packet = (Packet *)raw;
        if (!peek)
            unregister_packet(packet);
        {
            ResourceScope scope(context, packet);
            result = fvm_value_deserialize(context, packet->data, packet->size, out);
        }
        release(packet);
        return result;
    }
    try
    {
        *out = value;
        if (value.type == FVM_VALUE_STRING)
        {
            size_t length = fvm_string_size(value);
            char *copy = new (std::nothrow) char[length + 1];
            if (!copy)
                result = FVM_OUT_OF_MEMORY;
            else
            {
                std::memcpy(copy, value.string, length);
                copy[length] = 0;
                try
                {
                    context->strings.push_back(copy);
                }
                catch (...)
                {
                    delete[] copy;
                    throw;
                }
                out->string = copy;
            }
        }
    }
    catch (...)
    {
        result = FVM_OUT_OF_MEMORY;
    }
    fvm_channel_message_release(&value);
    if (result != FVM_OK)
        fvm_value_undefined(out);
    return result;
}

/* Receive channel value. */
extern "C" FVM_Result fvm_channel_receive_value(FVM_Context *c, FVM_Channel *ch, FVM_Value *out,
                                                int blocking)
{
    return receive_value(c, ch, out, blocking, false, 0);
}

/* Compute and return debug channel message. */
extern "C" FVM_Result fvm_debug_channel_message(FVM_Context *c, FVM_Channel *ch, size_t index,
                                                FVM_Value *out)
{
    return receive_value(c, ch, out, 0, true, index);
}

/* Send channel wire. */
FVM_Result fvm_channel_send_wire(FVM_Channel *channel, const unsigned char *data, size_t size)
{
    if (!data || size < 16 || size > 64UL * 1024 * 1024)
        return FVM_INVALID_ARGUMENT;
    Packet *p = new (std::nothrow) Packet;
    if (!p)
        return FVM_OUT_OF_MEMORY;
    p->data = (unsigned char *)std::malloc(size);
    if (!p->data)
    {
        delete p;
        return FVM_OUT_OF_MEMORY;
    }

    /* Computation / Вычисление и обновление состояния. */
    std::memcpy(p->data, data, size);
    p->size = size;
    FVM_Result r = register_packet(p, channel);
    if (r == FVM_OK)
        r = fvm_channel_packet_send(channel, p, release, retain_packet, 0);
    if (r != FVM_OK)
        delete p;
    return r;
}

/* Receive channel wire. */
FVM_Result fvm_channel_receive_wire(FVM_Context *c, FVM_Channel *ch, unsigned char **out,
                                    size_t *size)
{

    /* Initialization / Инициализация локального состояния. */
    void *raw = 0;
    FVM_Value value;

    /* Computation / Вычисление и обновление состояния. */
    *out = 0;
    *size = 0;
    FVM_Result r = fvm_channel_receive_mixed(ch, &value, &raw, 0);
    if (r != FVM_OK)
        return r;
    if (!raw)
    {
        r = fvm_value_serialize(c, &value, out, size);
        fvm_channel_message_release(&value);
        return r;
    }
    Packet *p = (Packet *)raw;
    unregister_packet(p);
    if (p->resources.empty())
    {
        *out = (unsigned char *)std::malloc(p->size);
        if (!*out)
            r = FVM_OUT_OF_MEMORY;
        else
        {
            std::memcpy(*out, p->data, p->size);
            *size = p->size;
        }
    }
    else
    {
        {
            ResourceScope scope(c, p);
            r = fvm_value_deserialize(c, p->data, p->size, &value);
        }
        if (r == FVM_OK)
        {
            ResourceScope scope(c, 0);
            r = fvm_value_serialize(c, &value, out, size);
        }
    }
    release(p);
    return r;
}
