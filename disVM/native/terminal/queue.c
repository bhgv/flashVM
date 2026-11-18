/* Copyright (c) 2026 bhgv
 * EN: Public bounded record queues and native completion waits; no wire I/O on producer/consumer.
 * RU: Public bounded record queues и native completion waits без wire I/O у producer/consumer.
 */
#include "internal.h"
#include <errno.h>


/* EN/RU: See control.h; encoding is atomic with queue publication, rejects without consuming a slot.
 * См. control.h; encoding атомарен с queue publication, отказ не занимает slot. */
int styxbidircontrolsend(StyxBidirControl *control, unsigned long tag, const void *payload, size_t size)
{
    if (!control || (!payload && size) || size >= control->limit)
    {
        return 0;
    }
    pthread_mutex_lock(&control->lock);
    int valid = !control->reason && control->outgoing.count < control->slots;
    if (valid)
    {
        unsigned slot = (control->outgoing.first+control->outgoing.count)%control->slots;
        size_t encoded;
        valid = fvm_srecord_write(tag, payload, size, control->outgoing.bytes+slot*control->extent,
            control->extent, &encoded) == FVM_S_OK;
        if (valid)
        {
            FVM_SRecord view;
            size_t used;
            valid = fvm_srecord_read(control->outgoing.bytes+slot*control->extent, encoded,
                control->limit, &view, &used) == FVM_S_OK && used == encoded;
        }
        if (valid)
        {
            control->outgoing.sizes[slot] = encoded;
            control->outgoing.count++;
            pthread_cond_broadcast(&control->changed);
        }
    }
    pthread_mutex_unlock(&control->lock);
    return valid;
}


/* EN/RU: See control.h; caller receives one copy, then RX capacity is returned to the native reader.
 * См. control.h; одна copied record, затем capacity возвращается native reader. */
int styxbidircontrolnext(StyxBidirControl *control, unsigned long *tag, void *payload, size_t capacity, size_t *size)
{
    if (!control || !tag || !size || (!payload && capacity))
    {
        return STYX_CONTROL_END;
    }
    pthread_mutex_lock(&control->lock);
    ControlQueue *queue = &control->incoming;
    int result = control->reason ? STYX_CONTROL_END : STYX_CONTROL_EMPTY;
    if (queue->count)
    {
        *size = queue->sizes[queue->first];
        result = STYX_CONTROL_SMALL;
        if (*size <= capacity)
        {
            if (*size)
            {
                memcpy(payload, queue->bytes+queue->first*control->extent, *size);
            }
            *tag = queue->tags[queue->first];
            queue->first = (queue->first+1)%control->slots;
            queue->count--;
            result = STYX_CONTROL_RECORD;
            pthread_cond_broadcast(&control->changed);
        }
    }
    pthread_mutex_unlock(&control->lock);
    return result;
}


/* EN/RU: Wait for incoming record or outgoing drain under one monotonic deadline;1 ready/0 otherwise.
 * Borrow control; incoming selects RX,0 selects TX. Never restarts on unrelated notifications.
 * Ждать RX record либо TX drain под одним deadline;1 ready/0 otherwise, inputs borrowed. */
static int waitqueue(StyxBidirControl *control, unsigned timeout, int incoming)
{
    if (!control || timeout > 120000)
    {
        return 0;
    }
    struct timespec deadline;
    if (clock_gettime(CLOCK_MONOTONIC, &deadline))
    {
        return 0;
    }
    deadline.tv_sec += timeout/1000;
    deadline.tv_nsec += (timeout%1000)*1000000L;
    deadline.tv_sec += deadline.tv_nsec/1000000000L;
    deadline.tv_nsec %= 1000000000L;
    pthread_mutex_lock(&control->lock);
    int error = 0;
    while ((incoming ? !control->incoming.count : control->outgoing.count != 0) && !control->reason && timeout && !error)
    {
        error = pthread_cond_timedwait(&control->changed, &control->lock, &deadline);
    }
    int drained = (incoming ? control->incoming.count != 0 : !control->outgoing.count && !control->reason) &&
        (!error || error == ETIMEDOUT);
    pthread_mutex_unlock(&control->lock);
    return drained;
}


/* EN/RU: See control.h; copy native lifecycle only, not peer execution state. / Копировать native lifecycle. */
int styxbidircontrolstate(StyxBidirControl *control)
{
    if (!control)
    {
        return STYX_CONTROL_READ_FAILED;
    }
    pthread_mutex_lock(&control->lock);
    int reason = control->reason;
    pthread_mutex_unlock(&control->lock);
    return reason;
}


/* EN/RU: See control.h; connection pin and association are immutable after native worker publication.
 * См. control.h; connection pin/association неизменны после публикации native workers.
 */
StyxClient *styxbidircontrolclient(StyxBidirControl *control)
{
    return control ? control->client : NULL;
}


/* EN/RU: See control.h; native receive wait never removes a queued record. / Native RX wait без consume. */
int styxbidircontrolwait(StyxBidirControl *control, unsigned timeout)
{
    return waitqueue(control, timeout, 1);
}


/* EN/RU: See control.h; drain acknowledges local 9P writes, not peer application execution.
 * См. control.h; drain подтверждает 9P writes, не peer application execution. */
int styxbidircontrolflush(StyxBidirControl *control, unsigned timeout)
{
    return waitqueue(control, timeout, 0);
}


/* EN/RU: See control.h; tap pins only the sticky native source, never the transport object.
 * См. control.h; tap удерживает sticky native source, не transport object.
 */
DVM_CancelTap *styxbidircontrolwatch(StyxBidirControl *control, void (*receive)(void *),
    void *state, void (*release)(void *))
{
    return control ? dvm_cancel_subscribe(control->retired, receive, state, release) : NULL;
}
