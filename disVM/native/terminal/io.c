/* Copyright (c) 2026 bhgv
 * EN: Independent native RX/watch and TX workers; precise ACKs, bounded queues and no payload dispatch.
 * RU: Независимые native RX/watch и TX workers; precise ACKs, bounded queues, без payload dispatch.
 */
#include "internal.h"


/* EN/RU: See internal.h; mutex pins active tokens across nonwaiting native cancellation signals.
 * См. internal.h; mutex удерживает active tokens во время native cancellation. */
void controlend(StyxBidirControl *control, int reason)
{
    pthread_mutex_lock(&control->lock);
    if (!control->reason)
    {
        control->reason = reason;
    }
    for (unsigned i = 0; i < 2; i++)
    {
        if (control->active[i])
        {
            styxcancelrequest(control->active[i]);
        }
    }
    pthread_cond_broadcast(&control->changed);
    pthread_mutex_unlock(&control->lock);
}


/* EN/RU: See internal.h; fresh single-operation tokens prevent stop racing token destruction/reuse.
 * См. internal.h; fresh token исключает гонку stop с destruction/reuse. */
int controlio(StyxBidirControl *control, unsigned worker, unsigned long long offset,
    void *bytes, size_t size, StyxTransfer *result)
{
    StyxCancel *cancel = styxcancelcreate();
    if (!cancel)
    {
        controlend(control, worker ? STYX_CONTROL_WRITE_FAILED : STYX_CONTROL_READ_FAILED);
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    int live = !control->reason;
    if (live)
    {
        control->active[worker] = cancel;
    }
    pthread_mutex_unlock(&control->lock);
    if (live)
    {
        if (worker)
        {
            styxclientwritecancel(control->tx, offset, bytes, size, cancel, result);
        }
        else
        {
            styxclientreadwatch(control->rx, offset, bytes, size, cancel, result);
        }
        pthread_mutex_lock(&control->lock);
        control->active[worker] = NULL;
        pthread_mutex_unlock(&control->lock);
    }
    styxcancelfree(cancel);
    return live;
}


/* EN/RU: Copy one complete CRC-verified payload under bounded queue capacity; no application callback.
 * Скопировать одну CRC-verified payload в bounded queue, без application callback. */
static int publish(StyxBidirControl *control, const FVM_SRecord *record)
{
    pthread_mutex_lock(&control->lock);
    while (!control->reason && control->incoming.count == control->slots)
    {
        pthread_cond_wait(&control->changed, &control->lock);
    }
    int live = !control->reason;
    if (live)
    {
        unsigned slot = (control->incoming.first+control->incoming.count)%control->slots;
        if (record->size)
        {
            memcpy(control->incoming.bytes+slot*control->extent, record->payload, record->size);
        }
        control->incoming.sizes[slot] = record->size;
        control->incoming.tags[slot] = record->tag;
        control->incoming.count++;
        pthread_cond_broadcast(&control->changed);
    }
    pthread_mutex_unlock(&control->lock);
    return live;
}


/* EN/RU: See internal.h; idle input uses untimed native watch, fragmented/coalesced reads preserve records.
 * См. internal.h; idle input через untimed native watch, fragments/coalescing не меняют records. */
void *controlreceive(void *opaque)
{
    StyxBidirControl *control = opaque;
    FVM_SStream decoder;
    fvm_sstream_init(&decoder, control->decode, control->extent, control->limit);
    unsigned long long offset = 0;
    for (;;)
    {
        unsigned char bytes[1024];
        StyxTransfer result;
        if (!controlio(control, 0, offset, bytes, sizeof(bytes), &result))
        {
            break;
        }
        if (!result.success || !result.confirmed || result.uncertain || result.count > sizeof(bytes))
        {
            controlend(control, STYX_CONTROL_READ_FAILED);
            break;
        }
        if (!result.count)
        {
            controlend(control, decoder.filled ? STYX_CONTROL_PROTOCOL : STYX_CONTROL_EOF);
            break;
        }
        if (result.count > 0x7fffffffffffffffULL-offset)
        {
            controlend(control, STYX_CONTROL_PROTOCOL);
            break;
        }
        offset += result.count;
        size_t position = 0;
        while (position < result.count)
        {
            FVM_SRecord record;
            size_t used = 0;
            FVM_SResult status = fvm_sstream_push(&decoder, bytes+position, result.count-position, &record, &used);
            position += used;
            if (status == FVM_S_OK)
            {
                if (!publish(control, &record))
                {
                    return NULL;
                }
                fvm_sstream_reset(&decoder);
            }
            else if (status != FVM_S_MORE || !used)
            {
                controlend(control, STYX_CONTROL_PROTOCOL);
                return NULL;
            }
        }
    }
    return NULL;
}


/* EN/RU: See internal.h; exactly one serialized writer, retaining head/prefix through every chunk.
 * См. internal.h; один writer, head/prefix удерживаются до подтверждения всех chunks. */
void *controltransmit(void *opaque)
{
    StyxBidirControl *control = opaque;
    unsigned long long offset = 0;
    for (;;)
    {
        pthread_mutex_lock(&control->lock);
        while (!control->reason && !control->outgoing.count)
        {
            pthread_cond_wait(&control->changed, &control->lock);
        }
        int ended = control->reason;
        unsigned slot = control->outgoing.first;
        size_t size = control->outgoing.sizes[slot];
        pthread_mutex_unlock(&control->lock);
        if (ended)
        {
            break;
        }
        size_t sent = 0;
        while (sent < size)
        {
            StyxTransfer result;
            if (!controlio(control, 1, offset, control->outgoing.bytes+slot*control->extent+sent, size-sent, &result))
            {
                return NULL;
            }
            if (!result.success || !result.confirmed || result.uncertain || !result.count ||
                result.count > size-sent || result.count > 0x7fffffffffffffffULL-offset)
            {
                controlend(control, STYX_CONTROL_WRITE_FAILED);
                return NULL;
            }
            sent += result.count;
            offset += result.count;
        }
        pthread_mutex_lock(&control->lock);
        control->outgoing.first = (slot+1)%control->slots;
        control->outgoing.count--;
        pthread_cond_broadcast(&control->changed);
        pthread_mutex_unlock(&control->lock);
    }
    return NULL;
}
