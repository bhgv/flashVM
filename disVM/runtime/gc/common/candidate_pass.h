/* Copyright (c) 2026 bhgv
 *
 * EN: Ownership handoff interfaces shared by candidate collectors.
 * RU: Общие интерфейсы передачи владения между сборщиками кандидатов.
 */
#ifndef DVM_GC_CANDIDATE_PASS_H
#define DVM_GC_CANDIDATE_PASS_H

#include "../../retirement_queue.h"
#include "disvm/native_world.h"

/* EN: Move a rooted candidate between queues. RU: Передать candidate из source в destination. */
int dvm_rc_move_candidate(DVM_RetirementQueue *source, DVM_RetirementQueue *destination,
    void *candidate);

/* EN: Scan source head, returning cost and retry status in busy.
 * RU: Проверить первый элемент source, вернуть затраты в cost и признак повтора в busy. */
int dvm_rc_scan_candidate(DVM_RetirementQueue *source, size_t *cost, int *busy);

/* EN: Run Duke using the borrowed world, work queue and fallback queue.
 * RU: Выполнить проход ДюкGC на context с budget, очередью queue и резервом fallback. */
unsigned int dvm_gc_duke_run(DVM_NativeContext *context, size_t budget,
    DVM_NativeWorld *world, DVM_RetirementQueue *queue, DVM_RetirementQueue *fallback);

#endif
