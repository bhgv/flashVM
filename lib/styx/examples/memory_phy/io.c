/* Copyright (c) 2026 bhgv
 * EN: Nonblocking precise byte prefixes, bounded circular storage and explicit injected uncertainty.
 * RU: Nonblocking точные byte prefixes, ограниченное circular storage и явная injected uncertainty.
 * standalone-reviewed: callback attempts never wait for peer consumption or ring-lock ownership.
 */
#include "internal.h"
#include <errno.h>


/* EN: Read into borrowed bytes/size from adopted state; return confirmed prefix,
 * AGAIN on empty/contention, EOF only after a disconnected peer's bytes have drained.
 * RU: Читать в bytes/size из state; вернуть confirmed prefix, AGAIN при empty/contention,
 * EOF — только после дочитывания байтов отключившегося peer.
 */
PhyTransfer memory_read(void *state, void *bytes, size_t size)
{
    MemoryEnd *end = state;
    MemoryLink *link = end->link;
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    memory_count(&link->stats->reads, 1);
    int error = pthread_mutex_trylock(&link->lock); /* Never block an I/O callback on another native owner. */
    if (error)
    {
        out.status = error == EBUSY ? PHY_AGAIN : PHY_ERROR;
        memory_count(&link->stats->again, error == EBUSY);
        return out;
    }
    unsigned side = end->side;
    out.count = size < link->used[side] ? size : link->used[side];
    for (size_t index = 0; index < out.count; index++)
    {
        ((unsigned char *)bytes)[index] = link->bytes[side][(link->head[side] + index) % MEMORY_CAPACITY];
    }
    link->head[side] = (link->head[side] + out.count) % MEMORY_CAPACITY;
    link->used[side] -= out.count;
    out.status = out.count ? PHY_OK : link->open[1 - side] ? PHY_AGAIN : PHY_EOF;
    pthread_mutex_unlock(&link->lock);
    if (out.count)
    {
        memory_count(&link->stats->read_bytes, out.count);
        memory_notify(&link->ends[1 - side]); /* Free capacity is now persistent write readiness for the peer. */
    }
    else if (out.status == PHY_AGAIN)
    {
        memory_count(&link->stats->again, 1);
    }
    return out;
}


/* EN: Copy borrowed bytes/size into the peer ring. Return only actual accepted
 * prefixes; AGAIN means full/contention. The optional cutoff reports terminal
 * ERROR|UNCERTAIN on its boundary; this function never retries or fabricates EOF.
 * RU: Копировать bytes/size в peer ring. Возвращать реально принятый prefix;
 * AGAIN — full/contention. Optional cutoff выдаёт terminal ERROR|UNCERTAIN на
 * границе; функция не повторяет запись и не выдумывает EOF.
 */
PhyTransfer memory_write(void *state, const void *bytes, size_t size)
{
    MemoryEnd *end = state;
    MemoryLink *link = end->link;
    PhyTransfer out = {0, PHY_AGAIN, PHY_PREFIX};
    memory_count(&link->stats->writes, 1);
    int error = pthread_mutex_trylock(&link->lock);
    if (error)
    {
        out.status = error == EBUSY ? PHY_AGAIN : PHY_ERROR;
        memory_count(&link->stats->again, error == EBUSY);
        return out;
    }
    unsigned peer = 1 - end->side;
    size_t space = MEMORY_CAPACITY - link->used[peer];
    out.count = size < space ? size : space;
    if (!link->open[peer])
    {
        out.count = 0;
        out.status = PHY_ERROR; /* No bytes were accepted into a retired peer. */
    }
    else
    {
        if (end->cut_after && out.count > end->cut_after - end->written)
        {
            out.count = end->cut_after - end->written;
        }
        for (size_t index = 0; index < out.count; index++)
        {
            link->bytes[peer][(link->head[peer] + link->used[peer] + index) % MEMORY_CAPACITY] = ((const unsigned char *)bytes)[index];
        }
        link->used[peer] += out.count;
        end->written += out.count;
        out.status = out.count ? PHY_OK : PHY_AGAIN;
        if (end->cut_after && end->written == end->cut_after)
        {
            out.status = PHY_ERROR;
            out.flags |= PHY_UNCERTAIN; /* Caller retains the prefix and stops; it must not replay the whole operation. */
        }
    }
    pthread_mutex_unlock(&link->lock);
    if (out.count)
    {
        memory_count(&link->stats->write_bytes, out.count);
        memory_notify(&link->ends[peer]);
    }
    else if (out.status == PHY_AGAIN)
    {
        memory_count(&link->stats->again, 1);
    }
    return out;
}
