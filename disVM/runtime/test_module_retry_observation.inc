/* Copyright (c) 2026 bhgv
 * [GC:] EN: Capture genuine short-lived module owners before later count samples.
 * RU: Зафиксировать настоящих кратковременных владельцев модуля до позднего чтения.
 */


/* [GC:] EN: Sample real module heap ownership while an independent reference
 * and root exist, then withdraw them before returning the actual sample result.
 * Other objects/attempts use the original load. No MP/native owner is changed.
 * RU: Прочитать настоящий счётчик heap при независимой ссылке/корне и снять их
 * до возврата настоящего результата. Прочие объекты/попытки используют исходное
 * чтение. Владение MP/native-модулем не меняется. */
int dvm_test_module_sole_owner(Heap *heap)
{
    pthread_mutex_lock(&sync);
    int selected = (CONTROL == 11 || CONTROL == 12 || CONTROL == 14 || CONTROL == 15) && heap == targets[0] &&
                   !(freed & 1) && !entry_refused;
    if (!selected)
    {
        pthread_mutex_unlock(&sync);
        return dvm_refcount_load_u(&heap->ref) == 1;
    }
    require(dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1 &&
                dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "actual preliminary module competitor owns reference and root");
    int sole = dvm_refcount_load_u(&heap->ref) == 1;
    require(!sole && dvm_runtime_root_remove(heap) && dvm_refcount_drop_nonfinal_u(&heap->ref) &&
                dvm_refcount_load_u(&heap->ref) == 1 && dvm_runtime_root_count(heap) == 1,
            "preliminary module competitor ends before later sampling");
    entry_refused = 1;
    pthread_mutex_unlock(&sync);
    return sole;
}


/* [GC:] EN: Observe production retry after an actual selected module refusal.
 * heap/retry are unmodified results. A later count one cannot erase the cause.
 * RU: Проверить production-retry после настоящего отказа выбранного модуля.
 * heap/retry не подменяются. Поздний счётчик один не должен стирать причину. */
void dvm_test_module_retry(Heap *heap, int retry)
{
    pthread_mutex_lock(&sync);
    if (heap == targets[0] && !(freed & 1) && CONTROL >= 10 && (claim_refused || entry_refused))
    {
        require(retry, "captured module owner refusal must request shared retry");
    }
    pthread_mutex_unlock(&sync);
}
