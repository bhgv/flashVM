/* Copyright (c) 2026 bhgv
 * EN: Encode bounded strings, recursive arrays and mixed value frames.
 * RU: Кодирование строк, вложенных массивов и смешанных кадров значений.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "kernel.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "managed_value.h"
#include "managed_array.h"
#include "managed_ref.h"
#include "disvm/refcount.h"
#include "shared_bytes.h"
#include "flashvm/utf8.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include "../../native/terminal/load_profile.h"
#include <string.h>
#include <stdlib.h>


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_managed_new(const char *type, const FVM_SRecord *record, void **result)
{
    if (!type || !record || !result || record->size > STYX_MANAGED_LIST_LIMIT ||
        (!record->payload && record->size))
    {
        return 0;
    }
    int string = !strcmp(type, "s");
    const char *end = styxshape_end(type);
    int array = type[0] == 'a' && end && !*end && styxmanaged_arrays(type);
    if (!string && !array)
    {
        return 0;
    }
    if (record->tag == STYX_MANAGED_NIL_TAG && !record->size)
    {
        *result = H;
        return 1;
    }
    if (array)
    {
        return type[3] == ']' && dvm_styx_primitive_array_supported(type[2]) ?
            dvm_styx_primitive_array_new(type[2], record, result) :
            dvm_styx_composite_array_new(type, record, result);
    }
    if (record->tag != STYX_MANAGED_STRING_TAG ||
        fvm_utf8_validate((const char *)record->payload, record->size, NULL) != FVM_UTF8_OK)
    {
        return 0;
    }
    *result = c2string((char *)record->payload, (int)record->size);
    return 1;
}


/* [GC:] EN: Snapshot one counted ASCII string or convert copied native runes to UTF-8.
 * RU: Скопировать строку ASCII с её длиной либо преобразовать копию Rune в UTF-8. */
static int write_string(String *text, void *bytes, size_t capacity, size_t *used)
{
    int length = text->len;
    int wide = length < 0;
    size_t count = wide ? (size_t)(-(int64_t)length) : (size_t)length;
    if (text->max < 0 || count > (size_t)text->max || count > capacity)
    {
        return 0;
    }
    size_t extent = count * (wide ? sizeof(Rune) : 1);
    unsigned char *snapshot = extent ? malloc(extent) : NULL;
    unsigned char *utf8 = wide ? malloc(capacity) : snapshot;
    int ok = (!extent || snapshot) && (!wide || utf8) &&
        !dvm_runtime_bytes_snapshot(&text->data, snapshot, extent);
    size_t size = wide ? 0 : count;
    if (ok && wide)
    {
        for (size_t i = 0; ok && i < count; i++)
        {
            Rune rune;
            size_t width;
            memcpy(&rune, snapshot + i * sizeof(Rune), sizeof(rune));
            ok = fvm_utf8_write(rune, (char *)utf8 + size, capacity - size, &width) == FVM_UTF8_OK;
            if (ok)
            {
                size += width;
            }
        }
    }
    ok = ok && fvm_srecord_write(STYX_MANAGED_STRING_TAG, utf8, size, bytes, capacity, used) == FVM_S_OK;
    if (wide)
    {
        free(utf8);
    }
    free(snapshot);
    return ok;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_managed_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used)
{
    if (!type || !value || !bytes || !used || !capacity)
    {
        return 0;
    }
    int string = !strcmp(type, "s");
    const char *end = styxshape_end(type);
    int array = type[0] == 'a' && end && !*end && styxmanaged_arrays(type);
    if (!string && !array)
    {
        return 0;
    }
    if (capacity > STYX_MANAGED_LIST_LIMIT)
    {
        capacity = STYX_MANAGED_LIST_LIMIT;
    }
    if (value == H)
    {
        return fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, used) == FVM_S_OK;
    }
    if (D2H(value)->t != (string ? &Tstring : &Tarray))
    {
        return 0;
    }
    if (string)
    {
        return write_string(value, bytes, capacity, used);
    }
    return type[3] == ']' && dvm_styx_primitive_array_supported(type[2]) ?
        dvm_styx_primitive_array_write(type[2], value, bytes, capacity, used) :
        dvm_styx_composite_array_write(type, value, bytes, capacity, used);
}


/* EN: Check frame layout and recursive value types before a bridge admits the call.
 * RU: Проверить раскладку кадра и вложенные типы перед допуском вызова. */
int dvm_styx_value_shape_supported(const char *shape)
{
    StyxScalarLayout layout;
    if (!shape || !styxmanaged_layout(shape, &layout))
    {
        return 0;
    }
    char refs[STYX_SHAPE_MAX + 1];
    unsigned mask;
    if (!styxmanaged_refs(shape, refs, &mask))
    {
        return 0;
    }
    return styxmanaged_arrays(shape);
}


