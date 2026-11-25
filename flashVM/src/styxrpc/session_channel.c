/* Copyright (c) 2026 bhgv
 * EN: Send and receive mixed ActionScript values on one named UniBus port.
 * RU: Отправка и приём разных значений ActionScript через один именованный порт UniBus.
 */
#include "session_methods.h"
#include "managed_values.h"
#include "variable_ref_adapter.h"
#include "flashvm/userdata.h"
#include "styxrpc/exchange.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_SCALAR_BUFFER (3U * FVM_SRECORD_INTEGER_MAX + STYXRPC_INT_REF_SIZE) /* Numeric or ref wire scratch. */

typedef struct ScriptChannel
{
    FVM_StyxRpcSession *session;   /* Borrowed binding for an attached client, NULL for hosted. */
    StyxRpcMessagePort *port;     /* Owned fid in client mode. */
    StyxRpcExchange *exchange;    /* Owned client mailbox or borrowed hosted mailbox. */
    int closed;                   /* Explicitly retired wrapper. */
} ScriptChannel;


/* [GC:] EN: Release userdata, deferring any client clunk. Hosted mailboxes belong to the player.
 * RU: Освободить userdata, отложив clunk клиента. Серверная очередь принадлежит плееру. */
static void release_channel(void *state)
{
    ScriptChannel *channel = state;
    if (channel->session)
    {
        styxrpc_messages_abandon(channel->port);
        styxrpc_exchange_destroy(channel->exchange);
    }
    free(channel);
}


/* EN: Queue a snapshot or imported reference operand on channel. The SDK copies the bytes.
 * RU: Поставить в очередь channel снимок значения или импортированную ссылку. SDK копирует байты. */
static StyxRpcStatus queue_value(ScriptChannel *channel, const FVM_Value *value)
{
    unsigned char scratch[CHANNEL_SCALAR_BUFFER];
    const void *bytes = scratch;
    size_t size = 0;
    StyxRpcExchangeKind kind;
    const double positive_zero = 0.0;
    switch (value->type)
    {
    case FVM_VALUE_NUMBER:
        if (value->number >= INT32_MIN && value->number <= INT32_MAX &&
            (double)(int32_t)value->number == value->number &&
            (value->number != 0 || !memcmp(&value->number, &positive_zero, sizeof(positive_zero))))
        {
            kind = STYXRPC_EXCHANGE_INT;
            if (styxrpc_value_integer(STYXRPC_VALUE_INT, (int32_t)value->number, scratch,
                sizeof(scratch), &size) != STYXRPC_OK)
            {
                return STYXRPC_INVALID;
            }
        }
        else
        {
            kind = STYXRPC_EXCHANGE_REAL;
            if (styxrpc_value_real(value->number, scratch, sizeof(scratch), &size) != STYXRPC_OK)
            {
                return STYXRPC_INVALID;
            }
        }
        break;
    case FVM_VALUE_BOOLEAN:
        kind = STYXRPC_EXCHANGE_BOOL;
        scratch[0] = value->boolean != 0;
        size = 1;
        break;
    case FVM_VALUE_STRING:
        kind = STYXRPC_EXCHANGE_TEXT;
        bytes = value->string;
        size = fvm_value_string_size(value);
        break;
    case FVM_VALUE_NULL:
        kind = STYXRPC_EXCHANGE_NULL;
        break;
    case FVM_VALUE_OBJECT:
        kind = STYXRPC_EXCHANGE_REF;
        if (!channel->session || !fvm_styxrpc_encode_int_ref(channel->session, value, scratch, sizeof(scratch)))
        {
            return STYXRPC_INVALID;
        }
        size = STYXRPC_INT_REF_SIZE;
        break;
    default:
        return STYXRPC_INVALID;
    }
    return styxrpc_exchange_send(channel->exchange, kind, bytes, size);
}


/* [GC:] EN: Materialize a counted wire string as a context-owned AS string in out.
 * RU: Создать в out строку AS во владении контекста из size сетевых байт. */
