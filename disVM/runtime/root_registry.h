/* Copyright (c) 2026 bhgv
 *
 * EN: Native root registry interface for externally held heap references.
 * RU: Интерфейс реестра корней для внешних удержаний объектов кучи.
 */
/* Native external-root metadata; registration never supplies object ownership.
 * Hosted GC visits this process-global table as managed Heap headers. There,
 * keys must be independently valid managed roots. Native-key tests run separate
 * processes with no collector; never mix arbitrary native keys into hosted GC. */
#ifndef DVM_ROOT_REGISTRY_H
#define DVM_ROOT_REGISTRY_H


/* Borrow one stable key during locked traversal. Callback must be bounded,
 * return normally and never allocate, wait, reenter registry or call the VM.
 * Payload access requires independent caller permission/lifetime exclusion. */
typedef void (*DVM_RootVisitor)(void *key, void *argument);


/* Register one occurrence of nonnull key. Return0 unchanged on native allocation
 * failure/null key. Native storage allocation happens outside the registry lock.
 * key is borrowed; caller preserves its referent and collector publication rules.
 * Duplicate registrations are independent multiset occurrences, not handles. */
int dvm_runtime_root_add(void *key);


/* Remove one ordinary add occurrence; batch occurrences are excluded.
 * Return 1 removed, 0 absent/null; no source dereference,
 * allocation or destructor. Caller must coordinate referent lifetime/collector. */
int dvm_runtime_root_remove(const void *key);


/* EN: Remove one ordinary key occurrence if permit(argument) succeeds.
 * The predicate runs under the registry mutex and must neither block nor reenter.
 * RU: Снять обычный корень key при успехе permit(argument).
 * Предикат выполняется под мьютексом реестра и не должен ждать или входить повторно. */
int dvm_runtime_root_remove_if(const void *key, int (*permit)(void *), void *argument);


/* Observe occurrences under mutex; return0 for absent/null. No owner/root
 * reservation or payload/COW permission. Saturate rather than wrap at ULONG_MAX. */
unsigned long dvm_runtime_root_count(const void *key);


/* Visit the locked multiset, once per occurrence. NULL visitor/empty table no-op.
 * argument is exclusively caller-owned through return. No snapshot escapes.
 * Managed GC callers additionally require stopped-world/root-lifetime exclusion.
 * Registry storage/mutex persist for process lifetime; no concurrent shutdown. */
void dvm_runtime_roots_visit(DVM_RootVisitor visitor, void *argument);
struct DVM_GCWorkers;


/* [GC:] Optional real managed root/Cycles/Fast diagnostic. workers borrows
 * live service; caller has ACTIVE controller except joined collector waits. */
void dvm_runtime_root_registry_probe(struct DVM_GCWorkers *workers);
#endif
