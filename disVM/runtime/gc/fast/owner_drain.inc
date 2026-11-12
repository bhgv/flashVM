/* Copyright (c) 2026 bhgv
 * EN: Bounded shared rescue of mandatory releases after optional publication failure.
 * RU: Ограниченная параллельная обработка обязательных ссылок после отказа публикации.
 */


/* [GC:] EN: Move at most budget mandatory owners to epoch-gated Fast leaves,
 * transfer a nonfinal cycle seed, or drop a nonfinal non-candidate owner. Called
 * only by the sole admitted Fast consumer while queues/world remain live. A new leaf root is published before
 * the old FIFO root is removed. Optional allocation failure rotates the unchanged
 * entry; unsupported callbacks stay queued for the existing retained path.
 * Charge every visited head, including refusal, so neither retries nor unsupported
 * objects can monopolize this turn. retry receives a pending cycle handoff;
 * return successful ownership transitions.
 * RU: Передать до budget обязательных ссылок в Fast после эпохи либо снять
 * непоследнюю ссылку не-кандидата либо передать seed цикла. Вызывается только
 * единственным допущенным Fast при живых очередях и мире. Новый корень листа публикуется до снятия старого.
 * При отказе выделения неизменённая запись перемещается в хвост; неизвестные
 * callbacks сохраняются для прежнего резервного пути. Каждый просмотр расходует
 * бюджет, включая отказ. retry отмечает ожидающую передачу Cycles. Вернуть число
 * завершённых передач владения.
 */
static unsigned int drain_shared_owners(size_t budget, int *retry)
{
    if (!budget)
    {
        return 0;
    }
    DVM_RetirementStats pending = {0};
    (void)dvm_retirement_statistics(retirement, &pending);
    if (budget > pending.ready)
    {
        budget = pending.ready;
    }
    unsigned int completed = 0;
    while (budget--)
    {
        void *value = dvm_retirement_peek(retirement);
        if (!value)
        {
            break;
        }
        Heap *heap = D2H(value);
        int candidate = record_candidate(heap);
        if (candidate && dvm_refcount_load_u(&heap->ref) > 1)
        {
            int transfer = rescue_cycle_owner(heap);
            if (transfer > 0)
            {
                completed++;
            }
            else
            {
                *retry |= transfer == 0;
                dvm_retirement_rotate(retirement);
            }
            continue;
        }
        int nonfinal = !candidate && dvm_refcount_drop_nonfinal_u(&heap->ref);
        if (!nonfinal)
        {
            require_shared_finalizer(heap);
        }
        if (!nonfinal && (!leaf_eligible(heap) || !defer_leaf(heap, poolmsize(heapmem, heap), 0)))
        {
            dvm_retirement_rotate(retirement);
            continue;
        }
        void *taken;
        unsigned long bytes;
        if (!dvm_retirement_take(retirement, &taken, &bytes) || taken != value || !dvm_runtime_root_remove(heap))
        {
            abort();
        }
        if (nonfinal)
        {
            dvm_runtime_gc_owner_lost();
            __atomic_fetch_add(&released, 1, __ATOMIC_RELAXED);
        }
        shared_mandatory_owners++;
        completed++;
    }
    return completed;
}