typedef struct FrameDecode
{
    const char *shape; /* Borrowed validated list shape. */
    FVM_SRecord list; /* Borrowed validated input list. */
    DVM_StyxValueFrame pending; /* Owned local staging until publication. */
    size_t leaf; /* Count of populated slots, in shape order. */
    DVM_StyxRefTable refs; /* Borrowed aliases of decoded argument slots. */
} FrameDecode;


/* [GC:] EN: Copy one validated node or recurse into a tuple.
 * RU: Скопировать один проверенный узел либо рекурсивно разобрать кортеж. */
static int decode_nodes(FrameDecode *decode, const char *shape, const unsigned char *data, size_t size)
{
    while (*shape != ')')
    {
        FVM_SRecord node;
        size_t used;
        if (fvm_srecord_read(data, size, STYX_MANAGED_LIST_LIMIT, &node, &used) != FVM_S_OK)
        {
            return 0;
        }
        const char *end = styxshape_end(shape);
        if (!end)
        {
            return 0;
        }
        if (*shape == '(')
        {
            if (!decode_nodes(decode, shape + 1, node.payload, node.size))
            {
                return 0;
            }
        }
        else
        {
            size_t slot = decode->leaf;
            StyxScalarLayout *layout = &decode->pending.layout;
            if (slot >= layout->count)
            {
                return 0;
            }
            unsigned char *destination = (unsigned char *)decode->pending.words + layout->offset[slot];
            if (layout->pointers & (1U << slot))
            {
                char type[STYX_SHAPE_MAX + 1] = {0};
                void *object;
                if (*shape == 'r')
                {
                    if (!dvm_styx_ref_read(&decode->refs, shape, &node, &object))
                    {
                        return 0;
                    }
                }
                else
                {
                    if (end - shape >= (ptrdiff_t)sizeof(type))
                    {
                        return 0;
                    }
                    memcpy(type, shape, (size_t)(end - shape));
                    if (!dvm_styx_managed_new(type, &node, &object))
                    {
                        return 0;
                    }
                }
                memcpy(destination, &object, sizeof(object));
                if (object != H)
                {
                    ptradd(D2H(object));
                    decode->pending.roots |= 1U << slot;
                    Setmark(D2H(object));
                }
            }
            else
            {
                StyxScalarKind kind;
                int64_t value;
                if (!styxscalarread(node.tag, data, used, &kind, &value))
                {
                    return 0;
                }
                if (layout->width[slot] == 1)
                {
                    *destination = (unsigned char)value;
                }
                else
                {
                    memcpy(destination, &value, sizeof(value));
                }
            }
            decode->leaf++;
        }
        shape = end;
        data += used;
        size -= used;
    }
    return size == 0;
}


/* [GC:] EN: Run the decoder within the VM exception boundary.
 * RU: Выполнить декодирование в границах обработчика ошибок VM. */
