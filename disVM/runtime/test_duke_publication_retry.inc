/* Copyright (c) 2026 bhgv
 * [GC:] EN: Actual publication allocations, rollback and retained Duke discovery.
 * RU: Настоящие выделения публикации, откат и сохранение завершённого обхода Duke.
 */
static unsigned publication_refusals;           /* Selected allocation failures; sole Duke producer. */
static unsigned publication_passes;             /* Actual positive Duke responses, under sync. */
static unsigned publication_captures;           /* Complete final-source discoveries. */
static unsigned publication_success;            /* Final commit attempted after successful epoch, under sync. */
static int publication_ready;                   /* Complete source snapshot remains on Duke. */
static _Thread_local int publication_scan;      /* This call scans the real fixture source. */
static _Thread_local int metadata_reserved;     /* First successful leaf reservation in this scan. */
static _Thread_local int metadata_active;       /* Native allocation/free observation window. */
static _Thread_local int metadata_fault;        /* Exactly one allocation refused in this attempt. */
static _Thread_local int metadata_cancelled;    /* Real reservation cancellation completed. */
static _Thread_local unsigned metadata_ordinal; /* Actual allocation position, one through five. */
static _Thread_local void *metadata_buffers[5]; /* Owned temporary allocations until real free. */


void *__real_malloc(size_t);


void *__real_calloc(size_t, size_t);


void __real_free(void *);


int __real_dvm_retirement_cancel(DVM_RetirementQueue *, DVM_RetirementReservation *);


unsigned long __real_dvm_world_retire_epoch(DVM_NativeWorld *);


/* [GC:] EN: Observe source scan selection without changing its result or queue.
 * selected is metadata; only Duke later completes this large source capture.
 * RU: Наблюдать выбор source, не меняя ответ или очередь. selected — метаданные;
 * лишь Duke впоследствии завершает этот большой обход. */
static void publication_begin(int selected)
{
    publication_scan = selected && SCENARIO >= 4;
    metadata_reserved = 0;
    metadata_active = 0;
    metadata_fault = 0;
    metadata_cancelled = 0;
}


/* [GC:] EN: A completed discovery must not run again during metadata retry.
 * start is a rooted source; inspect identity only before forwarding capture.
 * RU: Завершённый обход не повторяется при отказе метаданных. start удержан;
 * до передачи capture сравнивается только идентичность. */
static void publication_before_capture(void *start)
{
    require(!publication_scan || start != H2D(void *, nodes[0]) || !publication_ready,
            "metadata retry repeated Duke discovery");
}


/* [GC:] EN: Mark only a complete dead source graph, after its outside owner left.
 * result/group are actual capture output; do not mutate discovery or references.
 * RU: Отметить лишь полный мёртвый граф после снятия внешнего владельца.
 * result/group — настоящий ответ; обход и ссылки не меняются. */
static void publication_captured(int result, const RecordGroup *group)
{
    if (publication_scan && result == 1 && !group->live[0] && mutated && !outside_held)
    {
        require(group->nodes_used == Leaves + 2 && group->pinned_nodes == Leaves + 1,
                "complete Duke source array and string pins captured");
        publication_ready = 1;
        publication_captures++;
    }
}


/* [GC:] EN: Arm observation after the real first publication reservation.
 * result is forwarded unchanged; no queue allocation/result is substituted.
 * RU: Включить наблюдение после первого настоящего резерва публикации.
 * result не меняется; выделение очереди и её ответ не подменяются. */
static void publication_reserved(DVM_RetirementResult result)
{
    if (publication_scan && publication_ready && !metadata_reserved && result == DVM_RETIRE_OK)
    {
        metadata_reserved = 1;
        metadata_active = 1;
        metadata_ordinal = 0;
        memset(metadata_buffers, 0, sizeof(metadata_buffers));
    }
}


/* [GC:] EN: Refuse exactly the selected real malloc/calloc position for eight
 * passes. count/bytes/zero select the original allocator; successful allocations
 * remain production-owned and are observed through actual free, never freed here.
 * RU: Отказать выбранной позиции настоящего malloc/calloc на восемь проходов.
 * count/bytes/zero выбирают исходный аллокатор. Успешные буферы принадлежат runtime;
 * наблюдается настоящий free, сама проверка их не освобождает. */
