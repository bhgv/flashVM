/* Copyright (c) 2026 bhgv
 * EN: Independently count returned native callback acceptance on both public routes.
 * RU: Независимо считать принятие native callback по обоим публичным путям.
 */
#include "lib9.h"
#include "interp.h"
#include <stdio.h>

static unsigned long accepted; /* Atomic observations after real nonzero returns. */
static unsigned long declined; /* Atomic observations after real zero returns. */

int __real_dvm_runtime_fast(int, int (*)(REG *), REG *);
int __real_dvm_runtime_leaf(Jit *, int, REG *);
void __real_dvm_close_worker_stop(void);


/* EN: Record copied result after the genuine callback route completes unchanged.
 * RU: Учесть скопированный result после неизменённого настоящего пути callback. */
static void record(int result)
{
    if (result)
    {
        __atomic_fetch_add(&accepted, 1, __ATOMIC_RELAXED);
    }
    else
    {
        __atomic_fetch_add(&declined, 1, __ATOMIC_RELAXED);
    }
}


/* EN: Forward borrowed operation/fast/registers once and observe its exact result.
 * RU: Передать заимствованные operation/fast/registers и наблюдать точный результат. */
int __wrap_dvm_runtime_fast(int operation, int (*fast)(REG *), REG *registers)
{
    int result = __real_dvm_runtime_fast(operation, fast, registers);
    record(result);
    return result;
}


/* EN: Forward borrowed jit/operation/registers once, without inspecting VM state.
 * RU: Передать заимствованные jit/operation/registers, не исследуя состояние VM. */
int __wrap_dvm_runtime_leaf(Jit *jit, int operation, REG *registers)
{
    int result = __real_dvm_runtime_leaf(jit, operation, registers);
    record(result);
    return result;
}


/* EN: After genuine producer/close shutdown print stable independent observations.
 * No parameters/result; the driver compares them with the production stats line.
 * RU: После настоящего завершения производителей/close вывести стабильные данные.
 * Без параметров/результата; драйвер сверяет их со статистикой production. */
void __wrap_dvm_close_worker_stop(void)
{
    __real_dvm_close_worker_stop();
    fprintf(stderr, "NATIVE_ACCEPTANCE observed=%lu declined=%lu\n",
        __atomic_load_n(&accepted, __ATOMIC_RELAXED),
        __atomic_load_n(&declined, __ATOMIC_RELAXED));
}
