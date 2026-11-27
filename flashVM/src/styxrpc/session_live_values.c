/* Copyright (c) 2026 bhgv
 * [GC:] EN: Expose exact typed live variable proxies in the owner of a mounted session.
 * RU: Предоставить точные прокси живых переменных владельцу смонтированной сессии.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "wire_wait.h"
#include "flashvm/userdata.h"
#include "styxrpc/live_client.h"
#include <stdlib.h>
#include <string.h>


typedef struct LiveBinding
{
    StyxRpcLive *live; /* Owned exact binding, NULL after explicit close. */
} LiveBinding;

typedef struct LiveWait
{
    FVM_Worker *worker; /* Borrowed current owner, if it supports stop. */
    StyxRpcVfsStop stop; /* Optional callback sampled by the SDK. */
} LiveWait;


/* EN: Copy the current worker's optional stop hook for one native operation.
 * RU: Скопировать флаг остановки текущего задания для одной нативной операции. */
static LiveWait caller(FVM_Context *context)
{
    LiveWait wait = {NULL, NULL};
    fvm_worker_current(context, &wait.worker);
    wait.stop = wait.worker ? fvm_styxrpc_worker_stopped : NULL;
    return wait;
}


/* EN: Report a live I/O failure on the owning context with its outcome intact.
 * RU: Сообщить владельцу об ошибке обращения с сохранением исхода. */
static FVM_Result failure(FVM_Context *context, StyxRpcIoOutcome status)
{
    const char *message = status == STYXRPC_IO_UNCERTAIN ? "Styx live variable update outcome is uncertain" :
        status == STYXRPC_IO_CANCELLED ? "Styx live variable operation cancelled" :
        status == STYXRPC_IO_NOT_OPEN ? "Styx live variable is missing, stale or unavailable" :
        "Styx live variable operation failed";
    return fvm_styxrpc_managed_fail(context, message);
}


/* [GC:] EN: Release the native binding without blocking the collector.
 * RU: Освободить нативную привязку без блокировки сборщика. */
static void retire(void *state)
{
    LiveBinding *binding = state;
    styxrpc_live_close(binding->live, 1);
    free(binding);
}


/* EN: Validate an indexed argument, or use index zero for a scalar.
 * RU: Проверить аргумент-индекс либо использовать ноль для скаляра. */
static int position(StyxRpcLiveKind kind, unsigned argc, const FVM_Value *args, int64_t *index)
{
    if (kind == STYXRPC_LIVE_BIGS)
    {
        return argc == 1 && fvm_styxrpc_read_big(args, index);
    }
    if (kind == STYXRPC_LIVE_INTS)
    {
        uint64_t value;
        if (argc == 1 && fvm_styxrpc_number(args, UINT32_MAX, &value))
        {
            *index = (int64_t)value;
            return 1;
        }
        return 0;
    }
    *index = 0;
    return argc == 0 && kind != STYXRPC_LIVE_ROW;
}


/* [ASINC] [GC:] EN: Decode the current scalar or selected element into an owned AS value.
 * RU: Декодировать текущий скаляр либо выбранный элемент в собственное значение AS. */
static FVM_Result get_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    LiveBinding *binding = state;
    StyxRpcLiveValue value;
    StyxRpcLiveKind kind;
    LiveWait wait = caller(context);
    int64_t index;
    StyxRpcIoOutcome status;
    (void)self;
    if (!binding->live)
    {
        return failure(context, STYXRPC_IO_NOT_OPEN);
    }
    kind = styxrpc_live_kind(binding->live);
    if (!position(kind, argc, args, &index))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_live_read_until(binding->live, index, &value, wait.stop, wait.worker);
    if (status != STYXRPC_IO_OK)
    {
        return failure(context, status);
    }
    if (kind == STYXRPC_LIVE_TEXT)
    {
        if (value.nil)
        {
            fvm_value_undefined(out);
            out->type = FVM_VALUE_NULL;
            return FVM_OK;
        }
        return fvm_styxrpc_return_text(context, value.text, value.size, out);
    }
    return fvm_styxrpc_return_big(context, value.integer, out);
}


/* EN: Read a complete borrowed AS replacement into the SDK's native value.
 * RU: Прочитать полное заимствованное значение AS в нативное значение SDK. */
static int replacement(StyxRpcLiveKind kind, const FVM_Value *arg, StyxRpcLiveValue *value)
{
    if (kind == STYXRPC_LIVE_TEXT)
    {
        if (arg->type == FVM_VALUE_NULL)
        {
            value->nil = 1;
            return 1;
        }
        if (arg->type != FVM_VALUE_STRING || fvm_value_string_size(arg) > sizeof(value->text))
        {
            return 0;
        }
        value->size = fvm_value_string_size(arg);
        memcpy(value->text, arg->string, value->size);
        return 1;
    }
    if (kind == STYXRPC_LIVE_BIGS || kind == STYXRPC_LIVE_BIG)
    {
        return fvm_styxrpc_read_big(arg, &value->integer);
    }
    return arg->type == FVM_VALUE_NUMBER && fvm_styxrpc_read_big(arg, &value->integer) &&
        value->integer >= INT32_MIN && value->integer <= INT32_MAX;
}


