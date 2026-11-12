/* Copyright (c) 2026 bhgv
 *
 * EN: Pass entry points for the native GC scheduler.
 * RU: Точки входа проходов нативного планировщика GC.
 */
#ifndef DVM_GC_COLLECTOR_PASSES_H
#define DVM_GC_COLLECTOR_PASSES_H

#include "disvm/gc_workers.h"

struct Prog;


/* [GC:] EN: Closed concurrent Fast/Cycles routes borrow context/budget/user;
 * return DONE/MORE, never enter retained admission. Forbidden work exits 70.
 * RU: Замкнутые конкурентные Fast/Cycles заимствуют context/budget/user;
 * возвращают DONE/MORE без резервного допуска. Запрещённая работа даёт код 70. */
DVM_GCWorkResult dvm_gc_fast_concurrent(DVM_NativeContext *context, size_t budget, void *user);


DVM_GCWorkResult dvm_gc_cycles_concurrent(DVM_NativeContext *context, size_t budget, void *user);


/* [GC:] EN: Drain before closing queues after guest joins; temporarily leave
 * the current ACTIVE controller, then restore its same native context/registers.
 * Return zero before requests for wrong controller/world/phase, live object
 * guards/progress or in-flight guests; also zero on protocol/service failure.
 * No roots/owners are discarded.
 * RU: После join гостей обработать очереди до закрытия; временно вывести
 * ACTIVE-контроллер и вернуть тот же контекст/регистры. Ноль при ошибке службы
 * или протокола; корни и владельцы не отбрасываются. Чужой контроллер/мир/фаза,
 * guards/progress или выполняющиеся гости отвергаются до запросов. */
int dvm_runtime_gc_concurrent_drain(void);


/* EN: Initialize collector TLS errors. RU: Инициализировать буферы ошибок сборщика. */
void dvm_gc_collector_environment(void);


/* EN: Fast queue check. RU: Проверить очередь Fast. user не используется. */
int dvm_gc_fast_ready(void *user);


/* EN: Fast shared work on context with budget. RU: Параллельный проход Fast на context с бюджетом budget. */
unsigned int dvm_gc_fast_shared(DVM_NativeContext *context, size_t budget, void *user);


/* EN: Fast retained release. RU: Резервный проход Fast на context с бюджетом budget. */
DVM_GCWorkResult dvm_gc_fast_retained(DVM_NativeContext *context, size_t budget, void *user);


/* EN: Cycles shared work. RU: Обход кандидатов на context в пределах budget. */
unsigned int dvm_gc_cycles_shared(DVM_NativeContext *context, size_t budget, void *user);


/* EN: Trace the registered process roots and update pass counters.
 * RU: Обойти зарегистрированные корни и обновить счётчики прохода. */
DVM_GCWorkResult dvm_gc_cycles_trace(unsigned int *idle_passes, unsigned long *idle_epoch, unsigned long *passes);


/* EN: Duke shared work. RU: Обход больших графов на context в пределах budget. */
unsigned int dvm_gc_duke_shared(DVM_NativeContext *context, size_t budget, void *user);

#endif
