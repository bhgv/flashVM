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
        for (unsigned i = 0; i < hub->lanes; i++)
        {
            free(hub->channels[i].rx.bytes);
            free(hub->channels[i].tx.bytes);
        }
        pthread_mutex_destroy(&hub->lock);
        free(hub);
    }
}


/* EN/RU: See bidir.h; select the original wire profile. / См. bidir.h, исходный профиль. */
PhyBidir *phybidircreate(RemotePhy *carrier, size_t capacity)
{
    return phybidircreate_lanes(carrier, capacity, PHY_BIDIR_LANES);
}


/* EN/RU: See bidir.h; initialize queues before the worker. / См. bidir.h, очереди готовы до потока. */
PhyBidir *phybidircreate_lanes(RemotePhy *carrier, size_t capacity, unsigned lanes)
{
    PhyInfo info;
    if (lanes < PHY_BIDIR_LANES || lanes > PHY_BIDIR_MAX_LANES ||
        capacity < MUX_PAYLOAD || capacity > 1048576 || !phyinfo(carrier, &info) || !info.connected ||
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
    hub->lanes = lanes;
    hub->refs = 1;
    if (pthread_mutex_init(&hub->lock, NULL))
    {
        phyfree(carrier);
        free(hub);
        return NULL;
    }
    for (unsigned i = 0; i < lanes; i++)
    {
        hub->channels[i].hub = hub;
        hub->channels[i].id = i;
        hub->channels[i].grant = i < PHY_BIDIR_LANES ? capacity : 0;
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
    if (!hub || id >= hub->lanes)
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
            if (id >= PHY_BIDIR_LANES)
            {
                hub->channels[id].grant = hub->capacity;
            }
        }
    }
    pthread_mutex_unlock(&hub->lock);
    if (phy && id >= PHY_BIDIR_LANES)
    {
        phywake(hub->carrier);
    }
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
