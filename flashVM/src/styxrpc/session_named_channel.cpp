/* Copyright (c) 2026 bhgv
 * EN: Expose one leased blocking StyxRPC channel as an ActionScript object.
 * RU: Представить блокирующий канал StyxRPC объектом ActionScript.
 */
#include "session_named_values.hpp"
#include "flashvm/userdata.h"
#include "styxrpc/named_channel.h"
#include <stdlib.h>

extern "C"
{
#include "session_methods.h"
#include "managed_values.h"
#include "movie_load.h"
}

namespace
{
struct NamedHandle
{
    FVM_NamedRoute route;       /* Borrowed path binding and issuer until context destruction. */
    StyxRpcVfsFile *file;        /* Owned channel enrollment, NULL after delete. */
};


/* EN: Consume the fid once, without protocol I/O during collection.
 * RU: Освободить fid один раз, без протокольного I/O во время сборки мусора. */
void close_handle(NamedHandle *handle, int deferred)
{
    styxrpc_vfs_close(handle->file, deferred);
    handle->file = NULL;
}


/* [GC:] EN: Drop an unreachable wrapper's fid and native state.
 * RU: Снять fid недостижимой обёртки и освободить её нативное состояние. */
void release(void *state)
{
    NamedHandle *handle = static_cast<NamedHandle *>(state);
    close_handle(handle, 1);
    free(handle);
}


/* EN: Retire this enrollment. Repeated delete is harmless.
 * RU: Снять участие этого объекта. Повторный delete допустим. */
FVM_Result delete_channel(FVM_Context *, FVM_Object *, unsigned argc,
    const FVM_Value *, FVM_Value *out, void *state)
{
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    close_handle(static_cast<NamedHandle *>(state), 0);
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Wait for a read from another participant, sending a copy or live descriptor.
 * RU: Дождаться чтения другим участником, посылая копию либо живой дескриптор. */
FVM_Result send_channel(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    NamedHandle *handle = static_cast<NamedHandle *>(state);
    if (!handle->file || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned char *frame;
    size_t size;
    FVM_Result encoded = fvm_named_encode(context, handle->route, args, &frame, &size);
    if (encoded != FVM_OK)
    {
        return encoded;
    }
    StyxRpcIoOutcome outcome = styxrpc_named_send(handle->file, frame, size);
    free(frame);
    if (outcome != STYXRPC_IO_OK)
    {
        close_handle(handle, 0);
        return fvm_styxrpc_managed_fail(context, "named channel send failed");
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Wait for a matching write and bind a descriptor or unpack its graph.
 * RU: Дождаться парной записи и связать дескриптор либо восстановить граф. */
FVM_Result recv_channel(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *, FVM_Value *out, void *state)
{
    NamedHandle *handle = static_cast<NamedHandle *>(state);
    if (!handle->file || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned char *frame;
    size_t size;
    if (styxrpc_named_recv(handle->file, &frame, &size) != STYXRPC_IO_OK)
    {
        close_handle(handle, 0);
        return fvm_styxrpc_managed_fail(context, "named channel receive failed");
    }
    FVM_Result result = fvm_named_decode(context, handle->route, frame, size, out);
    free(frame);
    return result;
}


/* [GC:] EN: Transfer the opened fid to a rooted wrapper, also retained by extracted methods.
 * RU: Передать открытый fid удерживаемой обёртке, доступной из извлечённых методов. */
FVM_Result wrap(FVM_Context *context, NamedHandle *handle, FVM_Value *out)
{
    static const char *const names[] = {"recv", "send", "delete"};
    static const FVM_NativeFunction calls[] = {recv_channel, send_channel, delete_channel};
    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        release(handle);
        return status;
    }
    FVM_Value object;
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    bool managed = false;
    if (status == FVM_OK)
    {
        status = fvm_userdata_attach(context, object.object, handle, release);
        managed = status == FVM_OK;
    }
    for (size_t i = 0; i < sizeof(names) / sizeof(*names) && status == FVM_OK; ++i)
    {
        status = fvm_object_define_method_rooted(context, object.object, names[i], calls[i], handle, object.object);
    }
    if (status == FVM_OK)
    {
        *out = object;
    }
    else if (managed)
    {
        close_handle(handle, 0);
    }
    else
    {
        release(handle);
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [ASINC] [GC:] EN: Resolve args using the current cwd, then enroll on route.client.
 * RU: Разрешить args относительно текущего cwd и подключиться к каналу на route.client. */
FVM_Result open_channel(FVM_Context *context, const FVM_NamedRoute &route, unsigned argc,
    const FVM_Value *args, FVM_Value *out)
{
    char path[FVM_STYXRPC_ABSOLUTE_PATH];
    if (!route.client || argc != 1 || !fvm_styxrpc_resolve_path(route.vfs, args, path) || !styxrpc_named_path(path))
    {
        return FVM_INVALID_ARGUMENT;
    }
    NamedHandle *handle = static_cast<NamedHandle *>(calloc(1, sizeof(*handle)));
    if (!handle)
    {
        return FVM_OUT_OF_MEMORY;
    }
    handle->route = route;
    handle->file = styxrpc_named_open(route.client, path);
    if (!handle->file)
    {
        free(handle);
        return fvm_styxrpc_managed_fail(context, "named channel could not be opened");
    }
    return wrap(context, handle, out);
}
}


/* [ASINC] [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
extern "C" FVM_Result fvm_styxrpc_open_named_channel(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *session = static_cast<FVM_StyxRpcSession *>(state);
    const FVM_NamedRoute route = {session, session->client, NULL};
    return open_channel(context, route, argc, args, out);
}


/* [ASINC] [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
extern "C" FVM_Result fvm_styxrpc_open_peer_channel(FVM_Context *context, FVM_Object *, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcMovies *movies = static_cast<FVM_StyxRpcMovies *>(state);
    const FVM_NamedRoute route = {movies->binding, movies->named, movies->refs};
    return open_channel(context, route, argc, args, out);
}
