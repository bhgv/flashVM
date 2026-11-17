/* Copyright (c) 2026 bhgv
 * EN: Native PHY server admission, original-core owner scoping and safe wake-pin retirement.
 * RU: Native PHY server admission, original-core owner scoping и safe wake-pin retirement.
 */
#include "internal.h"


/* EN/RU: Consume one native context ref; last release occurs after server and wake users retire.
 * Поглотить native context ref; last release после server/wake users. */
void styxphywakefree(StyxPhyWake *context)
{
    if (context && __atomic_sub_fetch(&context->references, 1, __ATOMIC_ACQ_REL) == 0)
    {
        phyfree(context->phy);
        free(context->receive);
        free(context->output);
        free(context->lengths);
        free(context);
    }
}


/* EN/RU: See server.h; callback capability survives server end without retaining its shell/VM pointers.
 * См. server.h; callback capability живёт после server end без удержания shell/VM pointers. */
StyxPhyWake *styxphywakepin(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context)
    {
        return nil;
    }
    size_t refs = __atomic_load_n(&context->references, __ATOMIC_RELAXED);
    while (refs && refs != (size_t)-1)
    {
        if (__atomic_compare_exchange_n(&context->references, &refs, refs+1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED))
        {
            return context;
        }
    }
    return nil;
}


/* EN/RU: See server.h; no mutable server state or guest callback is accessed on a producer.
 * См. server.h; producer не касается mutable server state/guest callback. */
void styxphywake(StyxPhyWake *context)
{
    if (context)
    {
        phywake(context->phy);
    }
}


/* EN/RU: Legacy constructor cannot silently create a network listener in the PHY backend.
 * Legacy constructor не создаёт network listener в PHY backend. */
char *styxinit(Styxserver *server, Styxops *ops, char *port, int mode, int needfile)
{
    USED(server);
    USED(ops);
    USED(port);
    USED(mode);
    USED(needfile);
    return "use styxphyinit with an explicitly granted carrier";
}


/* EN/RU: See server.h; prepare all owned buffers before invoking unchanged original initialization.
 * См. server.h; подготовить buffers до unchanged original initialization. */
char *styxphyinit(Styxserver *server, Styxops *ops, RemotePhy *phy,
    int permissions, int needfile, unsigned queued_frames)
{
    PhyInfo info;
    if (!server || !ops || server->ftab || server->priv || queued_frames < 1 || queued_frames > 128 ||
        !phyinfo(phy, &info) || !info.connected ||
        (info.capabilities & (PHY_STREAM|PHY_ORDERED|PHY_RELIABLE)) != (PHY_STREAM|PHY_ORDERED|PHY_RELIABLE))
    {
        phyfree(phy);
        return "invalid PHY server admission";
    }
    StyxPhyWake *context = calloc(1, sizeof(*context));
    if (!context)
    {
        phyfree(phy);
        return Enomem;
    }
    context->references = 1;
    context->phy = phy;
    context->wanted = 4;
    context->slots = queued_frames;
    context->receive = malloc(MSGMAX);
    context->output = malloc((size_t)queued_frames*MSGMAX);
    context->lengths = calloc(queued_frames, sizeof(*context->lengths));
    if (!context->receive || !context->output || !context->lengths)
    {
        styxphywakefree(context);
        return Enomem;
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    char *error = styxcoreinit(server, ops, "PHY", permissions, needfile);
    styxphyowner = previous;
    if (error)
    {
        styxphywakefree(context);
    }
    return error;
}


/* EN/RU: Original process API scopes owner-local close tokens; hardware output remains nonblocking.
 * Исходный process API задаёт owner-local close context; hardware output nonblocking. */
char *styxprocess(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context)
    {
        return "PHY server is not initialized";
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    context->processing = 1;
    char *error = styxcoreprocess(server);
    context->processing = 0;
    styxphyoutput(context);
    styxphyowner = previous;
    return error ? error : context->failed ? "PHY server carrier failed" : nil;
}


/* EN/RU: Original end API retires deferred reads/fids/clients once, then releases native context owner.
 * Native producer pins remain wakeable after disconnect until their own release.
 * Исходный end API завершает reads/fids/clients один раз; producer pins остаются wakeable до release. */
char *styxend(Styxserver *server)
{
    if (!server)
    {
        return nil;
    }
    StyxPhyWake *context = server->priv;
    if (!context)
    {
        return nil;
    }
    StyxPhyWake *previous = styxphyowner;
    styxphyowner = context;
    if (server->ftab)
    {
        styxendserver(server);
    }
    phydisconnect(context->phy);
    server->priv = nil;
    styxphyowner = previous;
    styxphywakefree(context);
    return nil;
}


/* EN/RU: See server.h; no driver or protocol operation occurs during this native quota query.
 * См. server.h; без driver/protocol operation при native quota query. */
unsigned styxphyspace(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    if (!context || context->failed)
    {
        return 0;
    }
    unsigned reserve = context->processing || context->complete ? 1 : 0;
    unsigned free = context->slots-context->queued;
    return free > reserve ? free-reserve : 0;
}


/* EN/RU: See server.h; sum bounded reply extents minus only the head's confirmed prefix.
 * См. server.h; сумма bounded reply extents минус confirmed prefix head. */
size_t styxphypending(Styxserver *server)
{
    StyxPhyWake *context = server ? server->priv : nil;
    size_t pending = 0;
    if (context)
    {
        for (unsigned i = 0; i < context->queued; i++)
        {
            pending += context->lengths[(context->first+i)%context->slots];
        }
        pending -= context->sent;
    }
    return pending;
}