static void *publication_allocate(size_t count, size_t bytes, int zero)
{
    unsigned ordinal = ++metadata_ordinal;
    require(ordinal <= 5, "publication allocation sequence is bounded");
    if (publication_passes < 8 && ordinal == (unsigned)(SCENARIO - 3))
    {
        metadata_fault = 1;
        publication_refusals++;
        return NULL;
    }
    void *value = zero ? __real_calloc(count, bytes) : __real_malloc(bytes);
    metadata_buffers[ordinal - 1] = value;
    return value;
}


/* [GC:] EN: Forward all real malloc requests; observe only active metadata.
 * bytes is unchanged; the returned allocation belongs solely to its real caller.
 * RU: Передать настоящий malloc, наблюдая лишь активные метаданные.
 * bytes не меняется; полученный буфер принадлежит исходному вызывающему. */
void *__wrap_malloc(size_t bytes)
{
    return metadata_active ? publication_allocate(1, bytes, 0) : __real_malloc(bytes);
}


/* [GC:] EN: Observe cleanup of successful temporary native allocations.
 * value is forwarded exactly once; pointer identities are not dereferenced.
 * RU: Наблюдать очистку успешных временных native-выделений. value передаётся
 * ровно один раз; адреса сравниваются без чтения освобождённых данных. */
void __wrap_free(void *value)
{
    if (metadata_active && value)
    {
        for (unsigned index = 0; index < 5; index++)
        {
            if (metadata_buffers[index] == value)
            {
                metadata_buffers[index] = NULL;
            }
        }
    }
    __real_free(value);
}


/* [GC:] EN: Before real cancellation require every successful native allocation
 * to have been freed; preserve queue/token until the original cancellation.
 * RU: До настоящей отмены требовать free каждого успешного native-выделения;
 * queue/token сохраняются до исходного cancel. */
int __wrap_dvm_retirement_cancel(DVM_RetirementQueue *queue, DVM_RetirementReservation *token)
{
    if (metadata_active)
    {
        require(metadata_fault, "publication failure must be the selected allocation refusal");
        for (unsigned index = 0; index < 5; index++)
        {
            require(!metadata_buffers[index], "failed publication metadata must be freed");
        }
    }
    int result = __real_dvm_retirement_cancel(queue, token);
    if (metadata_active)
    {
        require(result, "publication reservation cancellation succeeds");
        metadata_cancelled = 1;
        metadata_active = 0;
    }
    return result;
}


/* [GC:] EN: A failed metadata preparation must not advance the real epoch.
 * world stays runtime-owned. Successful preparation forwards its real cutoff.
 * RU: Неудачная подготовка метаданных не продвигает настоящую эпоху.
 * world принадлежит runtime. Успешная подготовка получает настоящий cutoff. */
unsigned long __wrap_dvm_world_retire_epoch(DVM_NativeWorld *world)
{
    require(!metadata_active || !metadata_fault, "failed metadata must not consume retirement epoch");
    unsigned long result = __real_dvm_world_retire_epoch(world);
    if (metadata_active)
    {
        require(metadata_ordinal == 5 && result, "all publication metadata precedes actual retirement epoch");
        metadata_active = 0;
    }
    return result;
}


/* [GC:] EN: A refused source must retain its completed snapshot and all roots.
 * result is the unchanged real scan response. Managed values remain Duke-owned.
 * RU: Отказавший source сохраняет завершённый снимок и все корни.
 * result — неизменённый ответ обхода. Управляемые объекты ещё принадлежат Duke. */
static void publication_end(int result)
{
    if (publication_scan && metadata_fault)
    {
        require(metadata_cancelled, "failed metadata reservation must be cancelled");
        require(result == DVM_CANDIDATE_RESUME, "metadata refusal must retain complete Duke publication");
        for (unsigned index = 0; index < TotalTargets; index++)
        {
            if (!index || index >= ArrayIndex)
            {
                require(dvm_runtime_root_count(nodes[index]) == 1 && dvm_refcount_load_u(&nodes[index]->ref) == 2,
                        "metadata retry preserves existing discovery roots and references");
            }
        }
    }
    publication_scan = 0;
}
