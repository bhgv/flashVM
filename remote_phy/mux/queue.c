/* Copyright (c) 2026 bhgv
 * EN: Allocation-free bounded ring operations, called with native hub lock held.
 * RU: Bounded ring operations без allocation под native hub lock.
 */
#include "internal.h"


/* EN/RU: See internal.h; source borrowed, exact count already bounded by caller. / См. internal.h. */
void muxput(MuxQueue *queue, size_t capacity, const void *bytes, size_t count)
{
    size_t end = (queue->first+queue->count)%capacity;
    size_t head = capacity-end < count ? capacity-end : count;
    memcpy(queue->bytes+end, bytes, head);
    memcpy(queue->bytes, (const unsigned char *)bytes+head, count-head);
    queue->count += count;
}


/* EN/RU: See internal.h; output borrowed, exact count already bounded by caller. / См. internal.h. */
void muxget(MuxQueue *queue, size_t capacity, void *bytes, size_t count)
{
    size_t head = capacity-queue->first < count ? capacity-queue->first : count;
    memcpy(bytes, queue->bytes+queue->first, head);
    memcpy((unsigned char *)bytes+head, queue->bytes, count-head);
    queue->first = (queue->first+count)%capacity;
    queue->count -= count;
}
