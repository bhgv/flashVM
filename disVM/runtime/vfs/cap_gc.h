/* Copyright (c) 2026 bhgv
 * [GC:] EN: Exact VFS capability finalization; include interp.h first.
 * RU: Exact VFS capability finalization; сначала interp.h.
 */
#ifndef DVM_VFS_CAP_GC_H
#define DVM_VFS_CAP_GC_H


/* [GC:] EN/RU: Admit only exact runtime-private scalar shells, no arbitrary callbacks.
 * Допустить только exact runtime-private scalar shells. */
int dvm_cap_gc_eligible(const Heap *heap);


/* [GC:] EN/RU: Enqueue preallocated cleanup after zero-count reader grace; no I/O/VM calls.
 * Поставить preallocated cleanup после zero-count reader grace; без I/O/VM calls. */
void dvm_cap_gc_finalize(Heap *heap);


/* EN/RU: Register descriptors and hosted module once before any guest starts.
 * Зарегистрировать descriptors/module до старта guests. */
void dvm_cap_initialize(void);

/* EN/RU: Copy native construction/retirement counters; optional outputs, no VM/I/O.
 * Скопировать counters native state lifecycle; optional outputs, без VM/I/O. */
void dvm_cap_lifetime_snapshot(unsigned long *created, unsigned long *retired);
#endif
