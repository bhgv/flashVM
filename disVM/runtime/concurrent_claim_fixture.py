# Copyright (c) 2026 bhgv
# [GC:] EN: Prepare real graph-claim probes without an unreviewed callback backlog.
# RU: Подготовить настоящие проверки захвата графа без неизвестного callback в очереди.
from concurrent_owner_fixture import expand


# [GC:] EN: Borrow root/work; return an owned generated fixture path. Disable
# only the deliberately unknown-finalizer scaffolding. Preserve real graph,
# Module counts, stale edges, allocator faults, FULL/BYTES replay and epoch/free
# observations. Quiescent-assist cases must be excluded by the driver.
# Refuse changed or repeated macro boundaries before compilation.
# RU: Заимствовать root/work; вернуть путь личного производного теста. Убрать
# только обвязку намеренно неизвестного финализатора. Сохранить граф, числа
# Module, устаревшие рёбра, отказы памяти, FULL/BYTES replay и проверки эпох/free.
# Сценарии помощи в покое исключаются драйвером. Изменённые/повторные границы
# макроса вызывают отказ до сборки.
def prepare_claim_fixture(root, work):
    source = expand(root, (root / "test_fast_claim_storage.c").read_text())
    anchor = "#define MixedPending (CONTROL >= 9)"
    if source.count(anchor) != 1:
        raise RuntimeError("unknown-finalizer claim scaffold anchor changed")
    source = source.replace(anchor, "#define MixedPending 0")
    source = "#if CONTROL == 20\n#undef CONTROL\n#define CONTROL 19\n#endif\n" + source
    source += PRUNE_PROBE
    path = work / "concurrent_claim_storage.c"
    path.write_text(source)
    return path


# [GC:] EN: Real extra queue owner under the production candidate-transfer lock.
# RU: Настоящий дополнительный владелец очереди под mutex передачи кандидатов.
PRUNE_PROBE = r"""
static int prune_owner_published; /* Fixture sync protects one actual publication. */

/* [GC:] EN: Borrow queue/seed while production holds candidate_transfer. Publish
 * one real owner for an already pinned node; the real prune must remove it and
 * invalidate root observations. Never set snapshot flags or collector results.
 * Return one only after committing the new owner, zero if not selected.
 * RU: Заимствовать queue/seed под настоящим candidate_transfer. Передать одну
 * настоящую ссылку на уже удержанный узел; обычное pruning снимет её и признает
 * наблюдения корней устаревшими. Флаги снимка и ответы GC не подменяются. */
int dvm_test_add_prune_owner(DVM_RetirementQueue *queue, void *seed)
{
    pthread_mutex_lock(&sync);
    int selected = CONTROL == 19 && !prune_owner_published && node_count == NodeCount &&
                   seed == H2D(void *, nodes[0]);
    Heap *heap = selected ? nodes[1] : NULL;
    if (!selected || dvm_retirement_contains_key(queue, keys[1]))
    {
        pthread_mutex_unlock(&sync);
        return 0;
    }
    require(dvm_runtime_root_count(heap) != 0, "actual Duke discovery pin protects extra owner");
    DVM_RetirementReservation token = {0};
    require(dvm_retirement_reserve(queue, poolmsize(heapmem, heap), &token) == DVM_RETIRE_OK,
            "reserve actual extra candidate owner");
    require(dvm_refcount_retain_u(&heap->ref) == DVM_REFCOUNT_LIVE && dvm_runtime_root_add(heap),
            "actual additional strong reference and root");
    require(dvm_retirement_commit_key(queue, &token, H2D(void *, heap), keys[1], NULL),
            "publish actual extra candidate owner");
    prune_owner_published = 1;
    fprintf(stderr, "PRUNE_OWNER_PUBLISHED actual=1\n");
    pthread_mutex_unlock(&sync);
    return 1;
}
"""
