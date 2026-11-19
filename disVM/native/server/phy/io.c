/* Copyright (c) 2026 bhgv
 * EN: Bound physical input to complete original frames and retain noninterleaved reply prefixes.
 * RU: Ограничить physical input полными original frames и сохранять noninterleaved reply prefixes.
 */
#include "internal.h"


/* EN/RU: See internal.h; physical prefixes may complete an original request even before terminal failure.
 * См. internal.h; physical prefix может завершить request до terminal failure. */
void styxphyreceive(StyxPhyWake *context)
{
    for (unsigned step = 0; step < 64 && !context->complete && !context->failed; step++)
    {
        PhyTransfer out = phyread(context->phy, context->receive+context->used, context->wanted-context->used);
        context->used += out.count;
        int terminal = (out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN);
        if (context->used == 4 && context->wanted == 4)
        {
            context->wanted = GBIT32(context->receive);
            if (context->wanted < 7 || context->wanted > context->capacity)
            {
                context->failed = 1;
                return;
            }
        }
        if (context->used == context->wanted)
        {
            context->complete = 1;
        }
        if (terminal)
        {
            context->failed = 1;
        }
        if (!out.count || terminal)
        {
            return;
        }
    }
}


/* EN/RU: See internal.h; only confirmed output bytes advance, no unknown-tail retry.
 * См. internal.h; продвигать только confirmed bytes, без unknown-tail retry. */
void styxphyoutput(StyxPhyWake *context)
{
    for (unsigned step = 0; step < 64 && context->queued && !context->failed; step++)
    {
        unsigned remaining = context->lengths[context->first]-context->sent;
        PhyTransfer out = phywrite(context->phy,
            context->output+context->first*context->stride+context->sent, remaining);
        context->sent += out.count;
        if ((out.flags & PHY_UNCERTAIN) || (out.status != PHY_OK && out.status != PHY_AGAIN))
        {
            context->failed = 1;
            return;
        }
        if (context->sent == context->lengths[context->first])
        {
            context->sent = 0;
            context->first = (context->first+1)%context->slots;
            context->queued--;
        }
        if (!out.count)
        {
            return;
        }
    }
}


/* EN/RU: Original core receives from our complete frame, never from a blocking hardware callback.
 * Borrow output/count; consumed bytes leave framing state ready for the next request.
 * Исходный core читает complete frame из staging, не hardware; output/count borrowed. */
int styxrecv(Styxserver *server, int token, char *bytes, int size, int flags)
{
    StyxPhyWake *context = server->priv;
    USED(flags);
    if (token != 2 || size < 0 || !context->complete)
    {
        return 0;
    }
    unsigned count = context->used < (unsigned)size ? context->used : (unsigned)size;
    memmove(bytes, context->receive, count);
    context->used -= count;
    memmove(context->receive, context->receive+count, context->used);
    if (!context->used)
    {
        context->complete = 0;
        context->wanted = 4;
    }
    return count;
}


/* EN/RU: Original encoded reply is copied into one bounded slot; no caller buffer is retained.
 * Accepted queue bytes have the same local-buffer meaning as a successful socket send.
 * Исходный encoded reply копируется в bounded slot; caller buffer не удерживается.
 * Подтверждение означает local acceptance, не delivery/peer execution. */
int styxsend(Styxserver *server, int token, char *bytes, int size, int flags)
{
    StyxPhyWake *context = server->priv;
    USED(flags);
    if (token != 2 || size <= 0 || size > MSGMAX || context->failed || context->queued == context->slots)
    {
        context->failed = 1;
        return -1;
    }
    unsigned slot = (context->first+context->queued)%context->slots;
    if (context->walk_mode)
    {
        if (!styxphywalkresponse(context, bytes, (unsigned)size))
        {
            context->failed = 1;
            return -1;
        }
        size_t length = 0;
        unsigned kind = WALK_PACKET_REPLY;
        if ((unsigned char)bytes[4] == Rerror && context->walk_cycle &&
            GBIT16((unsigned char *)bytes + 5) == context->walk_request.tag)
        {
            kind |= WALK_PACKET_CYCLE;
        }
        void *packet = walk_packet_write(kind, context->walk_tag, &context->walk_root, bytes, (size_t)size, &length);
        if (!packet || length > context->stride)
        {
            free(packet);
            context->failed = 1;
            return -1;
        }
        if (GBIT16((unsigned char *)bytes + 5) == context->walk_request.tag)
        {
            context->walk_cycle = 0;
        }
        memmove(context->output + slot * context->stride, packet, length);
        context->lengths[slot] = (unsigned)length;
        free(packet);
    }
    else
    {
        memmove(context->output + slot * context->stride, bytes, size);
        context->lengths[slot] = size;
    }
    context->queued++;
    return size;
}


/* EN/RU: See internal.h; this timer is for explicit native flush budgets only.
 * См. internal.h; timer только для explicit native flush budgets. */
double styxphytime(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now))
    {
        return -1;
    }
    return now.tv_sec*1000.0+now.tv_nsec/1000000.0;
}


/* EN/RU: See server.h; retain exact prefix across a timeout, fail permanently on uncertain effects.
 * См. server.h; exact prefix сохраняется при timeout, uncertain означает permanent failure. */
int styxphyflush(Styxserver *server, unsigned timeout)
{
    StyxPhyWake *context = server ? server->priv : nil;
    double now = styxphytime();
    if (!context || timeout > 120000 || now < 0)
    {
        return 0;
    }
    double deadline = now+timeout;
    for (;;)
    {
        styxphyoutput(context);
        if (!context->queued || context->failed)
        {
            return !context->failed;
        }
        now = styxphytime();
        if (now < 0 || now >= deadline)
        {
            return 0;
        }
        unsigned ready;
        if (phywait(context->phy, PHY_WRITE, (int)(deadline-now), &ready) != PHY_OK || (ready & PHY_FAILED))
        {
            context->failed = 1;
            return 0;
        }
        if (ready & PHY_WAKE)
        {
            return 0;
        }
    }
}