static int decode_frame(void *state)
{
    FrameDecode *decode = state;
    return decode_nodes(decode, decode->shape + 1, decode->list.payload, decode->list.size);
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
void dvm_styx_value_frame_clear(DVM_StyxValueFrame *frame)
{
    unsigned char *storage = (unsigned char *)frame->words;
    for (size_t i = 0; i < frame->layout.count; i++)
    {
        if (frame->layout.pointers & (1U << i))
        {
            void *object;
            memcpy(&object, storage + frame->layout.offset[i], sizeof(object));
            if (object && object != H)
            {
                if (frame->roots & (1U << i))
                {
                    ptrdel(D2H(object));
                }
                destroy(object);
            }
        }
    }
    memset(frame, 0, sizeof(*frame));
}


/* EN/RU: See managed_array.h. / См. managed_array.h. */
int dvm_styx_array_element_read(const char *shape, const unsigned char *record, size_t size,
    const StyxScalarLayout *layout, DVM_StyxValueFrame *element)
{
    FrameDecode decode = {.shape = shape, .list = {.payload = record, .size = size}};
    decode.pending.layout = *layout;
    if (!styxmanaged_payload_valid(shape, record, size) ||
        !kprotect(decode_frame, &decode) || decode.leaf != layout->count)
    {
        dvm_styx_value_frame_clear(&decode.pending);
        return 0;
    }
    *element = decode.pending;
    return 1;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_value_frame_read(unsigned long tag, const char *shape, const void *bytes, size_t size,
    DVM_StyxValueFrame *frame)
{
    FrameDecode decode = {.shape = shape};
    if (!frame || !styxmanaged_layout(shape, &decode.pending.layout) ||
        !styxmanaged_list_read(tag, shape, bytes, size, &decode.list))
    {
        return 0;
    }
    if (!kprotect(decode_frame, &decode) || decode.leaf != decode.pending.layout.count)
    {
        dvm_styx_value_frame_clear(&decode.pending);
        return 0;
    }
    *frame = decode.pending;
    return 1;
}


typedef struct FrameEncode
{
    StyxScalarLayout layout; /* Offsets and kinds of the borrowed source slots. */
    const unsigned char *slots; /* Borrowed source; managed owners remain pinned. */
    unsigned char *scratch; /* Separate buffer per tuple depth. */
    size_t capacity; /* Size of each scratch region. */
    size_t leaf; /* Next source slot in schema order. */
    DVM_StyxRefTable refs; /* Borrowed identities during encoding. */
} FrameEncode;


/* [GC:] EN: Encode one checked scalar or managed slot into caller-owned scratch.
 * RU: Кодировать один проверенный скалярный или управляемый слот в scratch. */
static int encode_leaf(FrameEncode *encode, const char *shape, const char *end,
    unsigned char *bytes, size_t capacity, size_t *used)
{
    size_t leaf = encode->leaf++;
    const unsigned char *slot = encode->slots + encode->layout.offset[leaf];
    if (encode->layout.pointers & (1U << leaf))
    {
        char type[STYX_SHAPE_MAX + 1] = {0};
        void *value;
        memcpy(&value, slot, sizeof(value));
        if (*shape == 'r')
        {
            return dvm_styx_ref_write(&encode->refs, shape, value, bytes, capacity, used);
        }
        if (end - shape >= (ptrdiff_t)sizeof(type))
        {
            return 0;
        }
        memcpy(type, shape, (size_t)(end - shape));
        return dvm_styx_managed_write(type, value, bytes, capacity, used);
    }
    int64_t value;
    unsigned long tag;
    switch (*shape)
    {
    case 'y':
        return styxscalarwrite(STYX_CALL_BYTE_TAG, STYX_SCALAR_VALUE, *slot, bytes, capacity, used);
    case 'i':
        tag = STYX_CALL_INT_TAG;
        break;
    case 'b':
        tag = STYX_CALL_BIG_TAG;
        break;
    case 'f':
        tag = STYX_CALL_REAL_TAG;
        break;
    default:
        return 0;
    }
    memcpy(&value, slot, sizeof(value));
    return styxscalarwrite(tag, STYX_SCALAR_VALUE, value, bytes, capacity, used);
}


/* [GC:] EN: Encode ordered siblings; recurse into nonoverlapping tuple buffers.
 * RU: Кодировать соседние поля по порядку, используя отдельные буферы кортежей. */
static int encode_nodes(FrameEncode *encode, const char *shape, unsigned depth, size_t *size)
{
    if (depth >= DVM_STYX_TUPLE_SCRATCH)
    {
        return 0;
    }
    unsigned char *dest = encode->scratch + depth * encode->capacity;
    size_t offset = 0;
    while (*shape != ')')
    {
        const char *end = styxshape_end(shape);
        size_t used;
        if (!end)
        {
            return 0;
        }
        if (*shape == '(')
        {
            size_t child;
            if (!encode_nodes(encode, shape + 1, depth + 1, &child) ||
                fvm_srecord_write(STYX_CALL_TUPLE_TAG, dest + encode->capacity, child, dest + offset,
                    encode->capacity - offset, &used) != FVM_S_OK)
            {
                return 0;
            }
        }
        else if (encode->leaf == encode->layout.count ||
            !encode_leaf(encode, shape, end, dest + offset, encode->capacity - offset, &used))
        {
            return 0;
        }
        offset += used;
        shape = end;
    }
    *size = offset;
    return 1;
}


/* EN/RU: See managed_array.h. / См. managed_array.h. */
int dvm_styx_array_element_write(const char *shape, const DVM_StyxValueFrame *element,
    unsigned char *scratch, void *bytes, size_t capacity, size_t *used)
{
    FrameEncode encode = {.layout = element->layout, .slots = (const unsigned char *)element->words};
    const char *type = shape + 1;
    if (*type != '(')
    {
        return encode_leaf(&encode, type, styxshape_end(type), bytes, capacity, used);
    }
    encode.capacity = capacity;
    encode.scratch = scratch;
    size_t size;
    return scratch && encode_nodes(&encode, type + 1, 0, &size) &&
        encode.leaf == element->layout.count &&
        fvm_srecord_write(STYX_CALL_TUPLE_TAG, scratch, size, bytes, capacity, used) == FVM_S_OK;
}


/* [GC:] EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_value_frame_write(unsigned long tag, const char *shape, const void *slots,
    size_t slot_size, void *bytes, size_t capacity, size_t *used)
{
    FrameEncode encode = {.slots = slots};
    if (!styxmanaged_layout(shape, &encode.layout) || (encode.layout.size && !slots) ||
        encode.layout.size > slot_size || !bytes || !used || !capacity)
    {
        return 0;
    }
    encode.capacity = capacity < STYX_MANAGED_LIST_LIMIT ? capacity : STYX_MANAGED_LIST_LIMIT;
    encode.scratch = malloc(DVM_STYX_TUPLE_SCRATCH * encode.capacity);
    if (!encode.scratch)
    {
        return 0;
    }
    size_t size;
    int ok = encode_nodes(&encode, shape + 1, 0, &size) && encode.leaf == encode.layout.count &&
        styxmanaged_list_write(tag, shape, encode.scratch, size, bytes, capacity, used);
    free(encode.scratch);
    return ok;
}