static FVM_Result decode_text(FVM_Context *context, const unsigned char *bytes, size_t size, FVM_Value *out)
{
    FVM_Value borrowed;
    size_t count;
    unsigned short *units = malloc((size ? size : 1) * sizeof(*units));
    FVM_Result status;
    if (!units)
    {
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_value_string_bytes(&borrowed, (const char *)bytes, size);
    if (status == FVM_OK)
    {
        status = fvm_value_get_utf16(&borrowed, units, size, &count);
    }
    if (status == FVM_OK)
    {
        status = fvm_value_string_utf16(context, units, count, out);
    }
    free(units);
    return status;
}


/* [ASINC] [GC:] EN: Decode one retained record. A reference opens its issuer fid after the pump has ended.
 * RU: Декодировать одну удерживаемую запись. Ссылка открывает fid владельца после завершения приёма. */
static FVM_Result decode_record(FVM_Context *context, ScriptChannel *channel,
    StyxRpcExchangeKind kind, const unsigned char *bytes, size_t size, FVM_Value *out)
{
    int64_t integer;
    double real;
    switch (kind)
    {
    case STYXRPC_EXCHANGE_INT:
        if (styxrpc_value_get_integer(STYXRPC_VALUE_INT, bytes, size, &integer) == STYXRPC_OK)
        {
            fvm_value_number(out, (double)integer);
            return FVM_OK;
        }
        break;
    case STYXRPC_EXCHANGE_REAL:
        if (styxrpc_value_get_real(bytes, size, &real) == STYXRPC_OK)
        {
            fvm_value_number(out, real);
            return FVM_OK;
        }
        break;
    case STYXRPC_EXCHANGE_BOOL:
        if (size == 1 && bytes[0] <= 1)
        {
            fvm_value_boolean(out, bytes[0]);
            return FVM_OK;
        }
        break;
    case STYXRPC_EXCHANGE_NULL:
        if (!size)
        {
            fvm_value_undefined(out);
            out->type = FVM_VALUE_NULL;
            return FVM_OK;
        }
        break;
    case STYXRPC_EXCHANGE_TEXT:
        return decode_text(context, bytes, size, out);
    case STYXRPC_EXCHANGE_REF:
        if (channel->session)
        {
            return fvm_styxrpc_decode_int_ref(context, channel->session, bytes, size, out);
        }
        break;
    default:
        break;
    }
    return FVM_INVALID_ARGUMENT;
}


/* EN: Retire the wrapper and its optional client fid; deferred selects no transport I/O.
 * RU: Закрыть обёртку и её клиентский fid; deferred исключает обращения к транспорту. */
static void retire_channel(ScriptChannel *channel, int deferred)
{
    if (channel->port)
    {
        if (deferred)
        {
            styxrpc_messages_abandon(channel->port);
        }
        else
        {
            styxrpc_messages_close(channel->port);
        }
        channel->port = NULL;
        styxrpc_exchange_attach(channel->exchange, NULL);
    }
    channel->closed = 1;
}


/* EN: Check that the wrapper, and its host binding in client mode, are live.
 * RU: Проверить, жива ли обёртка и её привязка в клиентском режиме. */
static int channel_live(const ScriptChannel *channel)
{
    return !channel->closed && (!channel->session || channel->session->client);
}


/* [ASINC] EN: Submit args[0] to the peer on this channel. True confirms the Styx write.
 * RU: Отправить args[0] другой стороне канала. True подтверждает запись Styx. */
static FVM_Result send_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    StyxRpcStatus status;
    (void)self;
    if (!channel_live(channel) || argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = queue_value(channel, args);
    if (status == STYXRPC_INVALID)
    {
        return FVM_INVALID_ARGUMENT;
    }
    if (status == STYXRPC_OK && channel->port && styxrpc_messages_flush(channel->port) != STYXRPC_OK)
    {
        retire_channel(channel, 1);
        return fvm_styxrpc_managed_fail(context, "Styx RPC channel write failed");
    }
    fvm_value_boolean(out, status == STYXRPC_OK);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Return one FIFO value, or undefined if one bounded transport poll yields none.
 * RU: Вернуть одно значение FIFO или undefined, если один ограниченный опрос ничего не дал. */
static FVM_Result recv_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    unsigned char bytes[STYXRPC_MESSAGE_PAYLOAD_MAX];
    StyxRpcExchangeKind kind;
    size_t size;
    StyxRpcStatus status;
    (void)self;
    (void)args;
    if (!channel_live(channel) || argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_exchange_recv(channel->exchange, &kind, bytes, sizeof(bytes), &size);
    if (status == STYXRPC_MORE && channel->port)
    {
        if (styxrpc_messages_pump(channel->port) == STYXRPC_INVALID)
        {
            retire_channel(channel, 1);
            return fvm_styxrpc_managed_fail(context, "Styx RPC channel read failed");
        }
        status = styxrpc_exchange_recv(channel->exchange, &kind, bytes, sizeof(bytes), &size);
    }
    if (status == STYXRPC_MORE)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return status == STYXRPC_OK ? decode_record(context, channel, kind, bytes, size, out) : FVM_INVALID_ARGUMENT;
}


/* EN: Distinguish a buffered value (including null/false) from an idle queue. No I/O.
 * RU: Отличить значение в очереди (включая null/false) от отсутствия записи. Без I/O. */
static FVM_Result channel_ready(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc || !channel_live(channel))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, styxrpc_exchange_pending(channel->exchange));
    return FVM_OK;
}


