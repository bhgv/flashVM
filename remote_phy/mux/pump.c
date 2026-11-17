/* Copyright (c) 2026 bhgv
 * EN: Single native physical I/O owner, round-robin frames and fail-closed P9MX v1 decoding.
 * RU: Один native physical I/O owner, round-robin frames и fail-closed decoding P9MX v1.
 */
#include "internal.h"


/* EN/RU: Select one bounded frame under hub lock; data copied before caller releases lock.
 * Выбрать bounded frame под lock; copy данных до unlock; 0 означает no pending output. */
static size_t output(PhyMux *hub, unsigned char *frame)
{
    for (unsigned step = 0; step < 2; step++)
    {
        unsigned id = (hub->turn+step)%2;
        MuxChannel *channel = &hub->channels[id];
        int closing = channel->closed && !channel->sentclose;
        int granting = channel->grant && !channel->closed && !channel->remote;
        size_t count = channel->tx.count < channel->credit ? channel->tx.count : channel->credit;
        if (closing || granting || (count && !channel->remote))
        {
            count = closing ? 0 : granting ? 4 : count < MUX_PAYLOAD ? count : MUX_PAYLOAD;
            memcpy(frame, "P9MX", 4);
            frame[4] = 1;
            frame[5] = closing ? 1 : granting ? 2 : 0;
            frame[6] = id;
            frame[7] = 0;
            for (unsigned i = 0; i < 4; i++)
            {
                frame[8+i] = (unsigned char)(count >> (i*8));
            }
            if (granting && !closing)
            {
                for (unsigned i = 0; i < 4; i++)
                {
                    frame[MUX_HEADER+i] = (unsigned char)(channel->grant >> (i*8));
                }
                channel->allowance += channel->grant;
                channel->grant = 0;
            }
            else if (!closing)
            {
                muxget(&channel->tx, hub->capacity, frame+MUX_HEADER, count);
                channel->credit -= count;
            }
            channel->sentclose |= closing;
            hub->turn = 1-id;
            muxsignal(hub);
            return MUX_HEADER+count;
        }
    }
    return 0;
}


/* EN/RU: Validate complete header before reading its bounded payload; wanted output borrowed.
 * Проверить complete header до payload; wanted output borrowed, 1 valid/0 malformed. */
static int header(const unsigned char *frame, size_t *wanted)
{
    size_t count = 0;
    for (unsigned i = 0; i < 4; i++)
    {
        count |= (size_t)frame[8+i] << (i*8);
    }
    if (memcmp(frame, "P9MX", 4) || frame[4] != 1 || frame[5] > 2 || frame[6] > 1 || frame[7] ||
        count > MUX_PAYLOAD || (frame[5] == 0 && count == 0) || (frame[5] == 1 && count != 0) || (frame[5] == 2 && count != 4))
    {
        return 0;
    }
    *wanted = MUX_HEADER+count;
    return 1;
}


/* EN/RU: Publish one complete incoming frame under lock; 1 accepted/-1 invalid sequence or credit violation.
 * Опубликовать complete frame под lock; 1 accepted/-1 malformed sequence/credit. */
static int incoming(PhyMux *hub, const unsigned char *frame, size_t size)
{
    MuxChannel *channel = &hub->channels[frame[6]];
    if (channel->remote)
    {
        return -1;
    }
    if (frame[5] == 1)
    {
        channel->remote = 1;
        channel->tx.count = 0;
    }
    else if (frame[5] == 2)
    {
        size_t credit = 0;
        for (unsigned i = 0; i < 4; i++)
        {
            credit |= (size_t)frame[MUX_HEADER+i] << (i*8);
        }
        if (!credit || credit > 1048576-channel->credit)
        {
            return -1;
        }
        channel->credit += credit;
    }
    else
    {
        size_t count = size-MUX_HEADER;
        if (count > channel->allowance || count > hub->capacity-channel->rx.count)
        {
            return -1;
        }
        channel->allowance -= count;
        if (!channel->closed)
        {
            muxput(&channel->rx, hub->capacity, frame+MUX_HEADER, count);
        }
    }
    muxsignal(hub);
    return 1;
}


/* EN/RU: Any uncertain/terminal physical operation fails hub; never replay or scan for magic.
 * Любая uncertain/terminal physical operation завершает hub, без replay/resync. */
static int terminal(PhyTransfer out)
{
    return (out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN);
}


/* EN/RU: See internal.h; partial headers/payloads/output stay worker-local across waits.
 * См. internal.h; partial frames/output сохраняются на worker между wait. */
void *muxpump(void *state)
{
    PhyMux *hub = state;
    unsigned char rx[MUX_HEADER+MUX_PAYLOAD];
    unsigned char tx[MUX_HEADER+MUX_PAYLOAD];
    size_t used = 0;
    size_t wanted = MUX_HEADER;
    size_t sent = 0;
    size_t sending = 0;
    for (;;)
    {
        pthread_mutex_lock(&hub->lock);
        int stop = hub->stop;
        int accepted = used == wanted ? incoming(hub, rx, used) : 1;
        if (used == wanted && accepted == 1)
        {
            used = 0;
            wanted = MUX_HEADER;
        }
        if (!sending)
        {
            sending = output(hub, tx);
        }
        pthread_mutex_unlock(&hub->lock);
        if (stop || accepted < 0)
        {
            break;
        }
        int progressed = 0;
        if (sending)
        {
            PhyTransfer out = phywrite(hub->carrier, tx+sent, sending-sent);
            sent += out.count;
            progressed |= out.count != 0;
            if (terminal(out))
            {
                break;
            }
            if (sent == sending)
            {
                sent = sending = 0;
            }
        }
        if (used < wanted)
        {
            PhyTransfer out = phyread(hub->carrier, rx+used, wanted-used);
            used += out.count;
            progressed |= out.count != 0;
            if (used == MUX_HEADER && wanted == MUX_HEADER && !header(rx, &wanted))
            {
                break;
            }
            if (terminal(out))
            {
                /* A complete confirmed frame precedes the terminal transport result. */
                pthread_mutex_lock(&hub->lock);
                if (used == wanted)
                {
                    incoming(hub, rx, used);
                }
                pthread_mutex_unlock(&hub->lock);
                break;
            }
        }
        if (progressed)
        {
            continue;
        }
        unsigned ready;
        unsigned interests = used < wanted ? PHY_READ : 0;
        if (sending)
        {
            interests |= PHY_WRITE;
        }
        if (interests && (phywait(hub->carrier, interests, -1, &ready) != PHY_OK || (ready & PHY_FAILED)))
        {
            break;
        }
    }
    phydisconnect(hub->carrier);
    pthread_mutex_lock(&hub->lock);
    hub->failed = 1;
    muxsignal(hub);
    pthread_mutex_unlock(&hub->lock);
    return NULL;
}
