/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual childless last-claim loss with a short-lived independent owner.
 * RU: Настоящий проигрыш захвата без детей при кратковременном внешнем владельце.
 */


/* [GC:] EN: Execute the original atomic claim on independently live heap.
 * For the selected childless case acquire a real reference/root immediately
 * before the claim, then withdraw that owner before returning the actual result.
 * This forces a failed claim followed by count==1 without forging any result.
 * All payload stays unchanged; other heaps/attempts use the original claim.
 * RU: Выполнить исходный атомарный захват живого heap. Для выбранного сценария
 * без детей добавить настоящую ссылку/корень перед захватом и снять их до
 * возврата настоящего результата. Получить отказ и затем count==1 без подмены
 * ответа. Поля не меняются; прочие heap/попытки используют исходный захват. */
int dvm_test_empty_leaf_claim(Heap *heap)
{
    if (!EmptyRace || !selected || heap != target || races)
    {
        return dvm_refcount_claim_last_u(&heap->ref);
    }
    require(dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1,
            "childless claim starts with the real sole owner and root");
    require(dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "actual childless competing reference and root");
    int claimed = dvm_refcount_claim_last_u(&heap->ref);
    require(!claimed && dvm_refcount_load_u(&heap->ref) == 2 && dvm_runtime_root_count(heap) == 2,
            "actual atomic childless claim loses to the independent owner");
    require(dvm_runtime_root_remove(heap) && dvm_refcount_drop_nonfinal_u(&heap->ref) &&
                dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1,
            "childless competitor disappears before the later count sample");
    pthread_mutex_lock(&sync);
    races++;
    pthread_mutex_unlock(&sync);
    failure_seen = 1;
    return claimed;
}


/* [GC:] EN: Sample the original count while a real temporary owner exists,
 * then remove that owner before returning the actual eligibility result.
 * Other heap/attempts forward the original load. No payload/count is forged.
 * RU: Прочитать исходный счётчик при настоящем временном владельце, снять его
 * до возврата настоящего результата проверки. Прочие heap/попытки используют
 * исходное чтение; поля/счётчики не подменяются. */
int dvm_test_leaf_sole_owner(Heap *heap)
{
    if (!EligibilityRace || !selected || heap != target || races)
    {
        return dvm_refcount_load_u(&heap->ref) == 1;
    }
    require(dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1 &&
                dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "actual independent owner during preliminary count sample");
    int sole = dvm_refcount_load_u(&heap->ref) == 1;
    require(!sole && dvm_refcount_load_u(&heap->ref) == 2, "actual preliminary count observes the competing owner");
    require(dvm_runtime_root_remove(heap) && dvm_refcount_drop_nonfinal_u(&heap->ref) &&
                dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1,
            "preliminary competitor disappears before the caller samples again");
    pthread_mutex_lock(&sync);
    races++;
    pthread_mutex_unlock(&sync);
    failure_seen = 1;
    return sole;
}
