/* Copyright (c) 2026 bhgv
 * EN: Bounded copied FP diagnostics, never live CPU or managed-memory inspection.
 * RU: Ограниченная copied FP диагностика без live CPU и managed-memory inspection.
 */
#ifndef DVM_JIT_FP_VIEW_H
#define DVM_JIT_FP_VIEW_H
#include "signals.h"


/* EN/RU: Format borrowed copied state into caller output. Return1 for complete
 * base context,0 for absent/invalid/capacity error. No allocation or FP arithmetic.
 * Форматировать borrowed copy в caller output. Вернуть1 при complete base context,
 * 0 при absent/invalid/capacity error. Без allocation и FP arithmetic. */
int dvm_jit_fault_fp_json(const DVM_JitFault *fault, char *output, size_t capacity);


/* EN/RU: Log copied FP bits after signal landing, correlated with source pid.
 * Вывести copied FP bits после signal landing с PID исходного owner. */
void dvm_jit_fault_fp_report(const DVM_JitFault *fault, int pid);
#endif
