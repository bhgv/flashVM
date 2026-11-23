/* Copyright (c) 2026 bhgv
 * EN: Map compiled exports to the mounted gateway and verify exact load offers.
 * RU: Сопоставление скомпилированных экспортов со шлюзом и проверка предложений load.
 */
#include "load_profile.h"
#include "scalar_verify.h"
#include "call_record.h"
#include "tuple_record.h"
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
unsigned long styxloadedacceptshape(const char *name, const void *bytes, size_t size, StyxCallShape *shape)
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
        StyxCallShape decoded;
        unsigned long request = styxshapedpayload(record.payload, record.size, actual, NULL, &decoded);
        if (!request || strcmp(name, actual))
        {
            return 0;
        }
        if (shape)
        {
            *shape = decoded;
        }
        return request;
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



/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxloadedaccept(const char *name, const void *bytes, size_t size)
{
    return styxloadedacceptshape(name, bytes, size, NULL);
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
    if (type == 'i' || type == 'b' || type == 'f' || type == 'y' || type == 's' || type == 'h')
    {
        return 1;
    }
    if ((type == 'r' || type == 'a' || type == 'c') && **cursor == '[')
    {
        ++*cursor;
        if (type != 'r' && !shape_type(cursor, depth + 1))
        {
            return 0;
        }
        while (type == 'r' && **cursor != ']')
        {
            if (!shape_type(cursor, depth + 1))
            {
                return 0;
            }
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


/* EN/RU: See load_profile.h. / См. load_profile.h. */
const char *styxshape_end(const char *type)
{
    return type && shape_type(&type, 0) ? type : NULL;
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


/* EN: Check one scalar-only list and compute its flattened cell count.
 * RU: Проверить список скалярных типов и посчитать ячейки после раскрытия кортежей. */
static int scalar_cells(const char *shape, size_t *count)
{
    StyxCallValues cells = {0};
    for (const char *type = shape; *type; type++)
    {
        if (*type == 'i' || *type == 'b' || *type == 'f' || *type == 'y')
        {
            cells.count++;
        }
        else if (*type != '(' && *type != ')')
        {
            return 0;
        }
    }
    unsigned char bytes[STYX_FUNCTION_FRAME_CAPACITY];
    size_t used;
    if (cells.count > STYX_DYNAMIC_CELLS ||
        !styxcallrecordwrite(STYX_DYNAMIC_REQUEST_TAG, shape, &cells, bytes, sizeof(bytes), &used))
    {
        return 0;
    }
    *count = cells.count;
    return 1;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxshapecells(const StyxCallShape *shape, size_t *arguments, size_t *result)
{
    size_t a, r;
    if (!arguments || !result || !styxshapevalid(shape) ||
        !scalar_cells(shape->arguments, &a) || !scalar_cells(shape->result, &r))
    {
        return 0;
    }
    *arguments = a;
    *result = r;
    return 1;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxshaperef(const StyxCallShape *shape, size_t *results)
{
    if (!results || !styxshapevalid(shape))
    {
        return 0;
    }
    size_t refs = 0;
    for (const char *token = shape->arguments; *token; )
    {
        if (*token == '(' || *token == ')')
        {
            token++;
        }
        else if (!strncmp(token, "r[ib]", 5))
        {
            refs++;
            token += 5;
        }
        else
        {
            return 0;
        }
    }
    if (!refs || refs > 2)
    {
        return 0;
    }
    if (!strcmp(shape->result, "()"))
    {
        *results = 0;
    }
    else if (!strcmp(shape->result, "(i)"))
    {
        *results = 1;
    }
    else if (!strcmp(shape->result, "((ib))"))
    {
        *results = 2;
    }
    else
    {
        return 0;
    }
    return refs == 1 ? STYX_REF_REQUEST_TAG : STYX_REF_PAIR_REQUEST_TAG;
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


/* EN: Collect the bounded ref leaves across argument tuples for reply validation.
 * RU: Собрать ref-поля из кортежей аргументов для проверки ответа. */
int styxmanaged_refs(const char *shape, char refs[STYX_SHAPE_MAX + 1], unsigned *mask)
{
    unsigned index = 0;
    size_t length = 1;
    *mask = 0;
    refs[0] = '(';
    for (const char *type = shape + 1; *type; )
    {
        if (*type == '(' || *type == ')')
        {
            type++;
            continue;
        }
        const char *end = styxshape_end(type);
        if (!end || index >= STYX_CALL_VALUES)
        {
            return 0;
        }
        if (*type == 'r')
        {
            char fields[STYX_SHAPE_MAX + 1];
            StyxScalarLayout layout;
            size_t n = (size_t)(end - type);
            if (length + n + 1 > STYX_SHAPE_MAX || !styxmanaged_ref_fields(type, fields) ||
                !styxmanaged_layout(fields, &layout) || layout.size > 64)
            {
                return 0;
            }
            memcpy(refs + length, type, n);
            length += n;
            *mask |= 1U << index;
        }
        index++;
        type = end;
    }
    refs[length++] = ')';
    refs[length] = 0;
    return 1;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxshaperequest(uint32_t signature, const StyxCallShape *shape)
{
    unsigned long existing = styxloadedrequest(signature);
    if (existing)
    {
        return signature_shape(signature, shape) ? existing : 0;
    }
    size_t args, results;
    if (!signature)
    {
        return 0;
    }
    if (styxshapecells(shape, &args, &results))
    {
        return STYX_DYNAMIC_REQUEST_TAG;
    }
    unsigned long ref = styxshaperef(shape, &results);
    StyxScalarLayout input, output;
    if (ref || !styxshapevalid(shape) ||
        !styxmanaged_layout(shape->arguments, &input) || !styxmanaged_layout(shape->result, &output) ||
        input.size > 64 || output.size > 64 ||
        !styxmanaged_result_refs(shape->arguments) || !styxmanaged_result_refs(shape->result) ||
        !styxmanaged_arrays(shape->arguments, 1) || !styxmanaged_arrays(shape->result, 1) ||
        !(input.pointers || output.pointers))
    {
        return ref;
    }
    return STYX_MANAGED_REQUEST_TAG;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxshapedoffer(const char *name, uint32_t signature, const StyxCallShape *shape,
    void *bytes, size_t capacity, size_t *used)
{
    size_t length = styxscalar_name_length(name);
    if (!length || !styxshaperequest(signature, shape))
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
    unsigned long request = styxshaperequest(sig, &types);
    if (!request)
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
    return request;
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
unsigned long styxmanaged_capability(unsigned long high, unsigned long low)
{
    static const unsigned long tags[] = {STYX_MANAGED_DELTA_REQUEST_TAG, STYX_MANAGED_SPAN_REQUEST_TAG,
        STYX_MANAGED_RUN_REQUEST_TAG, STYX_MANAGED_SPLICE_REQUEST_TAG,
        STYX_MANAGED_TEXT_REQUEST_TAG, STYX_MANAGED_EDIT_REQUEST_TAG, STYX_MANAGED_WIDE_REFS_TAG,
        STYX_MANAGED_GRAPH_TAG}; /* Index is capability file length. */
    return !high && low < sizeof(tags) / sizeof(tags[0]) ? tags[low] : tags[0];
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxmanaged_has_snapshot(unsigned long tag)
{
    return tag == STYX_MANAGED_POSITION_FLAT_TAG ||
        (tag >= STYX_MANAGED_SPAN_REQUEST_TAG && tag <= STYX_MANAGED_WIDE_REFS_TAG);
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxmanaged_request_tag(unsigned long tag)
{
    return tag == STYX_MANAGED_REQUEST_TAG || tag == STYX_MANAGED_DELTA_REQUEST_TAG ||
        styxmanaged_graph_tag(tag) || styxmanaged_has_snapshot(tag);
}

/* EN: Legacy requests carry ref leaves in tuples and standalone ADT fields only.
 * RU: Старые запросы поддерживают ref в кортежах и поля без вложенных ссылок. */
int styxmanaged_graph_required(const char *arguments)
{
    char refs[STYX_SHAPE_MAX + 1];
    unsigned mask;
    return strpbrk(arguments, "hc") || !styxmanaged_refs(arguments, refs, &mask) ||
        !styxmanaged_arrays(arguments, 0);
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxmanaged_graph_tag(unsigned long tag)
{
    return tag == STYX_MANAGED_GRAPH_TAG || styxmanaged_sparse_graph_tag(tag);
}


/* EN/RU: See load_profile.h. / См. load_profile.h. */
int styxmanaged_sparse_graph_tag(unsigned long tag)
{
    return tag == STYX_MANAGED_SPARSE_GRAPH_TAG || tag == STYX_MANAGED_POSITION_GRAPH_TAG;
}
