/* Copyright (c) 2026 bhgv
 *
 * EN: Pass entry points for the native GC scheduler.
 * RU: Точки входа проходов нативного планировщика GC.
 */
#ifndef DVM_GC_COLLECTOR_PASSES_H
#define DVM_GC_COLLECTOR_PASSES_H

#include "disvm/gc_workers.h"

struct Prog;

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

/* EN: Trace roots and update pass counters.
 * RU: Обойти roots, обновить idle_passes, idle_epoch и passes. */
DVM_GCWorkResult dvm_gc_cycles_trace(struct Prog **roots, unsigned int *idle_passes,
    unsigned long *idle_epoch, unsigned long *passes);

/* EN: Duke shared work. RU: Обход больших графов на context в пределах budget. */
unsigned int dvm_gc_duke_shared(DVM_NativeContext *context, size_t budget, void *user);

#endif
