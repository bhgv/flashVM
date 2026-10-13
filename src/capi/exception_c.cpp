/* FlashVM: C embedding API: exception.
 * File: src/capi/exception_c.cpp.
 */

#include "../core/string_internal.hpp"
#include "../standard/error_state.hpp"
#include "../core/exception_internal.hpp"
#include "../core/capability_internal.hpp"
#include "flashvm/mutex.h"
#include "../../ports/channel_sync.h"
#include <cstring>
#include <new>
namespace
{
FVM_ExceptionInfo events[64];
size_t event_count = 0;

/* Copy the current value. */
void copy(char *dst, size_t size, const char *src)
{
    std::memset(dst, 0, size);
    if (src)
        std::strncpy(dst, src, size - 1);
}

/* Compute and return message. */
const char *message(unsigned long code)
{
    switch (code)
    {
    case FVM_RESOURCE_CLOSED:
        return "resource.closed: Resource is closed.";
    case FVM_RESOURCE_STALE:
        return "resource.stale: Resource descriptor is no longer valid.";
    case FVM_RESOURCE_DISCONNECTED:
        return "resource.disconnected: Remote connection ended or timed out.";
    case FVM_RESOURCE_UNREACHABLE:
        return "resource.unreachable: Cannot connect to resource authority.";
    case FVM_RESOURCE_NOT_OWNER:
        return "resource.not_owner: Mutex belongs to another context.";
    case FVM_RESOURCE_RECURSIVE:
        return "resource.recursive: Mutex acquisition is not recursive.";
    case FVM_RESOURCE_CANCELLED:
        return "resource.cancelled: Resource wait was cancelled.";
    case FVM_RESOURCE_IO:
        return "resource.io: File input or output failed.";
    default:
        return "resource.failure: Resource operation failed.";
    }
}

/* Implement record for this subsystem. */
void record(const FVM_ExceptionInfo &info)
{
    fvm_port_registry_lock();
    if (event_count == 64)
    {
        std::memmove(events, events + 1, 63 * sizeof(events[0]));
        --event_count;
    }

    /* Computation / Вычисление и обновление состояния. */
    events[event_count++] = info;
    fvm_port_registry_unlock();
}

/* Compute and return number. */
FVM_Result number(FVM_Context *c, FVM_Object *o, const char *key, unsigned long n)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_number(&v, (double)n);
    return fvm_object_set(c, o, key, &v);
}

/* Initialize string and its owned state. */
FVM_Result string(FVM_Context *c, FVM_Object *o, const char *key, const char *text)
{

    /* Initialization / Инициализация локального состояния. */
    FVM_Value v;
    fvm_value_string(&v, text);
    return fvm_object_set(c, o, key, &v);
}
} // namespace
/* Implement resource fault begin for this subsystem. */
void fvm_resource_fault_begin(FVM_Context *c)
{
    /* Preserve an explicit pending throw; fresh calls cannot reuse old raw faults. */
    if (c && !c->exception_pending)
        c->fault_pending = false;
}

/* Implement resource fault for this subsystem. */
void fvm_resource_fault(FVM_Context *c, unsigned long code, unsigned long id, const char *op,
                        const char *address)
{
    if (!c || c->destroying || c->exception_pending || c->fault_pending)
        return;
    FVM_ExceptionInfo &info = c->fault;

    /* Computation / Вычисление и обновление состояния. */
    std::memset(&info, 0, sizeof(info));
    info.code = code;
    info.resource = id;
    info.thread = c->identity;
    info.line = c->debug_line;
    info.offset = c->debug_offset;
    copy(info.message, sizeof(info.message), message(code));
    copy(info.operation, sizeof(info.operation), op);
    copy(info.address, sizeof(info.address), address);
    copy(info.path, sizeof(info.path), c->resource_receive_path.c_str());
    if (info.address[0])
        info.flags |= FVM_EXCEPTION_NETWORK;
    if (info.path[0])
        info.flags |= FVM_EXCEPTION_FILE;

    /* Computation / Вычисление и обновление состояния. */
    copy(info.source, sizeof(info.source), c->debug_source.c_str());
    copy(info.function, sizeof(info.function), c->debug_function.c_str());
    c->fault_pending = true;
}

