/* Private observer copy; never a retained Heap owner or a coherent graph pin. */
#ifndef DVM_RUNTIME_HEAP_SNAPSHOT_H
#define DVM_RUNTIME_HEAP_SNAPSHOT_H
struct Heap;


/* [GC:] Copy a live borrowed native header into disjoint PRIVATE Heap storage.
 * Read ref through native atomic/fallback primitive, never memcpy that field.
 * Other bytes still require VM exclusion/frozen metadata. Return0 unchanged on
 * nil/sentinel/alignment/span/overlap refusal; return1 after complete output.
 * Geometry is not memory provenance: caller keeps source/storage live and
 * mapped. No retain, mark, root, allocation, callback or managed admission.
 * Output carries observations only, not permission to destroy/retain its fields.
 * Caller must not already hold the port registry mutex. */
int dvm_runtime_heap_snapshot(struct Heap *destination, const void *source);


/* [GC:] Observe only ref in a live protected header into private unsigned-long
 * output. Return0 unchanged on nil/alignment/span refusal. Does not read other
 * header fields, retain, mark, allocate or acquire lifetime/admission. */
int dvm_runtime_heap_count(const void *source, unsigned long *result);


/* [GC:] Optional actual frozen-header/count-only concurrency diagnostic.
 * workers borrows live service; controller stays ACTIVE until every join. */
struct DVM_GCWorkers;
void dvm_runtime_snapshot_probe(struct DVM_GCWorkers *workers);
#endif
