/* Copyright (c) 2026 bhgv
 * [GC:] EN: Copy immutable Limbo lists through checked managed element records.
 * RU: Копирование неизменяемых списков Limbo через проверенные записи элементов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runt.h"
#include "managed_list.h"
#include "managed_array.h"
#include "channel_value.h"
#include <stdlib.h>
#include <string.h>


typedef struct ListDecode
{
    const char *schema;          /* Borrowed singleton element type list. */
    const unsigned char *bytes; /* Borrowed validated child records. */
    const size_t *offsets;       /* Checked record boundaries, including end. */
    size_t count;                /* Complete child count. */
    StyxScalarLayout layout;     /* Compiled native cell map for one list head. */
    Type *type;                  /* Held mapped descriptor for nodes containing pointers. */
    List *head;                  /* Owned current list prefix, built from the last item. */
    List *root;                  /* Temporary collector root, possibly on the previous head on error. */
    DVM_StyxValueFrame element;  /* Owned staged value until head installation. */
} ListDecode;


/* EN: Construct a descriptor that marks exactly the element's pointer slots.
 * RU: Создать дескриптор, отмечающий только поля указателей элемента. */
static unsigned char pointer_map(const StyxScalarLayout *layout)
{
    unsigned char map = 0;
    for (size_t i = 0; i < layout->count; ++i)
    {
        if (layout->pointers & (1U << i))
        {
            map |= 0x80U >> (layout->offset[i] / sizeof(WORD));
        }
    }
    return map;
}


/* [GC:] EN: Decode and link private nodes in reverse order. Until the next node
 * is installed, hold its tail and the prepared element through allocation.
 * RU: Декодировать и связать частные узлы в обратном порядке. Пока новый
 * узел не установлен, удерживать его хвост и подготовленный элемент. */
static int build_list(void *state)
{
    ListDecode *decode = state;
    for (size_t i = decode->count; i; --i)
    {
        size_t offset = decode->offsets[i - 1];
        if (!dvm_styx_array_element_read(decode->schema, decode->bytes + offset,
                decode->offsets[i] - offset, &decode->layout, NULL, &decode->element))
        {
            return 0;
        }
        size_t width = decode->layout.size;
        List *head = cons(width < sizeof(WORD) ? sizeof(WORD) : width, &decode->head);
        if (decode->type)
        {
            ++decode->type->ref;
            head->t = decode->type;
        }
        memcpy(head->data, decode->element.words, width);
        for (size_t j = 0; j < decode->layout.count; ++j)
        {
            if (decode->element.roots & (1U << j))
            {
                void *object;
                memcpy(&object, (unsigned char *)head->data + decode->layout.offset[j], sizeof(object));
                ptrdel(D2H(object));
            }
        }
        memset(&decode->element, 0, sizeof(decode->element));
        if (decode->root != H)
        {
            ptrdel(D2H(decode->root));
            decode->root = H;
        }
        ptradd(D2H(head));
        decode->root = head;
    }
    return 1;
}


/* [GC:] EN/RU: See managed_list.h. / См. managed_list.h. */
int dvm_styx_list_new(const char *type, const FVM_SRecord *record, void **out)
{
    char schema[STYX_SHAPE_MAX + 1];
    size_t offsets[STYX_MANAGED_LIST_ITEMS + 1] = {0};
    ListDecode decode = {.schema = schema, .bytes = record->payload, .offsets = offsets, .head = H, .root = H};
    if (record->tag != STYX_MANAGED_ARRAY_TAG || !record->size ||
        !styxmanaged_array_layout(type, schema, &decode.layout) || strpbrk(schema, "rhc"))
    {
        return 0;
    }
    while (offsets[decode.count] < record->size)
    {
        FVM_SRecord child;
        size_t used;
        size_t offset = offsets[decode.count];
        if (decode.count == STYX_MANAGED_LIST_ITEMS ||
            fvm_srecord_read(record->payload + offset, record->size - offset,
                STYX_MANAGED_LIST_LIMIT, &child, &used) != FVM_S_OK)
        {
            return 0;
        }
        offsets[++decode.count] = offset + used;
    }
    unsigned char map = pointer_map(&decode.layout);
    decode.type = map ? dtype(freeheap, (int)decode.layout.size, &map, 1) : NULL;
    if (map && !decode.type)
    {
        return 0;
    }
    int valid = kprotect(build_list, &decode);
    if (decode.root != H)
    {
        ptrdel(D2H(decode.root));
    }
    freetype(decode.type);
    if (!valid)
    {
        dvm_styx_value_frame_clear(&decode.element);
        destroy(decode.head);
        return 0;
    }
    *out = decode.head;
    return 1;
}


typedef struct ListEncode
{
    const char *schema;           /* Borrowed element type. */
    DVM_StyxValueFrame element;   /* Owned snapshot and roots of one head. */
    unsigned char *scratch;       /* Borrowed tuple scratch when the element is compound. */
    unsigned char *bytes;         /* Borrowed target for one head record. */
    size_t capacity;              /* Available output bytes. */
    size_t used;                  /* Confirmed encoded length. */
} ListEncode;


/* [GC:] EN: Root the retained element snapshots for recursive encoding.
 * RU: Удержать корни элементов при рекурсивном кодировании. */
static int encode_element(void *state)
{
    ListEncode *encode = state;
    return dvm_channel_value_root(&encode->element) &&
        dvm_styx_array_element_write(encode->schema, &encode->element, NULL,
            encode->scratch, encode->bytes, encode->capacity, &encode->used);
}


/* EN: Check the runtime head layout against the resolved element map.
 * RU: Сверить нативную раскладку головы с картой элемента из схемы. */
static int matching_head(const List *item, const StyxScalarLayout *layout, unsigned char map)
{
    const Type *type = item->t;
    if (!type)
    {
        return !map;
    }
    return type != H && type->size == (int)layout->size && (type->np == 0 ? map == 0 :
        type->np == 1 && type->map[0] == map);
}


/* [GC:] EN/RU: See managed_list.h. / См. managed_list.h. */
int dvm_styx_list_write(const char *type, void *value, void *bytes, size_t capacity, size_t *used)
{
    char schema[STYX_SHAPE_MAX + 1];
    StyxScalarLayout layout;
    if (!styxmanaged_array_layout(type, schema, &layout) || strpbrk(schema, "rhc"))
    {
        return 0;
    }
    unsigned char map = pointer_map(&layout);
    size_t regions = schema[1] == '(' ? 1 + DVM_STYX_TUPLE_SCRATCH : 1;
    unsigned char *payload = malloc(regions * capacity);
    if (!payload)
    {
        return 0;
    }
    size_t size = 0, count = 0;
    int ok = 1;
    for (List *item = value; ok && item != H; item = item->tail)
    {
        if (!item || D2H(item)->t != &Tlist || !matching_head(item, &layout, map) ||
            count++ == STYX_MANAGED_LIST_ITEMS || size == capacity)
        {
            ok = 0;
            break;
        }
        ListEncode encode = {.schema = schema, .element = {.layout = layout},
            .scratch = regions > 1 ? payload + capacity : NULL, .bytes = payload + size,
            .capacity = capacity - size};
        ok = dvm_styx_value_snapshot(item->data, &encode.element) && kprotect(encode_element, &encode);
        dvm_styx_value_frame_clear(&encode.element);
        size += encode.used;
    }
    ok = ok && count && fvm_srecord_write(STYX_MANAGED_ARRAY_TAG, payload, size, bytes, capacity, used) == FVM_S_OK;
    free(payload);
    return ok;
}
