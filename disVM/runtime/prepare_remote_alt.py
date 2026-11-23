# Copyright (c) 2026 bhgv
# EN: Generate guarded proxy ALT dispatch and a mixed nonblocking probe.
# RU: Генерация прокси-диспетчера ALT и неблокирующей проверки смешанных веток.
REMOTE_ALT_DISPATCH = r"""
#ifdef DVM_TERMINAL_LAUNCH
/* EN: Probe direct local and remote guards once in randomized order for a fallback ALT.
 * RU: Один раз проверить прямые локальные и удалённые ветки ALT в случайном порядке. */
static unsigned mixed_alt(DVM_ProxyGuard *guards, unsigned count)
{
    static _Thread_local uint32_t random = 0x20342U; /* Per-resident ordering state. */
    unsigned order[DVM_CHANNEL_BUS_SLOTS];
    for (unsigned i = 0; i < count; i++)
    {
        order[i] = i;
    }
    for (unsigned i = count; i > 1; i--)
    {
        random = random * 1103515245U + 12345U;
        unsigned chosen = (random >> 8) % i;
        unsigned temporary = order[i - 1];
        order[i - 1] = order[chosen];
        order[chosen] = temporary;
    }
    for (unsigned i = 0; i < count; i++)
    {
        DVM_ProxyGuard *guard = &guards[order[i]];
        if (dvm_call_channel_is_proxy(guard->channel))
        {
            DVM_ProxyGuard probe = *guard;
            probe.index = 0;
            if (dvm_call_channel_alt(&probe, 1, 0) == 0)
            {
                return guard->index;
            }
        }
        else if (dvm_runtime_alt_try_transfer(guard->channel, guard->operand, guard->receive))
        {
            return guard->index;
        }
    }
    return count;
}


/* EN: Use one UniBus transaction for an ALT with direct proxy guards.
 * RU: Использовать одну операцию UniBus для ALT с прямыми прокси-ветками. */
static int remote_alt(Alt *alt, int sends, int receives, int block, void *result)
{
    int count = sends + receives;
    int remote = 0;
    for (int i = 0; i < count; i++)
    {
        Channel *channel;
        memmove(&channel, (uchar *)alt + offsetof(Alt, ac) +
            (uintptr)i * sizeof(Altc) + offsetof(Altc, c), sizeof(channel));
        if (D2H(channel)->t == &Tchannel)
        {
            remote += dvm_call_channel_is_proxy(channel);
        }
    }
    if (!remote)
    {
        return 0;
    }
    if (count > DVM_CHANNEL_BUS_SLOTS)
    {
        error("remote alt requires at most eight direct channel guards");
    }
    if (block && remote != count)
    {
        error("mixed remote alt requires a fallback");
    }
    DVM_ProxyGuard guards[DVM_CHANNEL_BUS_SLOTS];
    for (int i = 0; i < count; i++)
    {
        Altc *entry = (Altc *)((uchar *)alt + offsetof(Alt, ac)) + i;
        altcapture(entry, &guards[i].channel, &guards[i].operand);
        if (D2H(guards[i].channel)->t != &Tchannel ||
            (remote == count && !dvm_call_channel_is_proxy(guards[i].channel)))
        {
            error("remote alt requires direct channel guards");
        }
        guards[i].index = (unsigned)i;
        guards[i].receive = i >= sends;
    }
    WORD selected = remote == count ?
        dvm_call_channel_alt(guards, (unsigned)count, block) : mixed_alt(guards, (unsigned)count);
    const char *problem = altpublish(result, selected, NULL, 0);
    if (problem)
    {
        error((char *)problem);
    }
    return 1;
}
#endif


"""
