/* Copyright (c) 2026 bhgv
 * EN: Object fields and quiet channel claims.
 * RU: Поля объектов и захват спокойных каналов.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Publish fields of the independently owned retiring heap before claiming its final count. Return
 * success; retry identifies a transient refusal. Roll back the packet if a competing retain changes the
 * claim.
 * RU: Опубликовать поля собственного освобождаемого heap до захвата последней ссылки. Вернуть успех; retry
 * означает временный отказ. При конкурирующем retain откатить пакет. */
static int transfer_object_fields(Heap *heap, int *retry)
{
    Type *type = heap->t;
    if (dvm_refcount_load_u(&heap->ref) != 1)
    {
        return 0;
    }
    void *keys[RECORD_EDGE_LIMIT];
    size_t count = 0;
    unsigned char *data = H2D(unsigned char *, heap);
    if (type == &Tlist)
    {
        List *list = H2D(List *, heap);
        if (!list->tail)
        {
            return 0;
        }
        if (list->tail != H)
        {
            keys[count++] = D2H(list->tail);
        }
        type = list->t;
        data = (unsigned char *)list->data;
    }
    for (int byte = 0; type && byte < type->np; byte++)
    {
        for (unsigned int bit = 0; bit < 8; bit++)
        {
            if (!(type->map[byte] & (0x80U >> bit)))
            {
                continue;
            }
            size_t slot = (size_t)byte * 8 + bit;
            void *child;
            if (slot >= (size_t)type->size / sizeof(child))
            {
                return 0;
            }
            memcpy(&child, data + slot * sizeof(child), sizeof(child));
            if (!child)
            {
                return 0;
            }
            if (child != H)
            {
                if (count == RECORD_EDGE_LIMIT)
                {
                    return 0;
                }
                keys[count++] = D2H(child);
            }
        }
    }
    if (!count)
    {
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(keys, count, count, &token, retry);
    if (!packet)
    {
        return 0;
    }
    if (dvm_refcount_claim_last_u(&heap->ref))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||
        !dvm_retirement_cancel(frames, &token))
    {
        abort();
    }
    free(packet);
    return 0;
}


extern int dvm_runtime_channel_buffer_busy_snapshot(Channel *channel);


extern int dvm_runtime_channel_post_close(Channel *channel);


/* EN: Claim the last owner of heap and return mapped descriptor on success.
 * retry reports packet pressure, an active buffer transfer or a competing retain.
 * RU: Захватить последнюю ссылку heap и вернуть дескриптор mapped при успехе.
 * retry сообщает о заполнении очереди, передаче буфера или новой ссылке. */
static int claim_quiet_channel(Heap *heap, Type **mapped, int *retry)
{
    Channel *channel = H2D(Channel *, heap);
    if (dvm_runtime_channel_buffer_busy_snapshot(channel))
    {
        *retry = 1;
        return 0;
    }
    Array *buffer = channel->buf;
    if (buffer == H)
    {
        int claimed = dvm_runtime_channel_claim_quiet(heap, buffer, mapped, 0);
        *retry = !claimed && (dvm_runtime_channel_buffer_busy_snapshot(channel) ||
                              dvm_refcount_load_u(&heap->ref) > 1 || dvm_runtime_channel_post_close(channel));
        return claimed;
    }
    if (!buffer)
    {
        return 0;
    }
    void *key = D2H(buffer);
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = prepare_release_packet(&key, 1, 1, &token, retry);
    if (!packet)
    {
        return 0;
    }
    if (dvm_runtime_channel_claim_quiet(heap, buffer, mapped, 0))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    if (!dvm_runtime_root_batch_clear(packet->roots) || !dvm_runtime_root_batch_destroy(&packet->roots) ||
        !dvm_retirement_cancel(frames, &token))
    {
        abort();
    }
    free(packet);
    *retry = dvm_runtime_channel_buffer_busy_snapshot(channel) || dvm_refcount_load_u(&heap->ref) > 1 ||
             dvm_runtime_channel_post_close(channel);
    return 0;
}
