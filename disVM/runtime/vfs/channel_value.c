/* Copyright (c) 2026 bhgv
 * EN: Verify channel element layouts and reuse the managed call value codec.
 * RU: Проверка раскладки элементов канала с повторным использованием кодека вызовов.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runt.h"
#include "channel_value.h"
#include "managed_ref.h"
#include "root_registry.h"
#include <string.h>

extern void movb(void);
extern void movw(void);
extern void movf(void);
extern void movl(void);
extern void movm(void);


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_shape(const char *type, char shape[STYX_SHAPE_MAX + 1])
{
    if (!type || type[0] != 'c' || type[1] != '[')
    {
        return 0;
    }
    const char *end = styxshape_end(type + 2);
    if (!end || *end != ']' || end - type >= STYX_SHAPE_MAX)
    {
        return 0;
    }
    size_t size = (size_t)(end - type - 2);
    shape[0] = '(';
    memcpy(shape + 1, type + 2, size);
    shape[size + 1] = ')';
    shape[size + 2] = 0;
    return dvm_styx_value_shape_supported(shape);
}


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_mover(const Channel *channel, const char *shape)
{
    if (!channel || channel == H || D2H(channel)->t != &Tchannel)
    {
        return 0;
    }
    switch (shape[1])
    {
    case 'i': return channel->mover == movw;
    case 'b': return channel->mover == movl;
    case 'f': return channel->mover == movf;
    case 'y': return channel->mover == movb;
    case 's':
    case 'a':
    case 'l':
    case 'c':
    case 'r':
    case 'h':
        return channel->mover == movp;
    case '(':
        break;
    default:
        return 0;
    }
    StyxScalarLayout layout;
    if (!styxmanaged_layout(shape, &layout))
    {
        return 0;
    }
    if (channel->mover == movm)
    {
        return !layout.pointers && channel->mid.w == (WORD)layout.size;
    }
    const Type *type = channel->mover == movtmp ? channel->mid.t : NULL;
    if (!type || type == H || type->size != (int)layout.size || type->np < 0)
    {
        return 0;
    }
    unsigned char map = 0;
    for (size_t i = 0; i < layout.count; i++)
    {
        if (layout.pointers & (1U << i))
        {
            map |= 0x80U >> (layout.offset[i] / sizeof(WORD));
        }
    }
    if ((type->np ? type->map[0] : 0) != map)
    {
        return 0;
    }
    for (int i = 1; i < type->np; i++)
    {
        if (type->map[i])
        {
            return 0;
        }
    }
    return 1;
}


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_empty(const char *shape, DVM_StyxValueFrame *frame)
{
    *frame = (DVM_StyxValueFrame){0};
    if (!styxmanaged_layout(shape, &frame->layout) || frame->layout.size > sizeof(frame->words))
    {
        return 0;
    }
    void *nil_value = H;
    for (size_t i = 0; i < frame->layout.count; i++)
    {
        if (frame->layout.pointers & (1U << i))
        {
            memcpy((unsigned char *)frame->words + frame->layout.offset[i], &nil_value, sizeof(nil_value));
        }
    }
    return 1;
}


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_root(DVM_StyxValueFrame *frame)
{
    for (size_t i = 0; i < frame->layout.count; i++)
    {
        if (frame->layout.pointers & (1U << i))
        {
            void *object;
            memcpy(&object, (unsigned char *)frame->words + frame->layout.offset[i], sizeof(object));
            if (object != H)
            {
                if (!object || !dvm_runtime_root_add(D2H(object)))
                {
                    return 0;
                }
                Setmark(D2H(object));
                frame->roots |= 1U << i;
            }
        }
    }
    return 1;
}


/* [GC:] EN: Hold partial graph owners outside the protected encoder invocation.
 * RU: Удержать частично закодированные объекты вне защищённого вызова кодека. */
typedef struct ChannelValueEncoding
{
    const char *shape; /* Borrowed checked frame schema. */
    const DVM_StyxValueFrame *frame; /* Borrowed rooted element. */
    DVM_StyxRefTable *refs; /* Borrowed graph ledger, retained after a VM error. */
    void *bytes; /* Borrowed destination buffer. */
    size_t capacity; /* Extent of bytes. */
    size_t *used; /* Borrowed output count. */
} ChannelValueEncoding;


/* [GC:] EN: Encode the borrowed value using the externally owned graph ledger.
 * RU: Кодировать заимствованное значение с журналом объектов вызывающего. */
static int pack_value(void *state)
{
    ChannelValueEncoding *encoding = state;
    return dvm_styx_graph_write(encoding->refs, STYX_MANAGED_GRAPH_TAG, encoding->shape,
        encoding->frame->words, encoding->frame->layout.size, encoding->bytes,
        encoding->capacity, encoding->used);
}


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_pack(const char *shape, const DVM_StyxValueFrame *frame,
    DVM_CallChannels *channels, void *bytes, size_t capacity, size_t *used)
{
    DVM_StyxRefTable refs = {.result = 1, .limit = STYX_CALL_REF_KEYS, .channels = channels};
    ChannelValueEncoding encoding = {shape, frame, &refs, bytes, capacity, used};
    int valid = kprotect(pack_value, &encoding);
    dvm_styx_ref_release(&refs, 0);
    return valid;
}


/* EN/RU: See channel_value.h. / См. channel_value.h. */
int dvm_channel_value_unpack(const char *shape, const void *bytes, size_t size,
    DVM_CallChannels *channels, DVM_StyxValueFrame *frame)
{
    FVM_SRecord record;
    DVM_StyxRefTable refs = {.result = 1, .limit = STYX_CALL_REF_KEYS, .channels = channels};
    size_t used;
    int valid = fvm_srecord_read(bytes, size, STYX_MANAGED_LIST_LIMIT, &record, &used) == FVM_S_OK &&
        used == size && styxmanaged_channel_value(shape, &record, NULL, NULL,
            channels && (channels->persistent || channels->message_owner)) &&
        dvm_styx_result_fields_read(&refs, shape, record.payload, record.size, frame);
    dvm_styx_ref_release(&refs, 0);
    return valid;
}


/* EN: Check the borrowed schema range, allowing channels in value arrays and tuples.
 * RU: Проверить заимствованный диапазон схемы, допустив каналы в массивах и кортежах. */
static int channel_positions_supported(const char *type, const char *limit)
{
    while (type < limit)
    {
        if (*type == '(' || *type == ')')
        {
            type++;
            continue;
        }
        const char *end = styxshape_end(type);
        char element[STYX_SHAPE_MAX + 1];
        if (!end || end > limit)
        {
            return 0;
        }
        if (*type == 'c' && !dvm_channel_value_shape(type, element))
        {
            return 0;
        }
        if (*type == 'a' && !channel_positions_supported(type + 2, end - 1))
        {
            return 0;
        }
        if (*type == 'r' && memchr(type, 'c', (size_t)(end - type)))
        {
            return 0;
        }
        type = end;
    }
    return 1;
}


/* EN/RU: See managed_value.h. / См. managed_value.h. */
int dvm_styx_channel_shape_supported(const StyxCallShape *shape)
{
    return styxmanaged_channel_compatible(shape) &&
        channel_positions_supported(shape->arguments, shape->arguments + strlen(shape->arguments)) &&
        channel_positions_supported(shape->result, shape->result + strlen(shape->result));
}
