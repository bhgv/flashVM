/* Copyright (c) 2026 bhgv
 * EN: Encode counted values or pass an existing live reference descriptor.
 * RU: Кодировать значения с длиной либо передавать дескриптор живой ссылки.
 */
#include "session_named_values.hpp"
#include "session_named_refs.hpp"
#include "session_named_snapshot.hpp"
#include "flashvm/serialization.h"
#include "../capi/variable_refs.hpp"
#include "styxrpc/named_channel.h"
#include <stdlib.h>
#include <string.h>
extern "C"
{
#include "variable_ref_adapter.h"
#include "managed_values.h"
}

namespace
{


/* EN: Materialize one type byte followed by a copied payload of size bytes.
 * RU: Создать байт типа, за которым следует копия size байтов. */
FVM_Result pack(unsigned tag, const unsigned char *payload, size_t size,
    unsigned char **out, size_t *extent)
{
    if (size >= STYXRPC_NAMED_MAX)
    {
        return FVM_INVALID_ARGUMENT;
    }
    unsigned char *frame = static_cast<unsigned char *>(malloc(size + 1));
    if (!frame)
    {
        return FVM_OUT_OF_MEMORY;
    }
    frame[0] = static_cast<unsigned char>(tag);
    if (size)
    {
        memcpy(frame + 1, payload, size);
    }
    *out = frame;
    *extent = size + 1;
    return FVM_OK;
}


/* [GC:] EN: Preserve counted UTF-8/WTF-8 through an owned VM string.
 * RU: Сохранить UTF-8/WTF-8 с длиной в строке, принадлежащей VM. */
FVM_Result unpack_text(FVM_Context *context, const unsigned char *payload, size_t size, FVM_Value *out)
{
    unsigned short *units = static_cast<unsigned short *>(malloc((size + 1) * sizeof(*units)));
    if (!units)
    {
        return FVM_OUT_OF_MEMORY;
    }
    FVM_Value borrowed;
    FVM_Result status = fvm_value_string_bytes(&borrowed, reinterpret_cast<const char *>(payload), size);
    size_t count;
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
}


/* [GC:] EN/RU: See session_named_values.hpp. / См. session_named_values.hpp. */
FVM_Result fvm_named_encode(FVM_Context *context, FVM_StyxRpcSession *session,
    const FVM_Value *value, unsigned char **bytes, size_t *size)
{
    unsigned char wire[STYXRPC_NAMED_NUMBER_BYTES];
    if (fvm_is_variable_ref(value))
    {
        unsigned char descriptor[STYXRPC_INT_REF_SIZE];
        if (fvm_styxrpc_encode_int_ref(session, value, descriptor, sizeof(descriptor)))
        {
            return pack(STYXRPC_NAMED_INT_REF, descriptor, sizeof(descriptor), bytes, size);
        }
        unsigned char token[STYXRPC_CELL_TOKEN];
        FVM_Result status = fvm_named_ref_token(context, session, value, token);
        return status == FVM_OK ? pack(STYXRPC_NAMED_CELL_REF, token, sizeof(token), bytes, size) : status;
    }
    switch (value->type)
    {
    case FVM_VALUE_NUMBER:
    {
        uint64_t bits;
        if (sizeof(value->number) != sizeof(bits))
        {
            return FVM_UNSUPPORTED;
        }
        memcpy(&bits, &value->number, sizeof(bits));
        for (unsigned i = sizeof(wire); i; --i)
        {
            wire[i - 1] = static_cast<unsigned char>(bits);
            bits >>= 8;
        }
        return pack(STYXRPC_NAMED_NUMBER, wire, sizeof(wire), bytes, size);
    }
    case FVM_VALUE_STRING:
        return pack(STYXRPC_NAMED_TEXT, reinterpret_cast<const unsigned char *>(value->string),
            fvm_value_string_size(value), bytes, size);
    case FVM_VALUE_BOOLEAN:
        wire[0] = value->boolean != 0;
        return pack(STYXRPC_NAMED_BOOLEAN, wire, 1, bytes, size);
    case FVM_VALUE_NULL:
        return pack(STYXRPC_NAMED_NULL, NULL, 0, bytes, size);
    case FVM_VALUE_UNDEFINED:
        return pack(STYXRPC_NAMED_UNDEFINED, NULL, 0, bytes, size);
    default:
        break;
    }
    unsigned char *snapshot;
    size_t length;
    FVM_Result status = fvm_named_snapshot(context, value, &snapshot, &length);
    if (status == FVM_OK)
    {
        status = pack(STYXRPC_NAMED_VALUE, snapshot, length, bytes, size);
        fvm_serialized_free(snapshot);
    }
    return status;
}


/* [GC:] EN/RU: See session_named_values.hpp. / См. session_named_values.hpp. */
FVM_Result fvm_named_decode(FVM_Context *context, FVM_StyxRpcSession *session,
    const unsigned char *bytes, size_t size, FVM_Value *out)
{
    if (!size)
    {
        return FVM_INVALID_ARGUMENT;
    }
    const unsigned char *payload = bytes + 1;
    --size;
    switch (bytes[0])
    {
    case STYXRPC_NAMED_CELL_REF:
        return fvm_named_ref_bind(context, session, payload, size, out);
    case STYXRPC_NAMED_INT_REF:
        return fvm_styxrpc_decode_int_ref(context, session, payload, size, out);
    case STYXRPC_NAMED_VALUE:
        return fvm_value_deserialize(context, payload, size, out);
    case STYXRPC_NAMED_TEXT:
        return unpack_text(context, payload, size, out);
    case STYXRPC_NAMED_NUMBER:
    {
        double number;
        uint64_t bits = 0;
        if (sizeof(number) != sizeof(bits) || size != STYXRPC_NAMED_NUMBER_BYTES)
        {
            return FVM_INVALID_ARGUMENT;
        }
        for (size_t i = 0; i < size; ++i)
        {
            bits = (bits << 8) | payload[i];
        }
        memcpy(&number, &bits, sizeof(number));
        fvm_value_number(out, number);
        return FVM_OK;
    }
    case STYXRPC_NAMED_BOOLEAN:
        if (size == 1 && payload[0] <= 1)
        {
            fvm_value_boolean(out, payload[0]);
            return FVM_OK;
        }
        break;
    case STYXRPC_NAMED_NULL:
    case STYXRPC_NAMED_UNDEFINED:
        if (!size)
        {
            fvm_value_undefined(out);
            if (*bytes == STYXRPC_NAMED_NULL)
            {
                out->type = FVM_VALUE_NULL;
            }
            return FVM_OK;
        }
        break;
    default:
        break;
    }
    return FVM_INVALID_ARGUMENT;
}
