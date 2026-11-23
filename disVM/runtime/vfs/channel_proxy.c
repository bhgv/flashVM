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
#include "channel_proxy.h"
#include "cancel.h"
#include "native_wait.h"
#include "runtime_gc.h"
#include "runqueue.h"
#include "shared_bytes.h"
#include "sys_capture.h"
#include "thread_port.h"
#include "disvm/refcount.h"
#include "../../native/terminal/call_record.h"
#include "../../native/terminal/scalar_record.h"
#include <string.h>

static uint32_t next_key; /* Never reissue a capability in this process. */

extern void movw(void);
extern int dvm_runtime_alt_try_transfer(Channel *channel, void *payload, int receive);


/* EN: Reserved mover identity. Ordinary queue transfers cannot use a proxy.
 * RU: Маркер мувера. Обычные операции с очередью не используют прокси. */
static void proxy_mover(void)
{
    error("remote channel requires a UniBus operation");
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channel_is_proxy(Channel *channel)
{
    return channel && channel != H && channel->mover == proxy_mover;
}


/* [GC:] EN: Retain one live heap channel before inserting it in an owner table.
 * RU: Удержать живой канал перед добавлением в таблицу владельца. */
static int root_channel(Channel *channel)
{
    Heap *heap = D2H(channel);
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        return 0;
    }
    ptradd(heap);
    Setmark(heap);
    return 1;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channels_release(DVM_CallChannels *channels)
{
    while (channels->count)
    {
        Channel *object = channels->objects[--channels->count];
        ptrdel(D2H(object));
        destroy(object);
    }
}


/* EN: Allocate a positive wire identity without wraparound.
 * RU: Выделить положительный wire-идентификатор без переполнения. */
static uint32_t allocate_key(void)
{
    uint32_t key = __atomic_load_n(&next_key, __ATOMIC_RELAXED);
    while (key < INT32_MAX)
    {
        if (__atomic_compare_exchange_n(&next_key, &key, key + 1, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return key + 1;
        }
    }
    return 0;
}


/* [GC:] EN: Encode nil or a key for this admitted integer channel.
 * RU: Кодировать nil либо ключ допущенного целочисленного канала. */
int dvm_call_channel_encode(DVM_CallChannels *channels, void *value,
    void *bytes, size_t capacity, size_t *size)
{
    if (value == H)
    {
        return fvm_srecord_write(STYX_MANAGED_NIL_TAG, NULL, 0, bytes, capacity, size) == FVM_S_OK;
    }
    Channel *channel = value;
    if (!channels || !channel || D2H(channel)->t != &Tchannel || channel->mover != movw)
    {
        return 0;
    }
    size_t slot = 0;
    while (slot < channels->count && channels->objects[slot] != channel)
    {
        slot++;
    }
    if (slot == channels->count)
    {
        uint32_t key = slot < DVM_CHANNEL_BUS_SLOTS ? allocate_key() : 0;
        if (!key || !root_channel(channel))
        {
            return 0;
        }
        channels->objects[slot] = channel;
        channels->keys[slot] = key;
        channels->count++;
    }
    uint32_t key = channels->keys[slot];
    unsigned char identity[4] = {key >> 24, key >> 16, key >> 8, key};
    return fvm_srecord_write(STYX_MANAGED_CHANNEL_TAG, identity, sizeof(identity),
        bytes, capacity, size) == FVM_S_OK;
}


/* [GC:] EN: Build one local proxy; repeated keys return the same channel.
 * RU: Создать локальный прокси. Повторные ключи возвращают тот же канал. */
int dvm_call_channel_decode(DVM_CallChannels *channels, const FVM_SRecord *record, void **result)
{
    if (record->tag == STYX_MANAGED_NIL_TAG && !record->size)
    {
        *result = H;
        return 1;
    }
    if (!channels || record->tag != STYX_MANAGED_CHANNEL_TAG || record->size != 4)
    {
        return 0;
    }
    const unsigned char *id = record->payload;
    uint32_t key = (uint32_t)id[0] << 24 | (uint32_t)id[1] << 16 | (uint32_t)id[2] << 8 | id[3];
    if (!key || key > INT32_MAX)
    {
        return 0;
    }
    for (size_t i = 0; i < channels->count; i++)
    {
        if (channels->keys[i] == key)
        {
            Channel *alias = channels->objects[i];
            if (dvm_refcount_retain_u(&D2H(alias)->ref) != DVM_REFCOUNT_LIVE)
            {
                return 0;
            }
            *result = alias;
            return 1;
        }
    }
    if (channels->count == DVM_CHANNEL_BUS_SLOTS)
    {
        return 0;
    }
    Channel *proxy = cnewc(&Tword, proxy_mover, 0);
    proxy->mid.w = key;
    if (!root_channel(proxy))
    {
        destroy(proxy);
        return 0;
    }
    size_t slot = channels->count++;
    channels->objects[slot] = proxy;
    channels->keys[slot] = key;
    *result = proxy;
    return 1;
}


/* EN: Submit one complete request, poll its exact fid, and return a checked reply.
 * RU: Передать полный запрос, опрашивать его fid и вернуть проверенный ответ. */
static int exchange(DVM_Provider *context, const unsigned char *bytes, size_t size, int64_t *result)
{
    DVM_VfsHandle fid = {0};
    if (dvm_provider_open(context, ".unibus", "rw", &fid) != FVM_OK)
    {
        return 0;
    }
    DVM_IoResult sent = dvm_vfs_write_chunk(&fid, bytes, size);
    int valid = 0;
    if (sent.status == FVM_OK && sent.count == size && !(sent.flags & DVM_IO_UNCERTAIN))
    {
        unsigned char reply[DVM_CHANNEL_BUS_WIRE];
        while (!dvm_cancel_requested(dvm_cancel_current()))
        {
            DVM_IoResult got = dvm_vfs_read_chunk(&fid, reply, sizeof(reply));
            if (got.status != FVM_OK || (got.flags & DVM_IO_UNCERTAIN))
            {
                break;
            }
            if (got.count)
            {
                StyxScalarKind kind;
                valid = got.count <= sizeof(reply) && styxscalarread(DVM_CHANNEL_BUS_REPLY, reply, got.count,
                    &kind, result) && kind == STYX_SCALAR_VALUE;
                break;
            }
            fvm_port_thread_sleep_us(1000);
        }
    }
    dvm_vfs_close(&fid);
    return valid;
}


/* EN: Exchange a copied request on the active call's native reverse route.
 * RU: Обменять копию запроса по нативному встречному маршруту текущего вызова. */
static int64_t channel_rpc(const unsigned char *bytes, size_t size)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)currun();
    DVM_Provider *context = process ? process->remote_call_context : NULL;
    DVM_VfsWait *wait;
    if (!context)
    {
        error("remote channel is outside its call");
    }
    int native = dvm_vfs_path_enter(&wait);
    if (native < 0)
    {
        error("remote channel operation cancelled");
    }
    int64_t result;
    int valid = exchange(context, bytes, size, &result);
    int cancelled = dvm_vfs_wait_end(wait);
    dvm_runtime_gc_native_enter(native);
    if (cancelled || !valid)
    {
        error(cancelled ? "remote channel operation cancelled" : "remote channel operation failed");
    }
    return result;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
void dvm_call_channel_operate(Channel *channel, int receive, void *destination, int64_t value)
{
    unsigned char bytes[DVM_CHANNEL_BUS_WIRE];
    size_t size;
    if (value < INT32_MIN || value > INT32_MAX ||
        !dvm_channel_bus_write((uint32_t)channel->mid.w, receive, (int32_t)value, bytes, sizeof(bytes), &size))
    {
        error("invalid remote channel operand");
    }
    int64_t result = channel_rpc(bytes, size);
    if (result < INT32_MIN || result > INT32_MAX)
    {
        error("invalid remote channel reply");
    }
    if (receive)
    {
        WORD received = (WORD)result;
        dvm_sys_scalar_result(&received, destination, sizeof(received));
    }
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
unsigned dvm_call_channel_alt(const DVM_ProxyGuard *guards, unsigned count, int block)
{
    static _Thread_local uint32_t random = 0x20342U; /* Same per-owner generator as local alt. */
    unsigned order[DVM_CHANNEL_BUS_SLOTS];
    unsigned char payload[DVM_CHANNEL_BUS_WIRE], bytes[DVM_CHANNEL_BUS_WIRE];
    size_t size = 0;
    for (unsigned i = 0; i < count; i++)
    {
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
        const DVM_ProxyGuard *guard = &guards[order[i]];
        WORD value = 0;
        size_t used;
        if (!guard->receive && dvm_runtime_bytes_snapshot(guard->operand, &value, sizeof(value)))
        {
            error("invalid remote alt operand");
        }
        if (value < INT32_MIN || value > INT32_MAX ||
            !dvm_channel_bus_write((uint32_t)guard->channel->mid.w, guard->receive, (int32_t)value,
                payload + size, sizeof(payload) - size, &used))
        {
            error("cannot encode remote alt");
        }
        size += used;
    }
    size_t used;
    if (fvm_srecord_write(block ? DVM_CHANNEL_BUS_ALT : DVM_CHANNEL_BUS_TRY_ALT,
        payload, size, bytes, sizeof(bytes), &used) != FVM_S_OK)
    {
        error("cannot encode remote alt");
    }
    const uint64_t reply = (uint64_t)channel_rpc(bytes, used);
    if (!reply && !block)
    {
        return count;
    }
    const uint64_t selected = reply >> 32;
    if (!selected || selected > count)
    {
        error("invalid remote alt reply");
    }
    const DVM_ProxyGuard *guard = &guards[order[selected - 1]];
    if (guard->receive)
    {
        const uint32_t bits = (uint32_t)reply;
        const WORD value = bits <= INT32_MAX ? (WORD)bits : (WORD)(-1 - (int32_t)(UINT32_MAX - bits));
        dvm_sys_scalar_result(&value, guard->operand, sizeof(value));
    }
    return guard->index;
}


/* EN: Attempt one native channel match from the active caller without registering a waiter.
 * RU: Попробовать сопоставить канал у активного клиента без регистрации ожидания. */
typedef struct ChannelTransfer
{
    Channel *channel; /* Borrowed from the caller's rooted table. */
    WORD value; /* Native scalar staging. */
    int local_receives; /* Direction seen by the original channel. */
    int delivered; /* Real handoff or buffer operation completed. */
} ChannelTransfer;


/* EN: Invoke the local ALT fast path on one channel under the VM exception boundary.
 * RU: Использовать локальную ветвь ALT для одного канала под защитой исключений VM. */
static int try_transfer(void *state)
{
    ChannelTransfer *transfer = state;
    transfer->delivered = dvm_runtime_alt_try_transfer(transfer->channel, &transfer->value,
        transfer->local_receives);
    return 1;
}


/* EN: Try guards in submitted order and complete at most one channel operation.
 * RU: Проверить ветки в переданном порядке и выполнить не более одной операции. */
static void progress_operation(DVM_ChannelBus *bus, const DVM_CallChannels *channels,
    const DVM_ChannelOperation *operation)
{
    for (unsigned i = 0; i < operation->count; i++)
    {
        if (dvm_cancel_requested(dvm_cancel_current()) || !dvm_channel_bus_pending(bus, operation->sequence))
        {
            return;
        }
        const DVM_ChannelChoice *choice = &operation->choices[i];
        ChannelTransfer transfer = {.channel = channels->objects[choice->slot], .value = choice->value,
            .local_receives = choice->receive};
        REG saved = DVM_R;
        int valid = kprotect(try_transfer, &transfer);
        DVM_R = saved;
        if (!valid || transfer.delivered)
        {
            valid = valid && transfer.value >= INT32_MIN && transfer.value <= INT32_MAX;
            int64_t reply = valid ? transfer.value : 0;
            if (operation->alt)
            {
                reply = ((int64_t)(i + 1) << 32) | (uint32_t)reply;
            }
            dvm_channel_bus_finish(bus, operation, valid, reply);
            return;
        }
    }
    if (operation->immediate)
    {
        dvm_channel_bus_finish(bus, operation, 1, 0);
    }
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
int dvm_call_channel_progress(DVM_ChannelBus *bus, const DVM_CallChannels *channels, int *native)
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
    int released = dvm_runtime_gc_native_try_leave();
    *native = released < 0 ? 0 : released;
    return released >= 0;
}