/* Compute and return exception raise. */
extern "C" FVM_Result fvm_exception_raise(FVM_Context *c, const FVM_Value *value)
{
    if (!c || c->destroying || fvm_validate_value(c, value) != FVM_OK)
        return FVM_INVALID_ARGUMENT;
    FVM_Value owned = *value;
    if (value->type == FVM_VALUE_STRING)
    {
        size_t size = fvm_string_size(*value) + 1;
        char *text = new (std::nothrow) char[size];
        if (!text)
            return FVM_OUT_OF_MEMORY;
        std::memcpy(text, value->string, size - 1);
        text[size - 1] = 0;
        try
        {
            /* [GC:] Register owned bytes and their counted length together; failed insertion leaves
             * caller cleanup active. */
            fvm_context_adopt_string(c, text, size - 1);
        }
        catch (...)
        {
            delete[] text;
            return FVM_OUT_OF_MEMORY;
        }
        owned.string = text;
    }

    /* Computation / Вычисление и обновление состояния. */
    c->exception_value = owned;
    c->exception_pending = true;
    c->fault_pending = false;
    return FVM_EXCEPTION;
}

/* Read exception. */
extern "C" FVM_Result fvm_exception_get(FVM_Context *c, FVM_Value *out)
{
    if (!c || !out || c->destroying)
        return FVM_INVALID_ARGUMENT;
    fvm_value_undefined(out);
    if (c->exception_pending)
    {
        *out = c->exception_value;
        return FVM_OK;
    }
    if (!c->fault_pending)
        return FVM_NOT_FOUND;
    try
    {
        FVM_Object *error = 0, *context = 0;
        FVM_Result r = fvm_object_create(c, &error);
        if (r != FVM_OK)
            return r;
        /* Root the first allocation before creating/filling the context object. */
        fvm_value_undefined(&c->exception_value);
        c->exception_value.type = FVM_VALUE_OBJECT;
        c->exception_value.object = error;
        c->exception_pending = true;
        r = fvm_object_create(c, &context);
        FVM_Value v;
        fvm_value_undefined(&v);
        v.type = FVM_VALUE_OBJECT;
        v.object = context;
        if (r == FVM_OK)
            r = fvm_object_set(c, error, "context", &v);
        const FVM_ExceptionInfo &f = c->fault;
#define PUT(call)                                                                                  \
    if (r == FVM_OK)                                                                               \
    r = (call)
        PUT(number(c, error, "code", f.code));
        PUT(string(c, error, "message", f.message));
        PUT(string(c, error, "name", "ResourceException"));
        PUT(number(c, context, "resource", f.resource));
        PUT(number(c, context, "threadId", f.thread));
        PUT(number(c, context, "originThreadId", f.origin_thread));
        PUT(number(c, context, "flags", f.flags));
        PUT(number(c, context, "line", f.line));
        PUT(number(c, context, "offset", f.offset));
        PUT(string(c, context, "operation", f.operation));
        PUT(string(c, context, "resourceKind", f.kind));
        PUT(string(c, context, "path", f.path));
        PUT(string(c, context, "address", f.address));
        PUT(string(c, context, "source", f.source));
        PUT(string(c, context, "function", f.function));
        PUT(string(c, context, "transport",
                   f.flags & FVM_EXCEPTION_FILE      ? "file"
                   : f.flags & FVM_EXCEPTION_NETWORK ? "network"
                   : f.flags & FVM_EXCEPTION_THREAD  ? "thread"
                                                     : "local"));
#undef PUT
        if (r != FVM_OK)
        {
            c->exception_pending = false;
            fvm_value_undefined(&c->exception_value);
            return r;
        }
        try
        {
            fvm_error_attach(c, error);
        }
        catch (...)
        {
            c->exception_pending = false;
            fvm_value_undefined(&c->exception_value);
            return FVM_OUT_OF_MEMORY;
        }
        record(f);
        *out = c->exception_value;
        return FVM_OK;
    }
    catch (...)
    {
        /* Keep the raw fault available for a retry after allocation failure. */
        c->exception_pending = false;
        fvm_value_undefined(&c->exception_value);
        return FVM_OUT_OF_MEMORY;
    }
}

