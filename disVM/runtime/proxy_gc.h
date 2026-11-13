/* Copyright (c) 2026 bhgv
 * [GC:] EN: Exact hosted lease types admitted for concurrent native finalization.
 * RU: Точные hosted lease types для concurrent native finalization.
 */
#ifndef DVM_PROXY_GC_H
#define DVM_PROXY_GC_H


/* EN/RU: Register native descriptors/module once before starting any guest/GC worker.
 * Зарегистрировать descriptors/module один раз до guest/GC worker. */
void dvm_proxy_gc_initialize(void);


/* [GC:] EN: Classify a borrowed live heap by immutable startup type identities.
 * RU: Классифицировать borrowed live heap по immutable startup type identities. */
int dvm_proxy_gc_eligible(const Heap *heap);


/* [GC:] EN: Finalize exact zero-count scalar native payload after reader grace.
 * No VM callback/managed access/network IO is allowed in its native destructor.
 * RU: Завершить точный zero-count scalar payload после reader grace.
 * Native destructor не выполняет VM callback/managed access/network IO. */
void dvm_proxy_gc_finalize(Heap *heap);
#endif
