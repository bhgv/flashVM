/* Copyright (c) 2026 bhgv
 * EN: Known native FD finalization without a stopped mutator world.
 * RU: Известный нативный финализатор FD без остановки всех исполнителей.
 */
#include "lib9.h"
#include "dat.h"
#include "interp.h"
#include "disvm/refcount.h"
#include "file_retirement.h"
#include <stdio.h>
#include <stdlib.h>

extern Type *TFD; /* Immutable Sys.FD identity, initialized before GC/guest startup. */


/* [GC:] EN: Admit only the native pointer-free FD type, never an arbitrary free callback.
 * RU: Допускать только нативный беспоинтерный FD, не произвольный free callback. */
int dvm_gc_fast_file_eligible(const Heap *heap)
{
    return TFD && heap->t == TFD;
}


/* [GC:] EN: Finalize a last-reference claimed heap; caller retains header/type storage.
 * RU: Завершить heap с захваченной последней ссылкой; заголовком/типом владеет вызывающий. */
void dvm_gc_fast_file_finalize(Heap *heap)
{
    if (!dvm_gc_fast_file_eligible(heap) || !heap->t->free || heap->t->np ||
        dvm_refcount_load_u(&heap->ref) != 0)
    {
        fprintf(stderr, "disvm: invalid shared FD finalizer claim\n");
        abort();
    }
    heap->t->free(heap, 0);
}
