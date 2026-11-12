/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual retained backlog beside shared graph and packet retry.
 * RU: Настоящая резервная очередь рядом с shared-повторами графа и пакета.
 */
static Heap *retained_target;               /* Unsupported final owner in actual mandatory FIFO, under sync. */
static DVM_HeapKey retained_key;            /* Distinguishes actual allocation generation. */
static unsigned retained_calls;             /* Actual unknown callback invocations, under sync. */
static unsigned retained_freed;             /* Actual unsupported object free completion, under sync. */
static int replay_stale;                    /* Actual guarded field move caused a stale snapshot. */
static int replay_done;                     /* Real packet commit accepted the refused snapshot owners. */
static unsigned replay_refusals;            /* Actual selected FULL/BYTES injection count. */
static unsigned replay_passes;              /* Real nonzero Fast responses during pressure. */
static DVM_RetirementQueue *replay_queue;   /* Observed packet destination through worker join. */
static _Thread_local int fast_shared_call;  /* Current call belongs to the actual shared Fast consumer. */
static _Thread_local int graph_retry_seen;  /* Actual selected graph BUSY in this Fast call. */
static _Thread_local int replay_retry_seen; /* Actual replay queue pressure in this Fast call. */
static _Thread_local int replay_reserved;   /* Real replay reservation awaits commit. */


DVM_RetirementResult __real_dvm_retirement_reserve(DVM_RetirementQueue *, unsigned long, DVM_RetirementReservation *);


int __real_dvm_retirement_commit(DVM_RetirementQueue *, DVM_RetirementReservation *, void *);


/* [GC:] EN: An actual unknown callback remains on the retained path. heap/swept
 * are forwarded to freeheap only after peers leave and stopped admission begins.
 * RU: Настоящий неизвестный callback остаётся на резервном пути. heap/swept
 * передаются freeheap лишь после ухода соседей и остановленного допуска. */
static void retained_destructor(Heap *heap, int swept)
{
    pthread_mutex_lock(&sync);
    DVM_WorldState state;
    require(heap == retained_target && released &&
                (QuiescentAssist || (freed == NodeCount && native_freed == module_count)) && !retained_calls &&
                dvm_world_state(dvm_gc_runtime.world, &state) && state.stopped && !state.active,
            "unknown callback requires real retained admission after peers leave");
    retained_calls++;
    pthread_mutex_unlock(&sync);
    freeheap(heap, swept);
}


/* [GC:] EN: Move the existing first strong edge to its other mapped slot under
 * actual write guards. group/cells stay runtime-owned; no RC owner is invented.
 * RU: Перенести первое сильное ребро в другой размеченный слот под настоящими
 * write guards. group/cells принадлежат runtime; новых RC-владельцев нет. */
static int change_replay_fields(const RecordGroup *group, DVM_CellDomain *cells, int selected)
{
    if (!ReplayPressure || !selected)
    {
        return 0;
    }
    require(!replay_stale || replay_done, "stale snapshot stays in replay queue during pressure");
    if (replay_done)
    {
        return 0;
    }
    void **fields = H2D(void **, nodes[0]);
    DVM_RangeAccess access = {0};
    require(dvm_range_access_begin(cells, NULL, 0, fields, 2 * sizeof(void *), &access) == DVM_CELL_ACQUIRED,
            "actual source field write guards");
    require(fields[0] == H2D(void *, nodes[1]) && fields[1] == H && group->nodes_used == NodeCount,
            "actual old source edge and empty alternate slot");
    fields[1] = fields[0];
    fields[0] = H;
    require(dvm_range_access_end(&access), "actual field write guards ended");
    return 1;
}


/* [GC:] EN: Refuse the real replay destination reservation for eight actual
 * Fast responses. queue/bytes/token remain unchanged during injected pressure.
 * Rooted snapshot owners must remain exactly unchanged on every refusal.
 * RU: Отказать настоящему резерву возврата пакета на восемь ответов Fast.
 * queue/bytes/token при внедрённом отказе не меняются. Владельцы и корни снимка
 * должны сохраняться при каждом отказе. */
DVM_RetirementResult
__wrap_dvm_retirement_reserve(DVM_RetirementQueue *queue, unsigned long bytes, DVM_RetirementReservation *token)
{
    if (fast_shared_call && ReplayPressure && replay_stale && !replay_done)
    {
        if (!replay_queue)
        {
            replay_queue = queue;
        }
        if (queue == replay_queue && replay_passes < 8)
        {
            for (unsigned index = 0; index < identity->nodes_used; index++)
            {
                Heap *heap = D2H(identity->nodes[index]);
                require(dvm_runtime_root_count(heap) == 1 &&
                            dvm_refcount_load_u(&heap->ref) == identity->incoming[index] + (index == 0),
                        "replay pressure preserves all snapshot roots and strong references");
            }
            replay_retry_seen = 1;
            pthread_mutex_lock(&sync);
            replay_refusals++;
            pthread_mutex_unlock(&sync);
            return ReplayBytes ? DVM_RETIRE_BYTES : DVM_RETIRE_FULL;
        }
        DVM_RetirementResult result = __real_dvm_retirement_reserve(queue, bytes, token);
        replay_reserved = queue == replay_queue && result == DVM_RETIRE_OK;
        return result;
    }
    return __real_dvm_retirement_reserve(queue, bytes, token);
}


/* [GC:] EN: Observe real packet commit without changing its owner/root transfer.
 * queue/token/value are production-owned and forwarded unchanged.
 * RU: Наблюдать настоящий commit пакета без изменения передачи владельцев/корней.
 * queue/token/value принадлежат runtime и передаются неизменными. */
int __wrap_dvm_retirement_commit(DVM_RetirementQueue *queue, DVM_RetirementReservation *token, void *value)
{
    int selected = replay_reserved && queue == replay_queue;
    int result = __real_dvm_retirement_commit(queue, token, value);
    if (selected && result)
    {
        pthread_mutex_lock(&sync);
        replay_done = 1;
        pthread_mutex_unlock(&sync);
        replay_reserved = 0;
    }
    return result;
}
