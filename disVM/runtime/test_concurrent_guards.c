/* Copyright (c) 2026 bhgv
 * [GC:] EN: Probe forbidden fallback entries on actual native DisVM processes.
 * RU: Проверить запрещённые входы fallback на настоящих нативных процессах DisVM.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "runtime_rc.h"
#include "gc/common/runtime_state.h"
#include <stdio.h>
#include <stdlib.h>


void __real_Sys_werrstr(void *);


/* [GC:] EN: This unknown callback must never run. heap/swept remain owned by
 * the real retirement path; fail explicitly rather than free them unsafely.
 * RU: Неизвестный callback не должен исполняться. heap/swept принадлежат
 * настоящему освобождению; явный отказ вместо небезопасного free. */
static void unknown(Heap *heap, int swept)
{
    (void)heap;
    (void)swept;
    fputs("FAIL strict guard: unknown callback executed\n", stderr);
    exit(12);
}


/* [GC:] EN: Forward borrowed frame and probe one real forbidden API according
 * to CONTROL. Its production guard must exit 70 before any legacy heap access.
 * RU: Выполнить заимствованный frame и проверить настоящий запрещённый API
 * по CONTROL. Его защита должна дать код 70 до прежнего доступа к куче. */
void __wrap_Sys_werrstr(void *frame)
{
    __real_Sys_werrstr(frame);
    if (CONTROL == 0)
    {
        Type *type = dtype(unknown, sizeof(void *), NULL, 0);
        Heap *heap = heapz(type);
        if (!dvm_runtime_rc_defer(H2D(void *, heap)))
        {
            exit(12);
        }
        for (;;)
        {
            dvm_native_context_poll();
        }
    }
    else if (CONTROL == 1)
    {
        rungc(up->prog);
    }
    else if (CONTROL == 2)
    {
        dvm_runtime_rc_drain(1);
    }
    else if (CONTROL == 3)
    {
        dvm_world_stop_begin(dvm_gc_runtime.world, dvm_native_context_current());
    }
    else
    {
        dvm_world_try_quiescent(dvm_gc_runtime.world, dvm_native_context_current());
    }
    fputs("FAIL strict guard: forbidden operation returned\n", stderr);
    exit(12);
}