/* [ASINC] EN: Replace one admitted cell; return true only on full acknowledgement.
 * RU: Заменить допущенную ячейку; вернуть true только после полного подтверждения. */
static FVM_Result set_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    LiveBinding *binding = state;
    StyxRpcLiveValue value = {0};
    StyxRpcLiveKind kind;
    LiveWait wait = caller(context);
    int64_t index;
    StyxRpcIoOutcome status;
    (void)self;
    if (!binding->live)
    {
        return failure(context, STYXRPC_IO_NOT_OPEN);
    }
    kind = styxrpc_live_kind(binding->live);
    if (!argc || !position(kind, argc - 1, args, &index) || !replacement(kind, args + argc - 1, &value))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_live_write_until(binding->live, index, &value, wait.stop, wait.worker);
    if (status != STYXRPC_IO_OK)
    {
        return failure(context, status);
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


/* [ASINC] EN: Query the current collection length or fixed count of named fields.
 * RU: Получить текущую длину коллекции либо число именованных полей. */
static FVM_Result get_length(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    LiveBinding *binding = state;
    LiveWait wait = caller(context);
    uint32_t length;
    StyxRpcIoOutcome status;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_live_length_until(binding->live, &length, wait.stop, wait.worker);
    if (status != STYXRPC_IO_OK)
    {
        return failure(context, status);
    }
    fvm_value_number(out, length);
    return FVM_OK;
}


/* EN: Close the exact binding once. All aliases and extracted methods see the same state.
 * RU: Закрыть точную привязку один раз. Алиасы и извлечённые методы видят одно состояние. */
static FVM_Result close_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    LiveBinding *binding = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, binding->live != NULL);
    styxrpc_live_close(binding->live, 0);
    binding->live = NULL;
    return FVM_OK;
}


/* [GC:] EN: Consume an exact live handle and publish one rooted script proxy.
 * RU: Поглотить точный живой дескриптор и опубликовать удерживаемый прокси. */
static FVM_Result wrap(FVM_Context *context, StyxRpcLive *live, FVM_Value *out);


/* [ASINC] [GC:] EN: Open a named descendant with independently owned exact identity.
 * RU: Открыть именованного потомка с независимым владением точной идентичностью. */
static FVM_Result open_field(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    LiveBinding *binding = state;
    LiveWait wait = caller(context);
    StyxRpcLive *child = NULL;
    char name[STYXRPC_TREE_NAME + 1];
    StyxRpcIoOutcome status;
    (void)self;
    if (argc != 1 || !fvm_styxrpc_name(args, name, sizeof(name)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_live_field_until(binding->live, name, &child, wait.stop, wait.worker);
    return status == STYXRPC_IO_OK ? wrap(context, child, out) : failure(context, status);
}


/* [GC:] EN/RU: See wrap declaration. / См. объявление wrap. */
static FVM_Result wrap(FVM_Context *context, StyxRpcLive *live, FVM_Value *out)
{
    static const struct
    {
        const char *name; /* Method exposed on the script proxy. */
        FVM_NativeFunction method; /* Borrowed owner-local implementation. */
    } methods[] = {
        {"get", get_value}, {"set", set_value}, {"length", get_length}, {"field", open_field}, {"close", close_value}
    }; /* Each method roots the whole proxy, including extracted callbacks. */
    FVM_GCRootScope *roots;
    FVM_Result status = fvm_gc_roots_begin(context, &roots);
    if (status == FVM_OK)
    {
        FVM_Value proxy;
        fvm_value_undefined(&proxy);
        status = fvm_object_create(context, &proxy.object);
        if (status == FVM_OK)
        {
            proxy.type = FVM_VALUE_OBJECT;
            status = fvm_gc_roots_add(roots, &proxy);
        }
        if (status == FVM_OK)
        {
            LiveBinding *binding = calloc(1, sizeof(*binding));
            status = binding ? fvm_userdata_attach(context, proxy.object, binding, retire) : FVM_OUT_OF_MEMORY;
            if (status == FVM_OK)
            {
                size_t i;
                binding->live = live;
                live = NULL;
                for (i = 0; status == FVM_OK && i < sizeof(methods) / sizeof(*methods); ++i)
                {
                    status = fvm_object_define_method_rooted(context, proxy.object, methods[i].name,
                        methods[i].method, binding, proxy.object);
                }
                if (status == FVM_OK)
                {
                    *out = proxy;
                }
                else
                {
                    styxrpc_live_close(binding->live, 0);
                    binding->live = NULL;
                }
            }
            else
            {
                free(binding);
            }
        }
        fvm_gc_roots_end(roots);
    }
    styxrpc_live_close(live, 0);
    return status;
}


/* [ASINC] [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_open_live_variable(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    LiveWait wait = caller(context);
    StyxRpcLive *live = NULL;
    char path[STYXRPC_VFS_PATH_MAX + 2];
    StyxRpcIoOutcome status;
    (void)self;
    if (!binding->client || argc < 1 || argc > 2 || (argc == 2 && args[1].type != FVM_VALUE_BOOLEAN) ||
        !fvm_styxrpc_name(args, path, sizeof(path)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_live_open_until(binding->client, path + (path[0] == '/'),
        argc == 2 && args[1].boolean, &live, wait.stop, wait.worker);
    return status == STYXRPC_IO_OK ? wrap(context, live, out) : failure(context, status);
}
