/* Copyright (c) 2026 bhgv
 * EN: Explicit mux construction and pinned retirement, separate from physical drivers.
 * RU: Создание mux и pinned retirement отдельно от физических drivers.
 */
#include "internal.h"


/* EN/RU: See internal.h; final free has no live native pump/I/O/wake caller. / См. internal.h. */
void muxdrop(PhyBidir *hub)
{
    pthread_mutex_lock(&hub->lock);
    int last = --hub->refs == 0;
    pthread_mutex_unlock(&hub->lock);
    if (last)
    {
        phyfree(hub->carrier);
        for (unsigned i = 0; i < 2; i++)
        {
            free(hub->channels[i].rx.bytes);
            free(hub->channels[i].tx.bytes);
        }
        pthread_mutex_destroy(&hub->lock);
        free(hub);
    }
}


/* EN/RU: See mux.h; all buffers/synchronizers exist before publishing native worker. / См. mux.h. */
PhyBidir *phybidircreate(RemotePhy *carrier, size_t capacity)
{
    PhyInfo info;
    if (capacity < MUX_PAYLOAD || capacity > 1048576 || !phyinfo(carrier, &info) || !info.connected ||
        (info.capabilities & (PHY_STREAM|PHY_ORDERED|PHY_RELIABLE)) != (PHY_STREAM|PHY_ORDERED|PHY_RELIABLE))
    {
        phyfree(carrier);
        return NULL;
    }
    PhyBidir *hub = calloc(1, sizeof(*hub));
    if (!hub)
    {
        phyfree(carrier);
        return NULL;
    }
    hub->carrier = carrier;
    hub->capacity = capacity;
    hub->refs = 1;
    if (pthread_mutex_init(&hub->lock, NULL))
    {
        phyfree(carrier);
        free(hub);
        return NULL;
    }
    for (unsigned i = 0; i < 2; i++)
    {
        hub->channels[i].hub = hub;
        hub->channels[i].id = i;
        hub->channels[i].grant = capacity;
        hub->channels[i].rx.bytes = malloc(capacity);
        hub->channels[i].tx.bytes = malloc(capacity);
        if (!hub->channels[i].rx.bytes || !hub->channels[i].tx.bytes)
        {
            muxdrop(hub);
            return NULL;
        }
    }
    if (pthread_create(&hub->worker, NULL, muxpump, hub))
    {
        muxdrop(hub);
        return NULL;
    }
    return hub;
}


/* EN/RU: See mux.h; hub owner serializes channel acquisition/free; worker uses only locked fields. / См. mux.h. */
RemotePhy *phybidirchannel(PhyBidir *hub, unsigned id)
{
    if (!hub || id > 1)
    {
        return NULL;
    }
    pthread_mutex_lock(&hub->lock);
    RemotePhy *phy = NULL;
    if (!hub->channels[id].taken && !hub->stop && !hub->failed)
    {
        phy = muxendpoint(&hub->channels[id]);
        if (phy)
        {
            hub->channels[id].taken = 1;
            hub->refs++;
        }
    }
    pthread_mutex_unlock(&hub->lock);
    return phy;
}


/* EN/RU: See mux.h; wake before join, preserve carrier wake storage for pinned logical handles. / См. mux.h. */
void phybidirfree(PhyBidir *hub)
{
    if (hub)
    {
        pthread_mutex_lock(&hub->lock);
        hub->stop = 1;
        muxsignal(hub);
        pthread_mutex_unlock(&hub->lock);
        phywake(hub->carrier);
        pthread_join(hub->worker, NULL);
        muxdrop(hub);
    }
}
