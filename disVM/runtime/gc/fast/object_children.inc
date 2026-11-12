/* Copyright (c) 2026 bhgv
 * EN: Object fields and quiet channel claims.
 * RU: Поля объектов и захват спокойных каналов.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Append borrowed value's owner to keys, or count it when keys is
 * NULL. Nil contributes no owner; reject null pointers and count overflow.
 * Store only the prefix fitting capacity, but count every occurrence. No
 * payload/reference changes; count is private scratch.
 * RU: Добавить владельца заимствованного value в keys либо посчитать при NULL
 * keys. Nil не добавляет ссылку; нулевой указатель или переполнение дают отказ.
 * Записать лишь префикс capacity, но посчитать все вхождения. Поля/ссылки
 * не меняются; count — личный рабочий счётчик. */
static int append_object_owner(void *value, void **keys, size_t capacity, size_t *count)
{
    if (!value)
    {
        return 0;
    }
    if (value != H)
    {
        if (*count == SIZE_MAX)
        {
            return 0;
        }
        if (keys && *count < capacity)
        {
            keys[*count] = D2H(value);
        }
        (*count)++;
    }
    return 1;
}


/* [GC:] EN: Count or capture every mapped owner of independently owned heap,
 * including a list tail. Caller has passed the leaf reader epoch; no competing
 * payload writer may remain. keys/capacity are private optional storage; count
 * receives all occurrences, including aliases, even beyond the stored prefix.
 * Return zero on invalid map/count overflow without changing managed ownership.
 * RU: Посчитать либо сохранить все размеченные ссылки собственного heap,
 * включая хвост списка. Эпоха читателей листа пройдена; конкурирующих писателей
 * полей быть не должно. keys/capacity — необязательная личная память; count
 * получает все вхождения, включая повторные ссылки сверх записанного префикса.
 * Ноль — неверная карта/переполнение; управляемое владение не меняется. */
static int collect_object_owners(Heap *heap, void **keys, size_t capacity, size_t *count)
{
    Type *type = heap->t;
    unsigned char *data = H2D(unsigned char *, heap);
    *count = 0;
    if (type == &Tlist)
    {
        List *list = H2D(List *, heap);
        if (!append_object_owner(list->tail, keys, capacity, count))
        {
            return 0;
        }
        type = list->t;
        data = (unsigned char *)list->data;
    }
    if (type && (type->size < 0 || type->np < 0))
    {
        return 0;
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
            if (!append_object_owner(child, keys, capacity, count))
            {
                return 0;
            }
        }
    }
    return 1;
}


/* [GC:] EN: Publish every mapped owner of independently owned heap before its
 * last-reference claim. Size private metadata by actual non-nil occurrences,
 * using local storage for small objects. retry distinguishes temporary memory
 * or packet pressure. Refusal and a competing retain roll back all new roots,
 * reservations and scratch; no payload or original owner is consumed.
 * RU: Опубликовать все размеченные ссылки собственного heap до захвата последней
 * ссылки. Размер личных данных определяется ненулевыми вхождениями; для малых
 * объектов используется стек. retry отличает временный отказ памяти/очереди.
 * При отказе или новой ссылке снять новые корни, резерв и рабочую память;
 * исходный владелец и поля не потребляются. */
static int transfer_object_fields(Heap *heap, int *retry)
{
    void *local_keys[RECORD_EDGE_LIMIT];
    size_t count;
    if (!leaf_has_sole_owner(heap, retry) || !collect_object_owners(heap, local_keys, RECORD_EDGE_LIMIT, &count))
    {
        return 0;
    }
    if (!count)
    {
        return claim_childless_leaf(heap, retry);
    }
    if (count > (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *))
    {
        return 0;
    }
    void **keys = local_keys;
    int valid = 1;
    if (count > RECORD_EDGE_LIMIT)
    {
        keys = malloc(count * sizeof(*keys));
        if (!keys)
        {
            *retry = 1;
            return 0;
        }
        size_t captured;
        valid = collect_object_owners(heap, keys, count, &captured) && captured == count;
    }
    DVM_RetirementReservation token = {0};
    FrameRetirement *packet = valid ? prepare_release_packet(keys, count, count, &token, retry) : NULL;
    if (keys != local_keys)
    {
        free(keys);
    }
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
    *retry = 1;
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


/* [GC:] EN: Claim independently live heap after its reader cutoff; mapped
 * receives an owned mover descriptor only on success. Preserve the queue-locked
 * refusal reason in retry through later snapshots and buffered packet rollback.
 * Request hosted waiter closure through owner mailboxes; do not execute peers.
 * RU: После эпохи захватить живой heap; mapped получает дескриптор лишь при
 * успехе. Сохранить причину отказа под мьютексом в retry через поздние снимки
 * и откат пакета буфера. Закрытие hosted-ожиданий передаётся владельцам через
 * почтовые ящики, без исполнения чужого Prog. */
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
        int claimed = dvm_runtime_channel_claim_observed(heap, buffer, mapped, 0, retry);
        *retry |= !claimed && (dvm_runtime_channel_buffer_busy_snapshot(channel) ||
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
    if (dvm_runtime_channel_claim_observed(heap, buffer, mapped, 0, retry))
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
    *retry |= dvm_runtime_channel_buffer_busy_snapshot(channel) || dvm_refcount_load_u(&heap->ref) > 1 ||
              dvm_runtime_channel_post_close(channel);
    return 0;
}
