/* Copyright (c) 2026 bhgv
 * EN: Map compiled exports to the mounted gateway and verify exact load offers.
 * RU: Сопоставление скомпилированных экспортов со шлюзом и проверка предложений load.
 */
#include "load_profile.h"
#include "scalar_verify.h"
#include "call_record.h"
#include "flashvm/srecord.h"
#include <string.h>


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxloadedrequest(uint32_t signature)
{
    switch (signature)
    {
    case DVM_SCALAR_INT_SIGNATURE:
        return STYX_FUNCTION_REQUEST_TAG;
    case DVM_SCALAR_BIG_SIGNATURE:
        return STYX_FUNCTION_BIG_TAG;
    case DVM_TUPLE_SIGNATURE:
        return STYX_TUPLE_REQUEST_TAG;
    case DVM_REF_SIGNATURE:
        return STYX_REF_REQUEST_TAG;
    case DVM_CALL_LIST_SIGNATURE:
        return STYX_CALL_LIST_REQUEST_TAG;
    case DVM_REF_PAIR_SIGNATURE:
        return STYX_REF_PAIR_REQUEST_TAG;
    default:
        return 0;
    }
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxloadedoffer(const char *name, uint32_t signature, void *bytes, size_t capacity, size_t *used)
{
    size_t length = styxscalar_name_length(name);
    unsigned char head[INTERFACE_HEADER];
    styxscalar_interface_header(head, signature);
    return length && styxloadedrequest(signature) &&
        fvm_srecord_write_pair(STYX_FUNCTION_PROFILE_TAG, head, sizeof(head), name, length,
            bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxloadedaccept(const char *name, const void *bytes, size_t size)
{
    size_t length = styxscalar_name_length(name), used;
    FVM_SRecord record;
    if (!length ||
        fvm_srecord_read(bytes, size, STYX_SHAPED_OFFER_MAX, &record, &used) != FVM_S_OK || used != size)
    {
        return 0;
    }
    if (record.tag == STYX_SHAPED_OFFER_TAG)
    {
        char actual[INTERFACE_NAME_LIMIT + 1];
        unsigned long request = styxshapedpayload(record.payload, record.size, actual, NULL, NULL);
        return request && !strcmp(name, actual) ? request : 0;
    }
    if (record.tag != STYX_FUNCTION_PROFILE_TAG || record.size != INTERFACE_HEADER + length ||
        record.payload[0] != INTERFACE_KIND || memcmp(record.payload + INTERFACE_HEADER, name, length))
    {
        return 0;
    }
    uint32_t signature = 0;
    for (size_t i = 1; i < INTERFACE_HEADER; i++)
    {
        signature = (signature << 8) | record.payload[i];
    }
    return styxloadedrequest(signature);
}



/* EN: Advance cursor past one bounded type token or compound type.
 * RU: Продвинуть cursor за один скалярный или составной тип установленной глубины. */
static int shape_type(const char **cursor, unsigned depth)
{
    if (!**cursor || depth == 6)
    {
        return 0;
    }
    char type = *(*cursor)++;
    if (type == 'i' || type == 'b')
    {
        return 1;
    }
    if (type == 'r' && **cursor == '[')
    {
        ++*cursor;
        while (**cursor == 'i' || **cursor == 'b')
        {
            ++*cursor;
        }
        if (**cursor != ']')
        {
            return 0;
        }
        ++*cursor;
        return 1;
    }
    if (type != '(')
    {
        return 0;
    }
    unsigned count = 0;
    while (**cursor != ')')
    {
        if (!shape_type(cursor, depth + 1))
        {
            return 0;
        }
        count++;
    }
    ++*cursor;
    return count >= 2;
}


/* EN: Check a list of types. The result list has at most one top-level value.
 * RU: Проверить список типов. Список результата содержит до одного значения верхнего уровня. */
static int shape_list(const char *text, int result)
{
    if (!memchr(text, 0, STYX_SHAPE_MAX + 1) || *text++ != '(')
    {
        return 0;
    }
    size_t count = 0;
    while (*text != ')')
    {
        if (!shape_type(&text, 0))
        {
            return 0;
        }
        count++;
    }
    return !text[1] && (!result || count <= 1);
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxshapevalid(const StyxCallShape *shape)
{
    return shape && shape_list(shape->arguments, 0) && shape_list(shape->result, 1);
}


/* EN: Match portable types to the supported native ABI for signature.
 * RU: Сверить переносимые типы с поддерживаемым нативным ABI сигнатуры. */
static int signature_shape(uint32_t signature, const StyxCallShape *shape)
{
    const char *args, *result;
    switch (signature)
    {
    case DVM_SCALAR_INT_SIGNATURE:
        args = result = "(i)";
        break;
    case DVM_SCALAR_BIG_SIGNATURE:
        args = result = "(b)";
        break;
    case DVM_TUPLE_SIGNATURE:
        args = result = "((ib))";
        break;
    case DVM_REF_SIGNATURE:
        args = "(r[ib])";
        result = "((ib))";
        break;
    case DVM_REF_PAIR_SIGNATURE:
        args = "(r[ib]r[ib])";
        result = "((ib))";
        break;
    case DVM_CALL_LIST_SIGNATURE:
        args = styxcall_list_args;
        result = styxcall_list_result;
        break;
    default:
        return 0;
    }
    return styxshapevalid(shape) && !strcmp(args, shape->arguments) && !strcmp(result, shape->result);
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxshapedoffer(const char *name, uint32_t signature, const StyxCallShape *shape,
    void *bytes, size_t capacity, size_t *used)
{
    size_t length = styxscalar_name_length(name);
    if (!length || !signature_shape(signature, shape))
    {
        return 0;
    }
    size_t args = strlen(shape->arguments), result = strlen(shape->result);
    unsigned char payload[INTERFACE_HEADER + 3 + INTERFACE_NAME_LIMIT + 2 * STYX_SHAPE_MAX];
    styxscalar_interface_header(payload, signature);
    payload[INTERFACE_HEADER] = (unsigned char)length;
    payload[INTERFACE_HEADER + 1] = (unsigned char)args;
    payload[INTERFACE_HEADER + 2] = (unsigned char)result;
    unsigned char *data = payload + INTERFACE_HEADER + 3;
    memcpy(data, name, length);
    memcpy(data + length, shape->arguments, args);
    memcpy(data + length + args, shape->result, result);
    return fvm_srecord_write(STYX_SHAPED_OFFER_TAG, payload, INTERFACE_HEADER + 3 + length + args + result,
        bytes, capacity, used) == FVM_S_OK;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxshapedpayload(const void *payload, size_t length, char *name, uint32_t *signature, StyxCallShape *shape)
{
    const unsigned char *bytes = payload;
    if (!bytes || length < INTERFACE_HEADER + 3 || bytes[0] != INTERFACE_KIND)
    {
        return 0;
    }
    size_t n = bytes[INTERFACE_HEADER], a = bytes[INTERFACE_HEADER + 1], r = bytes[INTERFACE_HEADER + 2];
    const unsigned char *data = bytes + INTERFACE_HEADER + 3;
    if (!n || n > INTERFACE_NAME_LIMIT || a > STYX_SHAPE_MAX || r > STYX_SHAPE_MAX ||
        length != INTERFACE_HEADER + 3 + n + a + r || memchr(data, 0, n + a + r))
    {
        return 0;
    }
    uint32_t sig = 0;
    for (size_t i = 1; i < INTERFACE_HEADER; i++)
    {
        sig = (sig << 8) | bytes[i];
    }
    StyxCallShape types = {{0}, {0}};
    memcpy(types.arguments, data + n, a);
    memcpy(types.result, data + n + a, r);
    if (!signature_shape(sig, &types))
    {
        return 0;
    }
    if (name)
    {
        memcpy(name, data, n);
        name[n] = 0;
    }
    if (signature)
    {
        *signature = sig;
    }
    if (shape)
    {
        *shape = types;
    }
    return styxloadedrequest(sig);
}
