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
#include "../../gc_fields.h"
#include "../../runtime_gc.h"
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


/* [GC:] EN: Keep an extra original MP owner before a possibly nonfinal Module
 * release. module stays independently owned; immutable origmp is borrowed only
 * before that release. Extra ownership prevents concurrent graph claims. Marking
 * must succeed before the nonfallible retain. Caller publishes with destroy.
 * RU: Удержать origmp перед возможным непоследним release Module. module ещё
 * независимо принадлежит вызывающему; origmp читается только до release.
 * Дополнительная ссылка исключает захват графа. Пометка предшествует retain;
 * вызывающий публикует владельца через destroy. */
void *dvm_gc_module_retry_mp(Module *module)
{
    if (!dvm_runtime_idle_cycles || !module || (module->rt & DYNMOD) || module->dlm ||
        dvm_refcount_load_i(&module->ref) <= 1 || !module->origmp || module->origmp == H)
    {
        return H;
    }
    Heap *heap = D2H(module->origmp);
    Setmark(heap);
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        fputs("disvm: original MP retry lost its independently owned reference\n", stderr);
        abort();
    }
    return module->origmp;
}


/* [GC:] EN: Consume graph-owned native module after its final reader cutoff.
 * Do not release its already-consumed origmp or zero native count again. Native
 * imports/types/code still follow the reviewed loader finish outside registry lock.
 * heap/module remain allocated through this call; no alias may escape.
 * RU: Снять native-модуль графа после последней эпохи. Не снимать повторно
 * origmp или native-счётчик ноль. Импорты/типы/код завершает loader вне mutex
 * реестра. heap/module ещё выделены; алиасы не выходят наружу. */
void dvm_gc_fast_module_graph_finalize(Heap *heap, Module *module)
{
    if (!heap || heap->t != &Tmodlink || H2D(Modlink *, heap)->m != module || H2D(Modlink *, heap)->MP != H ||
        dvm_refcount_load_u(&heap->ref) != 0 || !module || dvm_refcount_load_i(&module->ref) != 0 ||
        (module->rt & DYNMOD) || module->dlm)
    {
        fputs("disvm: invalid graph-owned native Module finalization\n", stderr);
        abort();
    }
    H2D(Modlink *, heap)->m = NULL;
    module->origmp = H;
    dvm_runtime_module_finish(module);
}
