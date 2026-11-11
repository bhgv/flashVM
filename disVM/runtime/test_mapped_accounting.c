/* Copyright (c) 2026 bhgv
 * EN: Observe genuine mapped slice completions without replacing transfer work.
 * RU: Наблюдать настоящие mapped-срезы без подмены работы передачи.
 */
#include "lib9.h"
#include "interp.h"
#include <stddef.h>
#include <stdio.h>

static _Thread_local int slicing; /* Current resident's nested slice observation. */
static _Thread_local unsigned completed; /* Real shared finishes within this slice. */
static unsigned long batches; /* Atomic real multi-instruction batch observations. */
static unsigned long observed; /* Atomic successful private slice observations. */

void __real_dvm_runtime_array_slice_copy(const void *, void *, Type *, size_t, Array *, Array *);
void __real_dvm_runtime_gc_shared_finish(void);
void __real_dvm_close_worker_stop(void);
int __real_dvm_runtime_integer_batch(const Module *, const Inst *, int);


/* EN: Forward borrowed spans/type/arrays once; count actual shared completion
 * only after the real private transfer returns, retaining all original ownership.
 * RU: Передать spans/type/arrays один раз; учесть настоящий shared-переход
 * после возврата личной передачи, сохраняя исходное владение. */
void __wrap_dvm_runtime_array_slice_copy(const void *source, void *destination,
    Type *type, size_t count, Array *from, Array *to)
{
    slicing = 1;
    completed = 0;
    __real_dvm_runtime_array_slice_copy(source, destination, type, count, from, to);
    __atomic_fetch_add(&observed, completed, __ATOMIC_RELAXED);
    slicing = 0;
}


/* EN: Preserve the real GC transition and observe it on this resident only.
 * Private arrays, including overlapping views, have no competing range claimant.
 * RU: Сохранить настоящий переход GC и наблюдать его только в этом потоке.
 * Личные массивы, включая перекрывающиеся views, не имеют конкурирующего захватчика. */
void __wrap_dvm_runtime_gc_shared_finish(void)
{
    __real_dvm_runtime_gc_shared_finish();
    if (slicing)
    {
        completed++;
    }
}


/* EN: Print independent observations after actual resident/close joins.
 * RU: Вывести независимые наблюдения после настоящего join потоков и close. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "SCALAR_OBSERVED batches=%lu\n",
        __atomic_load_n(&batches, __ATOMIC_RELAXED));
    fprintf(stderr, "MAPPED_OBSERVED slices=%lu\n",
        __atomic_load_n(&observed, __ATOMIC_RELAXED));
}


/* EN: Forward borrowed module/pc/index once and count returned multi-step work.
 * RU: Передать module/pc/index один раз и учесть возвращённую многошаговую работу. */
int __wrap_dvm_runtime_integer_batch(const Module *module, const Inst *pc, int index)
{
    int result = __real_dvm_runtime_integer_batch(module, pc, index);
    if (result > 1)
    {
        __atomic_fetch_add(&batches, 1, __ATOMIC_RELAXED);
    }
    return result;
}
