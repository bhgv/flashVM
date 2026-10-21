/* Bounded unique-part observations for one host-timed receive window; no clocks or I/O. */
#ifndef FLASHVM_SMEASURE_H
#define FLASHVM_SMEASURE_H /* Guard the standalone C89 measurement ABI. */
#include "sadapt.h"
#define FVM_SMEASURE_PARTS 4096UL /* Maximum explicitly expected distinct parts per window. */
#define FVM_SMEASURE_BITS 2U /* Missing, damaged, late or on-time category for each part. */
#define FVM_SMEASURE_PER_BYTE 4U /* Four two-bit categories per eight-bit wire octet. */
#define FVM_SMEASURE_BYTES (FVM_SMEASURE_PARTS / FVM_SMEASURE_PER_BYTE) /* Fixed native bitmap. */
#define FVM_SMEASURE_MISSING 0U /* No classified arrival; inferred absent at window close. */
#define FVM_SMEASURE_DAMAGED 1U /* Trusted part identity, but unusable data. */
#define FVM_SMEASURE_LATE 2U /* Verified usable data received after the deadline. */
#define FVM_SMEASURE_ON_TIME 3U /* Verified usable data received on or before the deadline. */
#define FVM_SMEASURE_MASK 3U /* Select one complete two-bit category. */
#define FVM_SMEASURE_VALID 1U /* The host verified usable data before recording the arrival. */
#define FVM_SMEASURE_INVALID 0U /* Only use with an independently trusted part identity. */
#ifdef __cplusplus
extern "C" {
#endif
/* Caller-owned serial state. Private fields after init; no VM roots or borrowed storage. */
typedef struct FVM_SMeasure {
    size_t struct_size; /* Exact native ABI, initialized by init. */
    FVM_SAdaptFeedback report; /* Exclusive current counts; RTT supplied only at close. */
    unsigned long deadline_ms; /* Positive host-relative deadline, not absolute wall time. */
    unsigned long last_ms; /* Greatest accepted arrival timestamp; requires monotonic host input. */
    int closed; /* A closed observation cannot accept further packets or produce a changed report. */
    unsigned char categories[FVM_SMEASURE_BYTES]; /* Fixed two-bit category for every expected part. */
} FVM_SMeasure;
/* Copy nonzero Uint32 stream/epoch/window, expected 1..PARTS and deadline 1..Uint32.
 * Failure preserves state. No allocation, socket, timer or implicit session adoption. */
FVM_API FVM_SResult fvm_smeasure_init(FVM_SMeasure *, unsigned long stream,
    unsigned long epoch, unsigned long window, unsigned long expected, unsigned long deadline_ms);
/* Record one trusted zero-based part identity and host-relative monotonic elapsed_ms.
 * valid must be VALID or INVALID. A damaged packet cannot supply its own trusted identity:
 * use an authenticated outer envelope or leave it missing. Repeats never increase distinct
 * counts. Usable on-time wins over late/damaged; late wins over damaged; repair can upgrade.
 * Never infer clock time from packet presentation timestamps. Invalid args INVALID,
 * regressing time or a closed window CONFLICT; every failure preserves state. */
FVM_API FVM_SResult fvm_smeasure_record(FVM_SMeasure *, unsigned long part,
    unsigned int valid, unsigned long elapsed_ms);
/* Close once, at/after deadline and last arrival; fill owned copied report atomically.
 * RTT known is 0/1; unknown requires zero RTT. No automatic probe or RTT inference.
 * Premature/regressing/already closed CONFLICT; invalid RTT/args INVALID. Later repair
 * cannot improve this historical report. Missing = expected-on_time-late-damaged. */
FVM_API FVM_SResult fvm_smeasure_finish(FVM_SMeasure *, unsigned long elapsed_ms,
    unsigned long rtt_known, unsigned long rtt_ms, FVM_SAdaptFeedback *);
#ifdef __cplusplus
}
#endif
#endif