/* [ASINC] EN: Close the fid and release buffered records. Repeated close succeeds.
 * RU: Закрыть fid и освободить накопленные записи. Повторное закрытие успешно. */
static FVM_Result close_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    ScriptChannel *channel = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    retire_channel(channel, channel->session && !channel->session->client);
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [ASINC] [GC:] EN: Construct an AS wrapper around a client fid or the provided hosted mailbox.
 * RU: Создать обёртку AS для клиентского fid либо серверной очереди. */
static FVM_Result create_channel(FVM_Context *context, FVM_StyxRpcSession *binding,
    const char *name, StyxRpcExchange *hosted, FVM_Value *out)
{
    FVM_GCRootScope *roots;
    FVM_Result status;
    FVM_Value object;
    ScriptChannel *channel = NULL;
    status = fvm_gc_roots_begin(context, &roots);
    if (status != FVM_OK)
    {
        return status;
    }
    fvm_value_undefined(&object);
    status = fvm_object_create(context, &object.object);
    if (status == FVM_OK)
    {
        object.type = FVM_VALUE_OBJECT;
        status = fvm_gc_roots_add(roots, &object);
    }
    if (status == FVM_OK)
    {
        channel = calloc(1, sizeof(*channel));
        status = channel ? fvm_userdata_attach(context, object.object, channel, release_channel) : FVM_OUT_OF_MEMORY;
        if (status != FVM_OK)
        {
            free(channel);
        }
    }
    if (status == FVM_OK)
    {
        channel->session = binding;
        channel->exchange = hosted;
        status = fvm_object_define_method_rooted(context, object.object, "send", send_value, channel, object.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "recv", recv_value, channel, object.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "close", close_channel, channel, object.object);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object.object, "ready", channel_ready, channel, object.object);
    }
    if (status == FVM_OK && binding)
    {
        StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1];
        StyxRpcMessageSpec spec;
        channel->exchange = styxrpc_exchange_create();
        if (!channel->exchange)
        {
            status = FVM_OUT_OF_MEMORY;
        }
        else
        {
            styxrpc_exchange_spec(channel->exchange, 0, types, &spec);
            if (styxrpc_messages_open_at(binding->client, name, &spec, &channel->port) != STYXRPC_OK)
            {
                status = FVM_NOT_FOUND;
            }
            else
            {
                styxrpc_exchange_attach(channel->exchange, channel->port);
            }
        }
    }
    if (status == FVM_OK)
    {
        *out = object;
    }
    fvm_gc_roots_end(roots);
    return status;
}


/* [ASINC] [GC:] EN: Open a named peer exchange on the existing session.
 * RU: Открыть именованный корень другой стороны в существующей сессии. */
static FVM_Result new_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    FVM_StyxRpcSession *binding = state;
    char address[STYXRPC_TREE_NAME + 2];
    const char *name;
    (void)self;
    if (!binding->client || argc != 1 || !fvm_styxrpc_name(args, address, sizeof(address)))
    {
        return FVM_INVALID_ARGUMENT;
    }
    name = address + (address[0] == '/');
    if (!*name || strchr(name, '/') || strlen(name) > STYXRPC_TREE_NAME)
    {
        return FVM_INVALID_ARGUMENT;
    }
    return create_channel(context, binding, name, NULL, out);
}


/* [GC:] EN: Open the standard local application mailbox published by the player.
 * RU: Открыть локальную очередь приложения, опубликованную плеером. */
static FVM_Result listen_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    char address[sizeof(STYXRPC_EXCHANGE_DEFAULT) + 1];
    (void)self;
    if (argc != 1 || !fvm_styxrpc_name(args, address, sizeof(address)) ||
        strcmp(address + (address[0] == '/'), STYXRPC_EXCHANGE_DEFAULT))
    {
        return FVM_INVALID_ARGUMENT;
    }
    return create_channel(context, NULL, NULL, state, out);
}


/* [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_host_exchange(FVM_Context *context, StyxRpcExchange *exchange)
{
    FVM_Value rpc;
    FVM_Result status = fvm_get_global(context, "rpc", &rpc);
    if (status != FVM_OK || rpc.type != FVM_VALUE_OBJECT)
    {
        return status == FVM_OK ? FVM_NOT_FOUND : status;
    }
    return fvm_object_define_method_rooted(context, rpc.object, "listen", listen_channel, exchange, NULL);
}


/* [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_channel_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    return fvm_object_define_method_rooted(context, object, "newchan", new_channel, binding, NULL);
}
