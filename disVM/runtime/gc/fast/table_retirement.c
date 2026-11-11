/* Copyright (c) 2026 bhgv
 * EN: Last-owner retirement of known scalar tables and immutable native keys.
 * RU: Освобождение последних владельцев скалярных таблиц и нативных снимков ключей.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/refcount.h"
#include "../../bigtable_owner.h"
#include "table_retirement.h"
#include <stdio.h>
#include <stdlib.h>


/* [GC:] EN: Admit exact startup types only; heap is borrowed through its GC owner.
 * RU: Допускать только точные стартовые типы; heap заимствован через владельца GC. */
int dvm_gc_fast_table_eligible(const Heap *heap)
{
    return dvm_bigtable_native_kind(heap) != 0;
}


/* [GC:] EN: Run a known native callback after last-reference claim and grace period.
 * RU: Выполнить известный callback после захвата последней ссылки и безопасной эпохи. */
int dvm_gc_fast_table_finalize(Heap *heap)
{
    int kind = dvm_bigtable_native_kind(heap);
    if (!kind || !heap->t->free || heap->t->np || dvm_refcount_load_u(&heap->ref) != 0 ||
        !dvm_bigtable_native_quiet(heap))
    {
        fprintf(stderr, "disvm: invalid shared native table finalizer claim\n");
        abort();
    }
    heap->t->free(heap, 0);
    return kind;
}
