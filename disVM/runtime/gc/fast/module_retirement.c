/* Copyright (c) 2026 bhgv
 * EN: Finalize known ordinary module metadata after shared Fast's MP handoff.
 * RU: Завершить известные метаданные обычного модуля после передачи MP на Fast.
 */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "disvm/refcount.h"
#include "module_retirement.h"
#include <stdio.h>
#include <stdlib.h>


/* [GC:] EN: Check borrowed live heap without invoking any free callback.
 * Exact static type plus owned ordinary Module exclude native plugin state.
 * RU: Проверить живой heap без free callback. Точный статический тип и ссылка
 * на обычный Module исключают состояние native-плагина. */
int dvm_gc_fast_module_eligible(const Heap *heap)
{
    if (!heap || heap->t != &Tmodlink)
    {
        return 0;
    }
    const Modlink *link = H2D(const Modlink *, heap);
    const Module *module = link->m;
    return module && module != H && !(module->rt & DYNMOD) && !module->dlm && !link->data && link->MP &&
           dvm_refcount_load_i(&module->ref) > 0;
}


/* [GC:] EN: Consume only link's Module owner after MP has its replacement root.
 * unload's managed release must use the caller's still-accepting shared RC path.
 * Do not read the Module after unload: it may unlink and free it synchronously.
 * RU: Снять только ссылку link на Module после публикации замещающего корня MP.
 * Управляемые ссылки unload передаёт ещё принимающему shared RC вызывающего.
 * После unload Module не читать: он может быть удалён и освобождён синхронно. */
void dvm_gc_fast_module_finalize(Heap *heap)
{
    if (!dvm_gc_fast_module_eligible(heap) || dvm_refcount_load_u(&heap->ref) != 0 || H2D(Modlink *, heap)->MP != H)
    {
        fputs("disvm: invalid shared Modlink finalizer claim\n", stderr);
        abort();
    }
    Module *module = H2D(Modlink *, heap)->m;
    H2D(Modlink *, heap)->m = NULL;
    unload(module);
}
