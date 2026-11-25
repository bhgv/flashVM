/* Copyright (c) 2026 bhgv
 * EN: Marshal copied mixed UniBus traffic between a Dis owner and the paired SDK.
 * RU: Передача копий смешанных сообщений между владельцем Dis и парной сессией SDK.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "paired_exchange.h"
#include "runtime_gc.h"
#include "sys_capture.h"
#include "disvm/unibus_binding.h"
#include "styxrpc/managed.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

extern char *dvm_runtime_file_path(String *name); /* Borrow UTF-8 after rejecting embedded NUL. */

#define EXCHANGE_WORD_BYTES 8U /* Width of the local numerical payload. */


/* EN: Close one leased port and release its native queue. Requires serialized ownership.
 * RU: Закрыть один порт и освободить нативную очередь. Требует единственного владельца. */
static void close_port(DvmPairedPort *port)
{
    styxrpc_messages_close(port->wire);
    styxrpc_exchange_destroy(port->exchange);
    memset(port, 0, sizeof(*port));
}


/* EN/RU: See paired_exchange.h. / См. paired_exchange.h. */
void dvm_paired_exchange_finish(DvmPairedExchange *exchange)
{
    for (size_t i = 0; i < DVM_EXCHANGE_SLOTS; ++i)
    {
        close_port(&exchange->ports[i]);
    }
}


/* EN: Look up a positive lease or select a free slot for zero. Keep slot storage in exchange.
 * RU: Найти положительный жетон либо свободное место для нуля. Слот хранится в exchange. */
static DvmPairedPort *select_port(DvmPairedExchange *exchange, int lease)
{
    for (size_t i = 0; lease >= 0 && i < DVM_EXCHANGE_SLOTS; ++i)
    {
        if (exchange->ports[i].lease == lease)
        {
            return &exchange->ports[i];
        }
    }
    return NULL;
}


/* [ASINC] EN: Open one exact exchange root outside managed admission. Return its lease or -1.
 * RU: Открыть точный корень обмена вне допуска к куче. Вернуть жетон либо -1. */
static WORD open_port(DvmPairedExchange *exchange, struct StyxClient *client, const char *address)
{
    DvmPairedPort *port = select_port(exchange, 0);
    if (!port || exchange->last == INT_MAX)
    {
        return -1;
    }
    int released = dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        return -1;
    }
    port->exchange = styxrpc_exchange_create();
    if (port->exchange)
    {
        StyxRpcMessageType types[STYXRPC_EXCHANGE_END - 1];
        StyxRpcMessageSpec spec;
        styxrpc_exchange_spec(port->exchange, 0, types, &spec);
        if (styxrpc_messages_open_at(client, address, &spec, &port->wire) == STYXRPC_OK)
        {
            styxrpc_exchange_attach(port->exchange, port->wire);
            port->lease = ++exchange->last;
        }
        else
        {
            close_port(port);
        }
    }
    dvm_runtime_gc_native_enter(released);
    return port->lease ? port->lease : -1;
}


/* EN: Decode or copy a local payload, and queue its canonical UniBus bytes in port.
 * RU: Декодировать или скопировать локальные данные и поставить каноническую запись в port. */
