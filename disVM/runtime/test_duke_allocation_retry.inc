/* Copyright (c) 2026 bhgv
 * [GC:] EN: Real Duke allocation failure and shared retry observations.
 * RU: Настоящий отказ выделения снимка Duke и наблюдение shared-повторов.
 */
static unsigned allocation_refusals;      /* Actual calloc refusals, under sync. */
static unsigned allocation_passes;        /* Actual positive Duke responses, under sync. */
static _Thread_local int selected_scan;   /* Current scan owns the fixture source. */
static _Thread_local int fail_allocation; /* Refuse only the selected snapshot calloc. */
static _Thread_local int failed_scan;     /* A genuine allocation failed in this scan. */


void *__real_calloc(size_t, size_t);


int __real_dvm_rc_grow_record_group(RecordGroup *, size_t);


int __real_dvm_rc_scan_candidate(DVM_RetirementQueue *, size_t *, int *);


/* [GC:] EN: Refuse the actual snapshot allocation only during a selected grow.
 * count/bytes are forwarded otherwise; no fake storage or managed owners appear.
 * RU: Отказать настоящему выделению снимка лишь внутри выбранного grow.
 * Иначе count/bytes передаются без изменения; фиктивных буферов и ссылок нет. */
void *__wrap_calloc(size_t count, size_t bytes)
{
    if (fail_allocation)
    {
        pthread_mutex_lock(&sync);
        allocation_refusals++;
        pthread_mutex_unlock(&sync);
        return NULL;
    }
    return __real_calloc(count, bytes);
}


/* [GC:] EN: Exercise initial/growing group allocation until eight real passes.
 * group stays runtime-owned; required is unchanged. Failure must preserve all
 * storage identities/cursors and signal busy, without declaring a hard limit.
 * RU: Проверить начальное/повторное выделение group до восьми настоящих проходов.
 * group принадлежит runtime, required не меняется. Отказ сохраняет буферы/курсоры,
 * сообщает busy и не означает превышения предела. */
int __wrap_dvm_rc_grow_record_group(RecordGroup *group, size_t required)
{
    pthread_mutex_lock(&sync);
    int inject = selected_scan && allocation_passes < 8 &&
                 ((SCENARIO == 2 && !group->owned_scratch) ||
                  (SCENARIO == 3 && group->owned_scratch && required > group->field_limit));
    pthread_mutex_unlock(&sync);
    RecordGroup before;
    memcpy(&before, group, sizeof(before));
    fail_allocation = inject;
    int result = __real_dvm_rc_grow_record_group(group, required);
    fail_allocation = 0;
    if (inject)
    {
        require(!result && group->busy && !group->oversized, "transient snapshot allocation must request retry");
        before.busy = group->busy;
        require(!memcmp(&before, group, sizeof(before)), "failed snapshot allocation preserves storage and cursors");
        RecordGroup limited;
        memcpy(&limited, group, sizeof(limited));
        require(!__real_dvm_rc_grow_record_group(&limited, ~(size_t)0) && !limited.busy,
                "hard Duke limit must remain distinct from allocation retry");
        before.busy = 0;
        require(!memcmp(&before, &limited, sizeof(before)), "hard snapshot limit preserves storage and cursors");
        memcpy(&limited, group, sizeof(limited));
        require(!dvm_rc_reserve_record_fields(&limited, ~(size_t)0) && !limited.busy && limited.oversized,
                "hard field limit must remain distinct from allocation retry");
        before.oversized = 1;
        require(!memcmp(&before, &limited, sizeof(before)), "hard field limit preserves storage and cursors");
        failed_scan = 1;
    }
    return result;
}


/* [GC:] EN: Forward the real source scan and observe transient initial failure.
 * queue/cost/busy stay borrowed. Never replace a scan result or queue operation.
 * RU: Выполнить настоящий обход source и проверить временный начальный отказ.
 * queue/cost/busy заимствованы. Ответ обхода и операции очереди не подменяются. */
int __wrap_dvm_rc_scan_candidate(DVM_RetirementQueue *queue, size_t *cost, int *busy)
{
    pthread_mutex_lock(&sync);
    selected_scan = node_count && dvm_retirement_peek(queue) == H2D(void *, nodes[0]);
    pthread_mutex_unlock(&sync);
    failed_scan = 0;
    int result = __real_dvm_rc_scan_candidate(queue, cost, busy);
    require(!failed_scan || (!result && *busy),
            SCENARIO == 2 ? "initial Duke allocation must remain shared" : "transient Duke growth escaped to fallback");
    selected_scan = 0;
    failed_scan = 0;
    return result;
}
