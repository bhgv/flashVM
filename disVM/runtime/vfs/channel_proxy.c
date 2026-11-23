/* Copyright (c) 2026 bhgv
 * [GC:] EN: Bridge ordinary Dis send/recv and call-local UniBus S-records.
 * RU: Связь обычных send/recv Dis с S-записями UniBus текущего вызова.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "kernel.h"
#include "runt.h"
#include "raise.h"
#include "channel_route.h"
#include "channel_value.h"
#include "cancel.h"
#include "native_wait.h"
#include "runtime_gc.h"
#include "shared_bytes.h"
#include "sys_capture.h"
#include "thread_port.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <string.h>

extern int dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive);


/* EN: Write a complete request and accumulate its framed reply on one fid.
 * RU: Записать полный запрос и собрать ответ с того же fid. */
static int exchange(DVM_Provider *context, const unsigned char *bytes, size_t size,
    unsigned char *reply, size_t *received)
{
    DVM_VfsHandle fid = {0};
    if (dvm_provider_open(context, ".unibus", "rw", &fid) != FVM_OK)
    {
        return 0;
    }
    int valid = 0;
    size_t offset = 0;
    while (offset < size && !dvm_cancel_requested(dvm_cancel_current()))
    {
        DVM_IoResult sent = dvm_vfs_write_chunk(&fid, bytes + offset, size - offset);
        if (sent.status != FVM_OK || (sent.flags & DVM_IO_UNCERTAIN) ||
            !sent.count || sent.count > size - offset)
        {
            break;
        }
        offset += sent.count;
    }
    size_t used = 0;
    while (offset == size && used < DVM_CHANNEL_BUS_WIRE &&
        !dvm_cancel_requested(dvm_cancel_current()))
    {
        DVM_IoResult got = dvm_vfs_read_chunk(&fid, reply + used, DVM_CHANNEL_BUS_WIRE - used);
        if (got.status != FVM_OK || (got.flags & DVM_IO_UNCERTAIN) || got.count > DVM_CHANNEL_BUS_WIRE - used)
        {
            break;
        }
        if (!got.count)
        {
            fvm_port_thread_sleep_us(1000);
            continue;
        }
        used += got.count;
        FVM_SRecord record;
        size_t extent;
        FVM_SResult parsed = fvm_srecord_read(reply, used, DVM_CHANNEL_BUS_WIRE, &record, &extent);
        if (parsed != FVM_S_MORE)
        {
            valid = parsed == FVM_S_OK && extent == used;
            break;
        }
    }
    dvm_vfs_close(&fid);
    *received = used;
    return valid;
}


/* EN: Exchange copied bytes under the exact route's cancellation source.
 * RU: Обменять копии байтов под токеном отмены точного маршрута. */
static void channel_rpc(DVM_Provider *context, DVM_Cancel *cancel, const unsigned char *bytes,
    size_t size, unsigned char *reply, size_t *received)
{
    DVM_VfsWait *wait;
    int native = dvm_vfs_path_enter_cancel(&wait, cancel);
    if (native < 0)
    {
        error("remote channel operation cancelled");
    }
    int valid = exchange(context, bytes, size, reply, received);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(native);
    if (cancelled || !valid)
    {
        error(cancelled ? "remote channel operation cancelled" : "remote channel operation failed");
    }
}


/* [GC:] EN: Encode one captured send element or the empty payload of a receive.
 * RU: Кодировать один зафиксированный элемент отправки или пустое тело приёма. */
static int write_choice(const DVM_ChannelSchema *schema, const DVM_ProxyGuard *guard,
    DVM_Cancel *scope, unsigned char *bytes, size_t capacity, size_t *used)
{
    unsigned char value[DVM_CHANNEL_BUS_WIRE];
    size_t size = 0;
    if (!guard->receive)
    {
        if (!strcmp(schema->shape, "(i)"))
        {
            WORD integer;
            if (dvm_runtime_bytes_snapshot(guard->operand, &integer, sizeof(integer)) ||
                integer < INT32_MIN || integer > INT32_MAX ||
                !styxscalarwrite(STYX_CALL_INT_TAG, STYX_SCALAR_VALUE, integer, value, sizeof(value), &size))
            {
                return 0;
            }
        }
        else
        {
            DVM_StyxValueFrame frame;
            int valid = dvm_channel_value_empty(schema->shape, &frame) &&
                dvm_styx_value_snapshot(guard->operand, &frame);
            if (valid)
            {
                DVM_CallChannels channels = {.message = scope};
                valid = dvm_channel_value_root(&frame) &&
                    dvm_channel_value_pack(schema->shape, &frame, &channels, value, sizeof(value), &size);
            }
            dvm_styx_value_frame_clear(&frame);
            if (!valid)
            {
                return 0;
            }
        }
    }
    return dvm_channel_bus_write(schema->key, guard->receive, value, size, bytes, capacity, used);
}


/* [GC:] EN: Apply the selected receive, using the declared mapped layout.
 * RU: Применить выбранный приём в рамках объявленной раскладки. */