static StyxRpcStatus queue_frame(DvmPairedPort *port, const unsigned char *data, size_t size)
{
    StyxRpcExchangeKind kind = (StyxRpcExchangeKind)*data++;
    --size;
    if (kind == STYXRPC_EXCHANGE_INT || kind == STYXRPC_EXCHANGE_REAL)
    {
        if (size != EXCHANGE_WORD_BYTES)
        {
            return STYXRPC_INVALID;
        }
        uint64_t bits = 0;
        for (size_t i = 0; i < size; ++i)
        {
            bits = (bits << 8) | data[i];
        }
        unsigned char wire[(3U * FVM_SRECORD_INTEGER_MAX + EXCHANGE_WORD_BYTES + 1U)];
        StyxRpcStatus encoded;
        if (kind == STYXRPC_EXCHANGE_INT)
        {
            int64_t value = bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
            encoded = styxrpc_value_integer(STYXRPC_VALUE_INT, value, wire, sizeof(wire), &size);
        }
        else
        {
            double value;
            if (sizeof(value) != sizeof(bits))
            {
                return STYXRPC_INVALID;
            }
            memcpy(&value, &bits, sizeof(value));
            encoded = styxrpc_value_real(value, wire, sizeof(wire), &size);
        }
        return encoded == STYXRPC_OK ? styxrpc_exchange_send(port->exchange, kind, wire, size) : encoded;
    }
    if ((kind == STYXRPC_EXCHANGE_BOOL && (size != 1 || data[0] > 1)) ||
        (kind == STYXRPC_EXCHANGE_NULL && size != 0))
    {
        return STYXRPC_INVALID;
    }
    return styxrpc_exchange_send(port->exchange, kind, data, size);
}


/* EN: Decode managed numerical bytes into the local eight-byte form. Other kinds stay counted.
 * RU: Перевести управляемые числа в локальную восьмибайтовую форму; прочие остаются с длиной. */
static int unpack_frame(StyxRpcExchangeKind kind, unsigned char *data, size_t *size)
{
    uint64_t bits;
    if (kind == STYXRPC_EXCHANGE_INT)
    {
        int64_t value;
        if (styxrpc_value_get_integer(STYXRPC_VALUE_INT, data, *size, &value) != STYXRPC_OK)
        {
            return 0;
        }
        bits = (uint64_t)value;
    }
    else if (kind == STYXRPC_EXCHANGE_REAL)
    {
        double value;
        if (sizeof(value) != sizeof(bits) || styxrpc_value_get_real(data, *size, &value) != STYXRPC_OK)
        {
            return 0;
        }
        memcpy(&bits, &value, sizeof(bits));
    }
    else
    {
        return (kind != STYXRPC_EXCHANGE_BOOL || (*size == 1 && data[0] <= 1)) &&
            (kind != STYXRPC_EXCHANGE_NULL || !*size);
    }
    for (size_t i = EXCHANGE_WORD_BYTES; i > 0; --i)
    {
        data[i - 1] = (unsigned char)bits;
        bits >>= 8;
    }
    *size = EXCHANGE_WORD_BYTES;
    return 1;
}


/* [ASINC] [GC:] EN: Copy the guest array into private storage before leaving managed admission.
 * RU: Скопировать массив гостя в частный буфер до выхода из допуска к куче. */
static WORD send_frame(DvmPairedPort *port, Array *array)
{
    if (!port || !array || array == H || !array->t || array->t == H || array->t->size != 1 || array->t->np ||
        array->len < 1 || (size_t)array->len > 1U + STYXRPC_MESSAGE_PAYLOAD_MAX)
    {
        return -1;
    }
    /* Leave before queuing. Only the original owner has a live frame and owns this port. */
    unsigned char bytes[1U + STYXRPC_MESSAGE_PAYLOAD_MAX];
    size_t size = array->len;
    memcpy(bytes, array->data, size);
    int released = dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        return 1;
    }
    StyxRpcStatus status = queue_frame(port, bytes, size);
    if (status == STYXRPC_OK && styxrpc_messages_flush(port->wire) != STYXRPC_OK)
    {
        close_port(port);
        status = STYXRPC_INVALID;
    }
    dvm_runtime_gc_native_enter(released);
    return status == STYXRPC_OK ? 0 : status == STYXRPC_MORE ? 1 : -1;
}


/* [ASINC] EN: Poll one bounded batch only for an empty queue; copy one retained message.
 * RU: Опросить одну порцию только при пустой очереди; скопировать одну удержанную запись. */
