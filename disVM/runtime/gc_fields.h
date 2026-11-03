/* Native scalar GC metadata access, not a concurrent graph/epoch protocol. */
#ifndef DVM_GC_FIELDS_H
#define DVM_GC_FIELDS_H


/* Load independently live/aligned int source into private disjoint result.
 * Return0 before access for NULL/unaligned/identical addresses. No allocation,
 * callback or lifetime/payload permission. All competing accesses use this API. */
int dvm_gc_field_load(const int *source, int *result);


/* Store one independently live/aligned native int. Return0 for NULL/unaligned
 * destination. Value is metadata, not a managed reference or publication root.
 * May take port registry mutex on fallback; caller must not already hold it. */
int dvm_gc_field_store(int *destination, int value);


/* Exchange live/aligned destination with value and return predecessor through
 * live/aligned private previous. Return0 before access on NULL/aliased cells.
 * All competing accesses use this API. May take registry mutex; no allocation,
 * callbacks or VM calls. Caller supplies lifetime and must not hold that mutex.
 * Atomic scalar consumption is not graph publication or epoch completion. */
int dvm_gc_field_exchange(int *destination, int value, int *previous);


/* Mark protected color if different from selector0..2, then publish pending1.
 * All three native int addresses are live/aligned/distinct. Return0 on malformed
 * geometry or invalid color/selector before writes. Propagator color is3.
 * No allocator/callback/VM call. Selector/collector epoch changes still require
 * caller exclusion; this does NOT atomically publish a graph edge or GC epoch. */
int dvm_gc_field_mark(int *color, const int *selector, int *pending);


/* [GC:] Hosted wrappers terminate on malformed native metadata. They acquire no
 * source owner/root/access permission. load returns one observed value, store
 * writes value, mark uses the protected color/selector/pending cells above.
 * No VM error/callback escapes under any native mutex. */
int dvm_runtime_gc_load(const int *source);
void dvm_runtime_gc_store(int *destination, int value);
void dvm_runtime_gc_mark(int *color, const int *selector, int *pending);


/* [GC:] Hosted exchange uses the same lifetime/error/locking contract above;
 * returns the predecessor, never an epoch-completion or root guarantee. */
int dvm_runtime_gc_exchange(int *destination, int value);
#endif