/* Compute and return exception info. */
extern "C" FVM_Result fvm_exception_info(FVM_Context *c, FVM_ExceptionInfo *out)
{
    if (!c || !out)
        return FVM_INVALID_ARGUMENT;
    if (!c->fault_pending)
        return FVM_NOT_FOUND;
    *out = c->fault;
    return FVM_OK;
}

/* Clear exception. */
extern "C" FVM_Result fvm_exception_clear(FVM_Context *c)
{
    if (!c)
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    c->exception_pending = false;
    c->fault_pending = false;
    fvm_value_undefined(&c->exception_value);
    return FVM_OK;
}

/* Implement resource fault resource for this subsystem. */
void fvm_resource_fault_resource(FVM_Context *c, unsigned long kind, void *handle,
                                 unsigned long origin)
{
    if (!c || !c->fault_pending || c->exception_pending || c->fault.kind[0])
        return;
    FVM_ExceptionInfo &f = c->fault;
    copy(f.kind, sizeof(f.kind), kind == 8 ? "mutex" : "channel");
    if (kind == 8 && handle)
    {
        f.resource = fvm_mutex_identity((FVM_Mutex *)handle);
        f.origin_thread = fvm_mutex_origin((FVM_Mutex *)handle);
    }
    else
    {
        if (handle)
            f.resource = fvm_channel_identity((FVM_Channel *)handle);
        f.origin_thread = origin;
        /* Preserve a carrier path if present; otherwise identify the failed native file resource.
         */
        const char *path = handle ? fvm_channel_file_path((FVM_Channel *)handle) : 0;
        if (path && *path && !f.path[0])
        {
            copy(f.path, sizeof(f.path), path);
            f.flags |= FVM_EXCEPTION_FILE;
        }
    }
    if (f.origin_thread && f.origin_thread != c->identity)
        f.flags |= FVM_EXCEPTION_THREAD;
}

/* Compute and return exception complete. */
FVM_Result fvm_exception_complete(FVM_Context *c, FVM_Object *self, FVM_Result result)
{
    if (result != FVM_ERROR && result != FVM_EXCEPTION)
        return result;
    if (!c->fault_pending && !c->exception_pending)
        return result;
    if (self && self->resource && (self->resource_kind == 1 || self->resource_kind == 8) &&
        c->fault_pending && !c->exception_pending)
    {
        FVM_ExceptionInfo &f = c->fault;
        fvm_resource_fault_resource(c, self->resource_kind, self->resource, self->resource_origin);
        if (!self->resource_path.empty())
        {
            copy(f.path, sizeof(f.path), self->resource_path.c_str());
            f.flags |= FVM_EXCEPTION_FILE;
        }
    }

    /* Initialization / Инициализация локального состояния. */
    FVM_Value thrown;
    FVM_Result r = fvm_exception_get(c, &thrown);
    return r == FVM_OK ? FVM_EXCEPTION : r;
}

/* Compute and return debug exception events. */
extern "C" FVM_Result fvm_debug_exception_events(FVM_ExceptionInfo *out, size_t capacity,
                                                 size_t *count, int clear)
{
    if (!count || (!out && capacity))
        return FVM_INVALID_ARGUMENT;

    /* Computation / Вычисление и обновление состояния. */
    fvm_port_registry_lock();
    *count = event_count;
    if (capacity < event_count)
    {
        fvm_port_registry_unlock();
        return FVM_NOT_FOUND;
    }
    if (event_count)
        std::memcpy(out, events, event_count * sizeof(events[0]));
    if (clear)
        event_count = 0;
    fvm_port_registry_unlock();
    return FVM_OK;
}