static StyxRpcStatus take_frame(DvmPairedPort *port, unsigned char *bytes, size_t *size)
{
    StyxRpcExchangeKind kind;
    StyxRpcStatus status = styxrpc_exchange_recv(port->exchange, &kind, bytes + 1,
        STYXRPC_MESSAGE_PAYLOAD_MAX, size);
    if (status == STYXRPC_MORE)
    {
        int released = dvm_runtime_gc_native_try_leave();
        if (released < 0)
        {
            return STYXRPC_MORE;
        }
        if (styxrpc_messages_pump(port->wire) == STYXRPC_INVALID)
        {
            close_port(port);
            status = STYXRPC_INVALID;
        }
        else
        {
            status = styxrpc_exchange_recv(port->exchange, &kind, bytes + 1, STYXRPC_MESSAGE_PAYLOAD_MAX, size);
        }
        dvm_runtime_gc_native_enter(released);
    }
    if (status == STYXRPC_OK)
    {
        if (!unpack_frame(kind, bytes + 1, size))
        {
            return STYXRPC_INVALID;
        }
        bytes[0] = (unsigned char)kind;
        ++*size;
    }
    return status;
}


/* [ASINC] [GC:] EN: Publish one copied array or nil through the caller's result slot.
 * RU: Выдать скопированный массив или nil через результирующий слот вызывающего. */
static void recv_frame(DvmPairedExchange *exchange, void *frame)
{
    F_Unibus_rpcrecv call;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_rpcrecv, ret), sizeof(*call.ret));
    Array *old = dvm_sys_owner(call.ret);
    DvmPairedPort *port = call.port > 0 ? select_port(exchange, call.port) : NULL;
    unsigned char bytes[1U + STYXRPC_MESSAGE_PAYLOAD_MAX];
    size_t size;
    StyxRpcStatus status = port ? take_frame(port, bytes, &size) : STYXRPC_INVALID;
    if (status == STYXRPC_INVALID)
    {
        error("StyxRPC receive failed");
    }
    Array *result = H;
    if (status == STYXRPC_OK)
    {
        result = H2D(Array *, heaparray(&Tbyte, (int)size));
        memcpy(result->data, bytes, size);
    }
    memmove(call.ret, &result, sizeof(result));
    destroy(old);
}


/* [ASINC] [GC:] EN/RU: See paired_exchange.h. / См. paired_exchange.h. */
int dvm_paired_exchange_invoke(DvmPairedExchange *exchange, struct StyxClient *client,
    unsigned method, void *frame)
{
    switch (method)
    {
    case UNIBUS_RPCOPEN:
    {
        F_Unibus_rpcopen call;
        char address[STYXRPC_TREE_NAME + 1];
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_rpcopen, ret), sizeof(*call.ret));
        WORD result = -1;
        if (call.address && call.address != H)
        {
            const char *name = dvm_runtime_file_path(call.address);
            if (name && *name == '/')
            {
                ++name;
            }
            size_t size = name ? strlen(name) : 0;
            if (size && size <= STYXRPC_TREE_NAME && !strchr(name, '/'))
            {
                memcpy(address, name, size + 1);
                result = open_port(exchange, client, address);
            }
        }
        dvm_sys_scalar_result(&result, call.ret, sizeof(result));
        return 1;
    }
    case UNIBUS_RPCSEND:
    {
        F_Unibus_rpcsend call;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_rpcsend, ret), sizeof(*call.ret));
        WORD result = send_frame(call.port > 0 ? select_port(exchange, call.port) : NULL, call.frame);
        dvm_sys_scalar_result(&result, call.ret, sizeof(result));
        return 1;
    }
    case UNIBUS_RPCRECV:
        recv_frame(exchange, frame);
        return 1;
    case UNIBUS_RPCCLOSE:
    {
        F_Unibus_rpcclose call;
        dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Unibus_rpcclose, ret), sizeof(*call.ret));
        DvmPairedPort *port = call.port > 0 ? select_port(exchange, call.port) : NULL;
        int released = port ? dvm_runtime_gc_native_try_leave() : -1;
        WORD result = -1;
        if (released >= 0)
        {
            close_port(port);
            dvm_runtime_gc_native_enter(released);
            result = 0;
        }
        dvm_sys_scalar_result(&result, call.ret, sizeof(result));
        return 1;
    }
    default:
        return 0;
    }
}
