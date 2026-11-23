/* Copyright (c) 2026 bhgv
 * EN: Select remote Dis guards one exact UniBus route at a time.
 * RU: Выбор удалённых веток Dis по одному точному маршруту UniBus за раз.
 */


/* EN: Collect guards into route groups, keeping borrowed operands and original indices.
 * Return the number of groups. Invalid or revoked routes raise a Dis error.
 * RU: Собрать ветки по маршрутам, сохранив операнды и исходные индексы.
 * Вернуть число групп. Недоступный маршрут вызывает ошибку Dis. */
static unsigned group_guards(const DVM_ProxyGuard *guards, unsigned count,
    unsigned *groups)
{
    DVM_Cancel *scopes[DVM_CHANNEL_BUS_SLOTS];
    unsigned length = 0;
    for (unsigned i = 0; i < count; i++)
    {
        DVM_Provider *provider;
        DVM_Cancel *scope;
        const unsigned char *token;
        if (!guard_route(guards[i].channel, &provider, &scope, &token))
        {
            error("remote channel is outside its call");
        }
        unsigned slot = 0;
        while (slot < length && scopes[slot] != scope)
        {
            slot++;
        }
        if (slot == length)
        {
            scopes[length++] = scope;
        }
        groups[i] = slot;
    }
    return length;
}


/* EN: Submit one group. Return a position in guards or count on refusal.
 * RU: Отправить одну группу. Вернуть позицию в guards либо count при отказе. */
static unsigned submit_group(const DVM_ProxyGuard *guards, unsigned count,
    const unsigned *groups, unsigned group, unsigned long tag)
{
    DVM_ProxyGuard selected[DVM_CHANNEL_BUS_SLOTS];
    unsigned original[DVM_CHANNEL_BUS_SLOTS];
    unsigned length = 0;
    for (unsigned i = 0; i < count; i++)
    {
        if (groups[i] == group)
        {
            selected[length] = guards[i];
            selected[length].index = length;
            original[length++] = i;
        }
    }
    unsigned found = submit(selected, length, tag, 0);
    return found < length ? original[found] : count;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
unsigned dvm_call_channel_alt(const DVM_ProxyGuard *guards, unsigned count, int block)
{
    if (!count || count > DVM_CHANNEL_BUS_SLOTS)
    {
        error("invalid remote alt guard count");
    }
    unsigned groups[DVM_CHANNEL_BUS_SLOTS];
    unsigned length = group_guards(guards, count, groups);
    if (length == 1)
    {
        return submit(guards, count, block ? DVM_CHANNEL_BUS_ALT : DVM_CHANNEL_BUS_TRY_ALT, block);
    }

    static _Thread_local unsigned start; /* Rotate the first route across resident selections. */
    unsigned current = start++ % length;
    unsigned long tag = block ? DVM_CHANNEL_BUS_WAIT_ALT : DVM_CHANNEL_BUS_TRY_ALT;
    do
    {
        for (unsigned i = 0; i < length; i++)
        {
            unsigned selected = submit_group(guards, count, groups, (current + i) % length, tag);
            if (selected < count)
            {
                return guards[selected].index;
            }
        }
    } while (block);
    return count;
}


/* EN/RU: See channel_proxy.h. / См. channel_proxy.h. */
unsigned dvm_call_channel_alt_window(const DVM_ProxyGuard *guards, unsigned count, unsigned fallback)
{
    unsigned groups[DVM_CHANNEL_BUS_SLOTS];
    if (!count || count > DVM_CHANNEL_BUS_SLOTS)
    {
        error("invalid remote alt guard count");
    }
    group_guards(guards, count, groups);
    unsigned found = submit_group(guards, count, groups, 0, DVM_CHANNEL_BUS_WAIT_ALT);
    return found < count ? guards[found].index : fallback;
}
