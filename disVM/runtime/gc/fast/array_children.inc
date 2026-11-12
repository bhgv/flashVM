/* Copyright (c) 2026 bhgv
 * EN: Guarded array child transfer.
 * RU: Передача дочерних ссылок массива под защитой полей.
 * Private composition fragment; see ../common/RC-SOURCE.md.
 */


/* EN: Stage the children of array heap before claiming its last owner.
 * retry reports temporary packet memory/queue pressure or a lost last claim.
 * Refusal preserves the array and every original child occurrence.
 * RU: Подготовить ссылки массива heap до снятия последнего владельца.
 * retry сообщает о нехватке памяти/места пакета либо проигранном захвате.
 * При отказе массив и все исходные дочерние ссылки сохраняются. */
static int transfer_array_children(Heap *heap, int *retry)
{
    Array *array = H2D(Array *, heap);
    Type *type = array->t;
    if (!frames || !leaf_has_sole_owner(heap, retry))
    {
        return 0;
    }
    size_t capacity = 1;
    if (array->root == H)
    {
        if (array->len < 0 || type->size <= 0 || type->np <= 0 ||
            (size_t)type->np > ((size_t)type->size / sizeof(void *) + 7) / 8)
        {
            return 0;
        }
        size_t mapped = 0;
        for (int byte = 0; byte < type->np; byte++)
        {
            mapped += __builtin_popcount((unsigned char)type->map[byte]);
        }
        size_t available = (SIZE_MAX - sizeof(FrameRetirement)) / sizeof(void *);
        if (mapped && (size_t)array->len > available / mapped)
        {
            return 0;
        }
        capacity = (size_t)array->len * mapped;
    }
    if (!capacity)
    {
        return claim_childless_leaf(heap, retry);
    }
    size_t storage = sizeof(FrameRetirement) + capacity * sizeof(void *);
    FrameRetirement *packet = malloc(storage);
    if (!packet)
    {
        *retry = 1;
        return 0;
    }
    packet->roots = NULL;
    packet->count = 0;
    packet->next = 0;
    int valid = 1;
    if (array->root != H)
    {
        if (!array->root)
        {
            valid = 0;
        }
        else
        {
            packet->keys[packet->count++] = D2H(array->root);
        }
    }
    else
    {
        for (WORD index = 0; valid && index < array->len; index++)
        {
            const uchar *element = array->data + (size_t)index * type->size;
            for (int byte = 0; valid && byte < type->np; byte++)
            {
                for (unsigned bit = 0; bit < 8; bit++)
                {
                    if (type->map[byte] & (1U << bit))
                    {
                        size_t slot = (size_t)byte * 8 + 7 - bit;
                        void *child;
                        if (slot >= (size_t)type->size / sizeof(child))
                        {
                            valid = 0;
                            break;
                        }
                        memcpy(&child, element + slot * sizeof(child), sizeof(child));
                        if (!child)
                        {
                            valid = 0;
                            break;
                        }
                        if (child != H)
                        {
                            packet->keys[packet->count++] = D2H(child);
                        }
                    }
                }
            }
        }
    }
    if (valid && !packet->count)
    {
        free(packet);
        return claim_childless_leaf(heap, retry);
    }
    if (valid)
    {
        packet->roots = dvm_runtime_root_batch_create(packet->count);
        *retry = packet->roots == NULL;
        valid = packet->roots != NULL;
    }
    unsigned long charge = 0;
    valid = valid && dvm_gc_packet_charge(packet->keys, packet->count, storage, ULONG_MAX, &charge);
    DVM_RetirementReservation token = {0};
    DVM_RetirementResult reservation = valid ? dvm_retirement_reserve(frames, charge, &token) : DVM_RETIRE_INVALID;
    *retry |= reservation == DVM_RETIRE_FULL || reservation == DVM_RETIRE_BYTES;
    int reserved = reservation == DVM_RETIRE_OK;
    int rooted = reserved && publish_release_roots(packet->roots, packet->keys, packet->count);
    if (rooted && dvm_refcount_claim_last_u(&heap->ref))
    {
        if (!dvm_retirement_commit(frames, &token, packet))
        {
            abort();
        }
        return 1;
    }
    *retry |= rooted;
    if (rooted && !dvm_runtime_root_batch_clear(packet->roots))
    {
        abort();
    }
    if (reserved)
    {
        dvm_retirement_cancel(frames, &token);
    }
    if (packet->roots)
    {
        dvm_runtime_root_batch_destroy(&packet->roots);
    }
    free(packet);
    return 0;
}