static void receive_value(const DVM_ChannelSchema *schema, void *destination,
    DVM_Cancel *scope, const void *bytes, size_t size)
{
    DVM_StyxValueFrame frame = {0};
    DVM_CallChannels channels = {.message = scope};
    if (!dvm_channel_value_unpack(schema->shape, bytes, size, &channels, &frame) ||
        !dvm_styx_value_frame_move(&frame, destination, frame.layout.size))
    {
        dvm_styx_value_frame_clear(&frame);
        error("invalid remote channel value");
    }
}


/* EN: Decode one reply, validate the guard ordinal, then apply its optional value.
 * RU: Декодировать ответ, проверить номер ветки и применить её значение при наличии. */
static unsigned receive_selection(const unsigned char *bytes, size_t size,
    const DVM_ProxyGuard *guards, const DVM_ChannelSchema *const *schemas,
    const unsigned *order, unsigned count, int alt, int block, int typed, DVM_Cancel *scope)
{
    unsigned ordinal;
    FVM_SRecord record;
    int64_t legacy = 0;
    if (typed)
    {
        size_t used;
        if (fvm_srecord_read(bytes, size, DVM_CHANNEL_BUS_WIRE, &record, &used) != FVM_S_OK ||
            used != size || record.tag != DVM_CHANNEL_BUS_TYPED_REPLY || !record.size)
        {
            error("invalid remote channel reply");
        }
        ordinal = record.payload[0];
        record.payload++;
        record.size--;
    }
    else
    {
        StyxScalarKind kind;
        if (!styxscalarread(DVM_CHANNEL_BUS_REPLY, bytes, size, &kind, &legacy) ||
            kind != STYX_SCALAR_VALUE || (!alt && (legacy < INT32_MIN || legacy > INT32_MAX)))
        {
            error("remote channel operation rejected");
        }
        ordinal = alt ? (unsigned)((uint64_t)legacy >> 32) : 1;
    }
    if (!ordinal && !block && (typed ? !record.size : !legacy))
    {
        return count;
    }
    if (!ordinal || ordinal > count)
    {
        error("invalid remote channel selection");
    }
    unsigned selected = order[ordinal - 1];
    const DVM_ProxyGuard *guard = &guards[selected];
    if (guard->receive)
    {
        if (typed)
        {
            receive_value(schemas[selected], guard->operand, scope, record.payload, record.size);
        }
        else
        {
            uint32_t bits = (uint32_t)legacy;
            WORD value = bits <= INT32_MAX ? (WORD)bits : (WORD)(-1 - (int32_t)(UINT32_MAX - bits));
            dvm_sys_scalar_result(&value, guard->operand, sizeof(value));
        }
    }
    else if (typed && record.size)
    {
        error("invalid remote channel acknowledgement");
    }
    return guard->index;
}


/* EN: Submit direct or ALT guards as one operation on a single inherited route.
 * RU: Передать прямую операцию или ветки alt одной записью по общему маршруту. */
