/* Copyright (c) 2026 bhgv
 * EN: Rooted frame owner packets.
 * RU: Пакеты владельцев кадров с корнями.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* [GC:] EN: Transfer mapped references from borrowed frame/type plus optional extra. Called by the owning shared
 * guest. Refusal leaves frame intact; on success caller must unpublish the frame before polling. No
 * destructors run.
 * RU: Передать размеченные ссылки заимствованных frame/type и необязательную extra из shared-гостя владельца.
 * При отказе кадр не меняется; при успехе снять его публикацию до polling. Деструкторы не вызываются. */
int dvm_runtime_rc_transfer_frame(const void *frame, const Type *type, void *extra)
{
    DVM_HeapOwner *current = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    DVM_RetirementReservation token = {0};
    size_t capacity = extra != NULL;
    if (!frames || !dvm_native_owner_statistics(current, &state) || state.phase != DVM_HEAP_SHARED ||
        state.pending_progress || dvm_native_owner_world(current) != world ||
        dvm_native_context_object_count(dvm_native_context_current()))
    {
        return 0;
    }
    unsigned long direct = extra ? poolmsize(heapmem, D2H(extra)) : 0;
    for (int byte = 0; type && byte < type->np; byte++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            if (type->map[byte] & (0x80U >> bit))
            {
                size_t slot = (size_t)byte * 8 + bit;
                if (slot < sizeof(Frame) / sizeof(void *) || slot >= (size_t)type->size / sizeof(void *))
                {
                    return 0;
                }
                void *value;
                memcpy(&value, (const unsigned char *)frame + slot * sizeof(value), sizeof(value));
                if (!value)
                {
                    return 0;
                }
                if (value != H)
                {
                    unsigned long size = poolmsize(heapmem, D2H(value));
                    if (size > ULONG_MAX - direct)
                    {
                        return 0;
                    }
                    direct += size;
                }
                capacity++;
            }
        }
    }
    if (!capacity)
    {
        return 1;
    }
    if (capacity > (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *))
    {
        return 0;
    }
    size_t bytes = sizeof(FrameRetirement) + capacity * sizeof(void *);
    if (bytes > ULONG_MAX - direct)
    {
        return 0;
    }
    DVM_RetirementResult reserved = dvm_retirement_reserve(frames, direct + (unsigned long)bytes, &token);
    if (reserved != DVM_RETIRE_OK)
    {
        return reserved == DVM_RETIRE_FULL || reserved == DVM_RETIRE_BYTES ? -1 : 0;
    }
    FrameRetirement *packet = malloc(bytes);
    if (!packet)
    {
        dvm_retirement_cancel(frames, &token);
        return 0;
    }
    packet->roots = dvm_runtime_root_batch_create(capacity);
    packet->count = 0;
    packet->next = 0;
    int valid = 1;
    for (int byte = 0; packet->roots && valid && type && byte < type->np; byte++)
    {
        for (unsigned bit = 0; bit < 8; bit++)
        {
            if (type->map[byte] & (1U << bit))
            {
                void *value;
                size_t offset = ((size_t)byte * 8 + 7 - bit) * sizeof(value);
                memcpy(&value, (const unsigned char *)frame + offset, sizeof(value));
                if (!value)
                {
                    valid = 0;
                    break;
                }
                if (value != H)
                {
                    packet->keys[packet->count++] = D2H(value);
                }
            }
        }
    }
    if (extra)
    {
        packet->keys[packet->count++] = D2H(extra);
    }
    if (packet->roots && valid && !packet->count)
    {
        dvm_runtime_root_batch_destroy(&packet->roots);
        free(packet);
        dvm_retirement_cancel(frames, &token);
        return 1;
    }
    if (!packet->roots || !valid || !publish_release_roots(packet->roots, packet->keys, packet->count))
    {
        if (packet->roots)
        {
            dvm_runtime_root_batch_destroy(&packet->roots);
        }
        free(packet);
        dvm_retirement_cancel(frames, &token);
        return 0;
    }
    int became_ready;
    if (!dvm_retirement_commit_ready(frames, &token, packet, &became_ready))
    {
        abort();
    }
    if (became_ready && notify_ready)
    {
        notify_ready();
    }
    return 1;
}
