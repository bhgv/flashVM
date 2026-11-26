/* Copyright (c) 2026 bhgv
 * EN: Send and receive mixed ActionScript values on the paired player's mailbox.
 * RU: Отправка и приём разных значений ActionScript через очередь парного плеера.
 */
#include "session_methods.h"
#include "styxrpc/managed.h"
#include "flashvm/execution.h"
#include "styxrpc/exchange.h"
#include <stdlib.h>
#include <string.h>

#define CHANNEL_SCALAR_BUFFER (3U * FVM_SRECORD_INTEGER_MAX + 9U) /* Buffer for one managed numeric record. */

/* EN: Queue a snapshot on the default exchange. The SDK copies the bytes.
 * RU: Поставить в очередь channel снимок значения. SDK копирует байты. */
static StyxRpcStatus queue_value(StyxRpcExchange *exchange, const FVM_Value *value)
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
    default:
        return STYXRPC_INVALID;
    }
    return styxrpc_exchange_send(exchange, kind, bytes, size);
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


/* [GC:] EN: Decode one retained value from the default mailbox.
 * RU: Декодировать значение из штатной очереди. */
static FVM_Result decode_record(FVM_Context *context,
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
    default:
        break;
    }
    return FVM_INVALID_ARGUMENT;
}


/* EN: Queue the value for the peer of the default exchange.
 * RU: Поставить значение в очередь для другой стороны штатного обмена. */
static FVM_Result send_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    StyxRpcExchange *exchange = state;
    StyxRpcStatus status;
    (void)self;
    (void)context;
    if (argc != 1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = queue_value(exchange, args);
    if (status == STYXRPC_INVALID)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, status == STYXRPC_OK);
    return FVM_OK;
}


/* [GC:] EN: Return the next buffered value, or undefined while the mailbox is empty.
 * RU: Вернуть следующее значение в очереди либо undefined, пока она пуста. */
static FVM_Result recv_value(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    StyxRpcExchange *exchange = state;
    unsigned char bytes[STYXRPC_MESSAGE_PAYLOAD_MAX];
    StyxRpcExchangeKind kind;
    size_t size;
    StyxRpcStatus status;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    status = styxrpc_exchange_recv(exchange, &kind, bytes, sizeof(bytes), &size);
    if (status == STYXRPC_MORE)
    {
        fvm_value_undefined(out);
        return FVM_OK;
    }
    return status == STYXRPC_OK ? decode_record(context, kind, bytes, size, out) : FVM_INVALID_ARGUMENT;
}


/* EN: Distinguish a buffered value (including null/false) from an idle queue. No I/O.
 * RU: Отличить значение в очереди (включая null/false) от отсутствия записи. Без I/O. */
static FVM_Result channel_ready(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    StyxRpcExchange *exchange = state;
    (void)context;
    (void)self;
    (void)args;
    if (argc)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_boolean(out, styxrpc_exchange_pending(exchange));
    return FVM_OK;
}


/* EN: Accept a counted address spelling without interpreting the pending dial grammar.
 * RU: Проверить строку адреса без разбора ещё не определённой грамматики. */
static int valid_address(const FVM_Value *value)
{
    size_t size = value->type == FVM_VALUE_STRING ? fvm_value_string_size(value) : 0;
    return size && !memchr(value->string, 0, size);
}


/* EN: Reserve the address-based constructor. Additional connections are pending.
 * RU: Зарезервировать создание по адресу. Дополнительные соединения пока не открываются. */
static FVM_Result new_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)context;
    (void)self;
    (void)state;
    if (argc != 1 || !valid_address(args))
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* EN: Reserve publication syntax without making a listener or retaining the callback.
 * RU: Зарезервировать синтаксис публикации без ожидания соединения и удержания колбэка. */
static FVM_Result publish_channel(FVM_Context *context, FVM_Object *self, unsigned argc,
    const FVM_Value *args, FVM_Value *out, void *state)
{
    (void)context;
    (void)self;
    (void)state;
    if (argc != 2 || !valid_address(args) ||
        args[1].type != FVM_VALUE_OBJECT || !args[1].object)
    {
        return FVM_INVALID_ARGUMENT;
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_host_exchange(FVM_Context *context, StyxRpcExchange *exchange)
{
    FVM_Value vfs;
    FVM_Result status = fvm_get_global(context, "vfs", &vfs);
    if (status != FVM_OK || vfs.type != FVM_VALUE_OBJECT)
    {
        return status == FVM_OK ? FVM_NOT_FOUND : status;
    }
    status = fvm_object_define_method_rooted(context, vfs.object, "send", send_value, exchange, NULL);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, vfs.object, "recv", recv_value, exchange, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, vfs.object, "ready", channel_ready, exchange, NULL);
    }
    return status;
}


/* [GC:] EN/RU: See session_methods.h. / См. session_methods.h. */
FVM_Result fvm_styxrpc_channel_members(FVM_Context *context, FVM_Object *object, FVM_StyxRpcSession *binding)
{
    FVM_Result status = fvm_object_define_method_rooted(context, object, "chan",
        fvm_styxrpc_open_named_channel, binding, NULL);
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "newchan", new_channel, binding, NULL);
    }
    if (status == FVM_OK)
    {
        status = fvm_object_define_method_rooted(context, object, "publishchan", publish_channel, binding, NULL);
    }
    return status;
}
