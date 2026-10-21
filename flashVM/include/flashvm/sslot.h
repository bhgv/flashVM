/* Fixed S-record slots preserve the next boundary even when a record length is damaged. */
#ifndef FLASHVM_SSLOT_H
#define FLASHVM_SSLOT_H /* Guard the standalone C89 slot and batch API. */
#include "srecord.h"
#define FVM_SSLOT_BYTES 1024U /* Profile 1 fixed slot size, agreed before receiving data. */
#define FVM_SSLOT_BATCH 64U   /* Bounded number of slots processed in one owner call. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* One complete record followed by zero padding; input/output/state must not overlap.
     * No allocation, transport, VM or tag binding. Writer publishes exactly SSLOT_BYTES on OK.
     * The slot profile/size must be agreed over a trusted control channel, not inferred from
     * damage. */
    FVM_API FVM_SResult fvm_sslot_write(unsigned long tag, const void *payload, size_t size,
                                        void *output, size_t capacity, size_t *used);
    /* Read exactly one full slot. A shorter input is MORE; longer input INVALID.
     * Check record checksum and all padding before publishing a borrowed view. A MORE from the
     * embedded record in a complete slot becomes INVALID: never consume the next slot as its body.
     * View and used remain unchanged on failure; used is the fixed slot size on success. */
    FVM_API FVM_SResult fvm_sslot_read(const void *, size_t, FVM_SRecord *, size_t *used);
    /* Callback receives a borrowed verified record on OK, otherwise NULL, once per fixed boundary.
     * Owner serializes calls, preserves input throughout callbacks and must not mutate that input.
     * This callback cannot stop a batch or change the next boundary. No managed roots are borrowed.
     */
    typedef void (*FVM_SSlotNotify)(size_t index, FVM_SResult, const FVM_SRecord *, void *userdata);
    /* Owned result counts for a completely traversed batch, including individually damaged slots.
     */
    typedef struct FVM_SSlotReport
    {
        size_t slots;   /* Number of fixed slot boundaries visited. */
        size_t valid;   /* Records passing framing, checksum and zero-padding validation. */
        size_t damaged; /* Rejected slots, independently skipped without scanning their length. */
    } FVM_SSlotReport;
    /* Accept zero through BATCH complete slots; partial trailing slot is INVALID before callbacks.
     * Every structurally bounded batch returns OK even if individual slots are damaged. Inspect
     * report/callback results. Invalid arguments leave report untouched and invoke no callbacks. */
    FVM_API FVM_SResult fvm_sslot_batch(const void *, size_t, FVM_SSlotNotify, void *,
                                        FVM_SSlotReport *);
#ifdef __cplusplus
}
#endif
#endif
