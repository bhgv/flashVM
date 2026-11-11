/* Copyright (c) 2026 bhgv
 * EN: Duke queue readiness and runtime pass entry.
 * RU: Готовность очереди ДюкGC и вход прохода runtime.
 */


/* [GC:] EN: Read only native committed Duke queue metadata. Return boolean; no managed payload is borrowed.
 * RU: Прочитать только нативные опубликованные метаданные очереди ДюкGC. Вернуть bool без заимствования
 * управляемых данных. */
int dvm_runtime_rc_duke_ready(void)
{
    return dvm_retirement_peek(duke_candidates) != NULL;
}


/* EN: Run Duke on context with budget using the runtime-owned queues.
 * RU: Запустить ДюкGC на context с бюджетом budget и очередями рантайма. */
unsigned int dvm_runtime_rc_shared_duke(DVM_NativeContext *context, size_t budget)
{
    return dvm_gc_duke_run(context, budget, world, duke_candidates, cycle_refused);
}
