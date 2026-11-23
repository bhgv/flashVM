/* Copyright (c) 2026 bhgv
 * EN: Rendezvous two pending fids on an unbuffered channel within one UniBus.
 * RU: Сопоставление двух ожидающих fid на небуферизованном канале одной UniBus.
 */

/* EN: Stage a wire reply to the retained fid without publishing its length.
 * Zero means the selected element did not fit. The bus lock protects both.
 * RU: Подготовить сетевой ответ в удерживаемом fid без публикации его длины.
 * Ноль означает, что выбранный элемент не поместился. Оба защищены замком шины. */
static size_t stage_bus_reply(BusFid *fid, int success, int64_t value,
    unsigned selected, const void *element, size_t size)
{
    size_t used = 0;
    if (!fid->request.typed)
    {
        return styxscalarwrite(DVM_CHANNEL_BUS_REPLY,
            success ? STYX_SCALAR_VALUE : STYX_SCALAR_ERROR, value, fid->reply,
            sizeof(fid->reply), &used) ? used : 0;
    }
    unsigned char outcome = success ? selected : 255;
    return fvm_srecord_write_pair(DVM_CHANNEL_BUS_TYPED_REPLY, &outcome, 1,
        success ? element : NULL, success ? size : 0, fid->reply,
        sizeof(fid->reply), &used) == FVM_S_OK ? used : 0;
}


/* EN: Pack the selected ordinal in the established int-only ALT reply format.
 * RU: Упаковать номер выбранной ветки в существующем скалярном формате ALT. */
static int64_t pair_integer(const BusFid *fid, unsigned selected, int32_t value)
{
    return fid->request.alt ? ((int64_t)selected << 32) | (uint32_t)value : value;
}


/* EN: Stage and publish both different fids with the owner's bus lock held.
 * The send element was recursively validated during admission. A typed receiver
 * selected on an int channel needs one managed frame around the same scalar.
 * RU: Подготовить и опубликовать два разных fid под замком шины владельца.
 * Элемент отправителя рекурсивно проверен при приёме. Для типизированного
 * получателя на канале int завернуть тот же скаляр в управляемый кадр. */
static void pair_replies(BusFid *sender, unsigned send, BusFid *receiver, unsigned recv,
    int scalar_channel)
{
    const DVM_ChannelChoice *choice = &sender->request.choices[send];
    const unsigned char *element = sender->request.bytes + choice->offset;
    size_t extent = choice->size;
    unsigned char managed[2 * STYX_SCALAR_RECORD_CAPACITY];
    int valid = !receiver->request.typed || !scalar_channel ||
        fvm_srecord_write(STYX_MANAGED_GRAPH_TAG, element, extent,
            managed, sizeof(managed), &extent) == FVM_S_OK;
    if (receiver->request.typed && scalar_channel)
    {
        element = managed;
    }
    size_t received = valid ? stage_bus_reply(receiver, 1, pair_integer(receiver, recv + 1, choice->integer),
        recv + 1, element, extent) : 0;
    size_t sent = received ? stage_bus_reply(sender, 1, pair_integer(sender, send + 1, choice->integer),
        send + 1, NULL, 0) : 0;
    if (!sent || !received)
    {
        sent = stage_bus_reply(sender, 0, 0, 0, NULL, 0);
        received = stage_bus_reply(receiver, 0, 0, 0, NULL, 0);
    }
    if (sent && received && !dvm_cancel_requested(dvm_cancel_current()))
    {
        sender->size = sent;
        receiver->size = received;
    }
}


/* EN: Match the first eligible guard pair in submitted order. Inputs are
 * distinct live pending fids protected by their common bus lock.
 * RU: Сопоставить первую допустимую пару веток в порядке запроса. Входы —
 * разные живые ожидающие fid под общим замком шины. */
static void pair_fids(BusFid *first, BusFid *second, unsigned unbuffered_mask)
{
    for (unsigned i = 0; i < first->request.count; i++)
    {
        const DVM_ChannelChoice *a = &first->request.choices[i];
        if (!(unbuffered_mask & (1U << a->slot)))
        {
            continue;
        }
        for (unsigned j = 0; j < second->request.count; j++)
        {
            const DVM_ChannelChoice *b = &second->request.choices[j];
            if (a->slot == b->slot && a->receive != b->receive)
            {
                int scalar = !strcmp(first->bus->schemas[a->slot].shape, "(i)");
                if (a->receive)
                {
                    pair_replies(second, j, first, i, scalar);
                }
                else
                {
                    pair_replies(first, i, second, j, scalar);
                }
                return;
            }
        }
    }
}


/* EN/RU: See channel_bus.h. / См. channel_bus.h. */
void dvm_channel_bus_settle(DVM_ChannelBus *bus, uint64_t probed_through, unsigned unbuffered_mask)
{
    if (!probed_through)
    {
        return;
    }
    BusFid *pending[DVM_CHANNEL_BUS_SLOTS * 2];
    size_t count = 0;
    fvm_port_sync_lock(bus->lock);
    for (BusFid *fid = bus->sealed ? NULL : bus->head; fid; fid = fid->next)
    {
        if (!fid->size && fid->request.sequence && fid->request.sequence <= probed_through)
        {
            size_t i = count++;
            while (i && pending[i - 1]->request.sequence > fid->request.sequence)
            {
                pending[i] = pending[i - 1];
                i--;
            }
            pending[i] = fid;
        }
    }
    for (size_t i = 0; unbuffered_mask && i < count && !dvm_cancel_requested(dvm_cancel_current()); i++)
    {
        for (size_t j = i + 1; j < count && !pending[i]->size; j++)
        {
            if (!pending[j]->size)
            {
                pair_fids(pending[i], pending[j], unbuffered_mask);
            }
        }
    }
    for (size_t i = 0; i < count && !dvm_cancel_requested(dvm_cancel_current()); i++)
    {
        BusFid *fid = pending[i];
        if (!fid->size && fid->request.immediate)
        {
            fid->size = stage_bus_reply(fid, 1, 0, 0, NULL, 0);
        }
    }
    fvm_port_sync_unlock(bus->lock);
}
