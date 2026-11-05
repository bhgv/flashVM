/* Native LENA with a continuous guarded source owner and no temporary root. */
#ifndef DVM_SHARED_ARRAY_LENGTH_H
#define DVM_SHARED_ARRAY_LENGTH_H


/* [GC:] source is a live owned array-pointer slot; destination a live WORD slot.
 * Return static error or NULL after releasing all guards. Source stays guarded
 * from pointer capture through field read/publication. Nil/H values produce0.
 * Caller preserves mapped referent/root lifetime; no allocation, retain/root,
 * callback or general parallel GC/code admission is supplied. */
const char *dvm_runtime_array_length(const void *source, void *destination);
/* Capture live pointer/index slots into private storage under their guard union.
 * Caller retains NativeWorld. A scheduled retry sets replay and leaves both
 * snapshots untouched. Otherwise return NULL on success or a static error. */
const char *dvm_runtime_array_index_operands(const void *source, const void *index,
    void *array_snapshot, void *index_snapshot, int *replay);
#endif