static unsigned submit(const DVM_ProxyGuard *guards, unsigned count, int alt, int block)
{
    static _Thread_local uint32_t random = 0x20342U; /* Same per-owner generator as local alt. */
    const DVM_ChannelSchema *schemas[DVM_CHANNEL_BUS_SLOTS];
    DVM_Provider *context = NULL;
    DVM_Cancel *cancel = NULL;
    unsigned order[DVM_CHANNEL_BUS_SLOTS];
    unsigned char payload[DVM_CHANNEL_BUS_WIRE], bytes[DVM_CHANNEL_BUS_WIRE];
    int typed = 0;
    size_t size = 0;
    if (!count || count > DVM_CHANNEL_BUS_SLOTS)
    {
        error("invalid remote alt guard count");
    }
    for (unsigned i = 0; i < count; i++)
    {
        DVM_Provider *found;
        DVM_Cancel *scope;
        schemas[i] = dvm_channel_route_lookup((uint32_t)guards[i].channel->mid.w, &found, &scope);
        if (!schemas[i] || (context && (found != context || scope != cancel)))
        {
            error("remote channel is outside its call");
        }
        context = found;
        cancel = scope;
        typed |= strcmp(schemas[i]->shape, "(i)") != 0;
        order[i] = i;
    }
    for (unsigned i = count; i > 1; i--)
    {
        random = random * 1103515245U + 12345U;
        unsigned selected = (random >> 8) % i;
        unsigned swap = order[i - 1];
        order[i - 1] = order[selected];
        order[selected] = swap;
    }
    for (unsigned i = 0; i < count; i++)
    {
        unsigned selected = order[i];
        size_t used;
        if (!write_choice(schemas[selected], &guards[selected], cancel, payload + size, sizeof(payload) - size, &used))
        {
            error("cannot encode remote channel operation");
        }
        size += used;
    }
    size_t sent = size;
    if (alt && fvm_srecord_write(block ? DVM_CHANNEL_BUS_ALT : DVM_CHANNEL_BUS_TRY_ALT,
        payload, size, bytes, sizeof(bytes), &sent) != FVM_S_OK)
    {
        error("cannot encode remote alt");
    }
    size_t received;
    if (alt)
    {
        channel_rpc(context, cancel, bytes, sent, payload, &received);
        return receive_selection(payload, received, guards, schemas, order, count, alt, block, typed, cancel);
    }
    channel_rpc(context, cancel, payload, sent, bytes, &received);
    return receive_selection(bytes, received, guards, schemas, order, count, alt, block, typed, cancel);
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channel_operate(Channel *channel, int receive, void *operand)
{
    DVM_ProxyGuard guard = {.channel = channel, .receive = receive, .operand = operand};
    submit(&guard, 1, 0, 1);
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
unsigned dvm_call_channel_alt(const DVM_ProxyGuard *guards, unsigned count, int block)
{
    return submit(guards, count, 1, block);
}


/* EN: Attempt one native channel match from the active caller without registering a waiter.
 * RU: Попробовать сопоставить канал у активного клиента без регистрации ожидания. */
typedef struct ChannelTransfer
{
    Channel *channel; /* Borrowed from the caller's rooted table. */
    DVM_StyxValueFrame frame; /* Owned managed staging, outside the transport. */
    int local_receives; /* Direction seen by the original channel. */
    int delivered; /* Real handoff or buffer operation completed. */
} ChannelTransfer;


/* EN: Invoke the local ALT fast path on one channel under the VM exception boundary.
 * RU: Использовать локальную ветвь ALT для одного канала под защитой исключений VM. */
static int try_transfer(void *state)
{
    ChannelTransfer *transfer = state;
    transfer->delivered = dvm_runtime_alt_try_transfer(transfer->channel, transfer->frame.words,
        transfer->local_receives);
    return 1;
}


/* EN: Try guards in submitted order and complete at most one channel operation.
 * RU: Проверить ветки в переданном порядке и выполнить не более одной операции. */
static void progress_operation(DVM_ChannelBus *bus, DVM_CallChannels *channels,
    const DVM_ChannelOperation *operation)
{
    for (unsigned i = 0; i < operation->count; i++)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) || !dvm_channel_bus_pending(bus, operation->sequence))
        {
            return;
        }
        const DVM_ChannelChoice *choice = &operation->choices[i];
        const char *shape = channels->schemas[choice->slot].shape;
        ChannelTransfer transfer = {.channel = channels->objects[choice->slot], .local_receives = choice->receive};
        int scalar = !strcmp(shape, "(i)");
        int valid = choice->receive || scalar ? dvm_channel_value_empty(shape, &transfer.frame) :
            dvm_channel_value_unpack(shape, operation->bytes + choice->offset, choice->size, channels, &transfer.frame);
        if (!choice->receive && scalar)
        {
            WORD value = choice->integer;
            memcpy(transfer.frame.words, &value, sizeof(value));
        }
        REG saved = DVM_R;
        valid = valid && kprotect(try_transfer, &transfer);
        DVM_R = saved;
        unsigned char element[DVM_CHANNEL_BUS_WIRE];
        size_t size = 0;
        int settled = !valid || transfer.delivered;
        if (transfer.delivered && choice->receive && operation->typed)
        {
            const size_t previous = channels->count;
            valid = valid && dvm_channel_value_root(&transfer.frame) &&
                dvm_channel_value_pack(shape, &transfer.frame, channels, element,
                    sizeof(element) - 16, &size);
            if (valid && channels->count != previous)
            {
                valid = dvm_channel_bus_extend(bus, channels->schemas + previous, channels->count - previous);
            }
            if (!valid)
            {
                dvm_call_channels_truncate(channels, previous);
            }
        }
        int64_t legacy = 0;
        if (settled && !operation->typed)
        {
            memcpy(&legacy, transfer.frame.words, sizeof(legacy));
            valid = valid && legacy >= INT32_MIN && legacy <= INT32_MAX;
            if (operation->alt)
            {
                legacy = ((int64_t)(i + 1) << 32) | (uint32_t)legacy;
            }
        }
        dvm_styx_value_frame_clear(&transfer.frame);
        if (settled)
        {
            dvm_channel_bus_finish(bus, operation, valid, legacy, i + 1, element, size);
            return;
        }
    }
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channel_progress(DVM_ChannelBus *bus, DVM_CallChannels *channels, int *native)
{
    DVM_ChannelOperation operation;
    if (!dvm_channel_bus_next(bus, 0, &operation))
    {
        return 1;
    }
    dvm_runtime_gc_native_enter(*native);
    uint64_t after = 0;
    for (unsigned round = 0; round < DVM_CHANNEL_BUS_SLOTS * 2 &&
        !dvm_cancel_requested(dvm_cancel_current()) &&
        dvm_channel_bus_next(bus, after, &operation); round++)
    {
        after = operation.sequence;
        progress_operation(bus, channels, &operation);
    }
    unsigned unbuffered = 0;
    for (size_t slot = 0; slot < channels->count; slot++)
    {
        if (channels->objects[slot]->buf == H)
        {
            unbuffered |= 1U << slot;
        }
    }
    dvm_channel_bus_settle(bus, after, unbuffered);
    int released = dvm_runtime_gc_native_try_leave();
    *native = released < 0 ? 0 : released;
    return released >= 0;
}
