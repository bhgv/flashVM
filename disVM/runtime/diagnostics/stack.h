/* Copyright (c) 2026 bhgv
 * EN: Bounded pointer-free call-chain copies; no stack or module retention.
 * RU: Ограниченные копии цепочки вызовов без указателей и удержания stack/module.
 */
#ifndef DVM_DIAGNOSTIC_STACK_H
#define DVM_DIAGNOSTIC_STACK_H
#include "location.h"

#define DVM_DIAG_STACK_FRAMES 8 /* Top location plus at most seven caller return positions. */


/* EN/RU: Exactly one terminal capture classification, independent of label flags.
 * Ровно одна итоговая классификация capture, отдельно от label flags. */
enum
{
    DVM_STACK_COMPLETE = 1,   /* Reached root with no saved caller frame. */
    DVM_STACK_TRUNCATED = 2,  /* More caller frames exist beyond bounded capacity. */
    DVM_STACK_UNAVAILABLE = 4 /* Invalid/unavailable position or repeated frame in trusted chain. */
};


/* EN/RU: Immutable native prefix; each entry is a display location, never an address.
 * Неизменяемый native prefix; каждый entry — display location, не адрес. */
typedef struct DVM_DiagnosticStack
{
    unsigned long sequence;                               /* Same owner boundary as the top location. */
    int count;                                            /* Valid copied prefix length, 0..8. */
    int flags;                                            /* Exactly one DVM_STACK_* classification. */
    DVM_DiagnosticLocation frames[DVM_DIAG_STACK_FRAMES]; /* Top first, then caller next/return PCs. */
} DVM_DiagnosticStack;


/* [GC:] EN: Active owner borrows live top/frame/module roots through this call.
 * Copy up to eight locations into disjoint native output; top is already captured.
 * No allocation, callback or retain. Geometry checks do not validate arbitrary
 * foreign pointers. fp/module/return links must be valid immutable owner metadata.
 * RU: Active owner заимствует живые top/frame/module roots до возврата.
 * Копировать до восьми locations в отдельный native output; top уже снят.
 * Без allocation, callback либо retain. Geometry не проверяет произвольные чужие
 * pointers. fp/module/return links — корректные metadata своего owner. */
void dvm_diagnostic_stack_capture(DVM_DiagnosticStack *output,
                                  const DVM_DiagnosticLocation *top,
                                  struct Modlink *module,
                                  const void *fp);
#endif
