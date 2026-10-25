/* Multipurpose tag-zero service extension: bounded per-tag size offer/accept exchanges. */
#ifndef FLASHVM_SCONTROL_H
#define FLASHVM_SCONTROL_H         /* Guard the allocation-free C89 service negotiation API. */
#include "sslot.h"
#define FVM_SCONTROL_VERSION 2UL   /* Distinct from legacy version-1 tag-binding payloads. */
#define FVM_SCONTROL_OFFER 1UL     /* Propose limits for one direction-bound nonzero tag. */
#define FVM_SCONTROL_ACCEPT 2UL    /* Correlated accepted limits; never enlarge an offer. */
#define FVM_SCONTROL_OFFER_AT 3UL  /* Propose a bulk profile at a future media object. */
#define FVM_SCONTROL_ACCEPT_AT 4UL /* Echo the exact proposed activation boundary. */
#define FVM_SCONTROL_FIELDS \
    11U                            /* Eight generic fields plus optional stream, epoch and first object. */
#define FVM_SCONTROL_FRAMING 32UL  /* Conservative record header reserve within fixed slots. */
#define FVM_SCONTROL_METADATA \
    64UL                           /* Conservative typed part metadata reserve within record_limit. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Limits are caller-owned native metadata; never serialize this structure's native layout. */
typedef struct FVM_SControlLimits
{
    unsigned long
        tag;                    /* Nonzero Uint32 local tag, already bound to a type in this direction. */
    unsigned long record_limit; /* Maximum tag+payload bytes, 1..SRECORD_BODY_MAX. */
    unsigned long
        slot_bytes;             /* Zero for compact records; otherwise negotiated fixed bulk extent. */
    unsigned long
        part_bytes;             /* Zero for compact mode; otherwise raw part quota inside the slot. */
} FVM_SControlLimits;


/* Optional media activation boundary; all zero for quiesced generic tag traffic. */
typedef struct FVM_SControlBoundary
{
    unsigned long stream; /* Nonzero Uint32 media stream for boundary-aware negotiation. */
    unsigned long epoch;  /* Nonzero Uint32 stream incarnation; late old traffic is rejected. */
    unsigned long object; /* First new-profile object; the host chooses a future segment. */
} FVM_SControlBoundary;


/* One checked service message; new command IDs may extend the tag-zero service in later
     * versions. */
typedef struct FVM_SControlMessage
{
    unsigned long command;         /* OFFER/ACCEPT or their boundary-aware AT variants. */
    unsigned long id;              /* Nonzero Uint32 correlation ID, never reused in an exchange lifetime. */
    FVM_SControlLimits limits;     /* Proposed or accepted per-tag extents. */
    FVM_SControlBoundary boundary; /* Exact agreed activation point, or zero triple. */
} FVM_SControlMessage;


/* One direction/tag exchange; keep sender and receiver state separate and serialize ownership.
     */
typedef struct FVM_SControlExchange
{
    size_t struct_size;                    /* Initialized exact ABI size; fields private after init. */
    FVM_SControlLimits ceiling;            /* Local immutable native buffer/policy maxima. */
    FVM_SControlLimits pending;            /* Offered or selected values, inactive until commit. */
    FVM_SControlLimits active;             /* Last committed values, meaningful only with active_id. */
    unsigned long
        last_id;                           /* Greatest initiated/responded ID; cancellation does not permit reuse. */
    unsigned long pending_id;              /* Current in-flight correlation, zero when idle. */
    unsigned long active_id;               /* Last completed correlation, zero before first agreement. */
    unsigned long role;                    /* OFFER for initiator, ACCEPT for responder, zero when idle. */
    FVM_SControlBoundary pending_boundary; /* Inactive activation point until correlated ACK. */
    FVM_SControlBoundary
        active_boundary;                   /* First object using active limits; zero for generic. */
} FVM_SControlExchange;


/* Encode/decode an ordinary compact tag-zero S-record. No padding or implicit state mutation.
     * First payload integer 2 selects this service; version 1 remains the legacy bindings codec.
     * Unknown service version/command UNKNOWN, malformed known data INVALID, CRC errors forwarded.
     * reserved must be zero. Outputs unchanged on failure, read used counts one record only. */
FVM_API FVM_SResult fvm_scontrol_write(const FVM_SControlMessage *, void *, size_t,
                                       size_t *used);


FVM_API FVM_SResult fvm_scontrol_read(const void *, size_t, FVM_SControlMessage *,
                                      size_t *used);


/* Copy local maxima; compact ceiling has zero slot/part, fixed ceiling may also accept compact
     * offers. Native host must really provide these capacities and authenticate service messages.
     */
FVM_API FVM_SResult fvm_scontrol_init(FVM_SControlExchange *,
                                      const FVM_SControlLimits *ceiling);


/* Start one strictly increasing ID; reject overlapping negotiations CONFLICT and excess local
     * limits LIMIT. Output is an OFFER to encode/send, never immediate activation. */
FVM_API FVM_SResult fvm_scontrol_propose(FVM_SControlExchange *, unsigned long id,
                                         const FVM_SControlLimits *,
                                         FVM_SControlMessage *offer);


/* Select bounded limits from an authenticated offer and local maxima, store pending selection,
     * return ACCEPT. Unsupported fixed mode UNKNOWN. No active configuration changes yet. */
FVM_API FVM_SResult fvm_scontrol_respond(FVM_SControlExchange *,
                                         const FVM_SControlMessage *offer,
                                         FVM_SControlMessage *accept);


/* Initiator commits only its correlated bounded ACCEPT; responder commits its exact selection
     * after queuing that ACK on the reliable control channel. Host quiesces old payload traffic
     * before switching framing. Exact committed ACK repeats are idempotent. No I/O, timers or
     * authentication. */
FVM_API FVM_SResult fvm_scontrol_commit(FVM_SControlExchange *,
                                        const FVM_SControlMessage *accept);


/* Abandon pending negotiation, preserve active limits and retired IDs. Late ACK cannot commit.
     * Host must reconcile an uncertain peer state before resuming payload, or start a new session.
     */
FVM_API FVM_SResult fvm_scontrol_cancel(FVM_SControlExchange *);


/* Boundary-aware offer: nonzero stream/epoch/first object uses OFFER_AT/ACCEPT_AT. The ACK
     * must echo the complete boundary exactly, not silently move it. All-zero boundary selects
     * the generic offer behavior. Host chooses a future object and drains/retires old receiver
     * storage without pausing the overall timeline. Sizes may increase or decrease within ceiling.
     */
FVM_API FVM_SResult fvm_scontrol_propose_at(FVM_SControlExchange *, unsigned long id,
                                            const FVM_SControlLimits *,
                                            const FVM_SControlBoundary *,
                                            FVM_SControlMessage *offer);


/* Copy the committed limits/boundary without exposing mutable exchange internals to adapters.
     * UNKNOWN before any agreement; INVALID for bad native state/outputs. Outputs remain unchanged
     * on error and must be disjoint from each other and the exchange. Owner-serialized, no I/O. */
FVM_API FVM_SResult fvm_scontrol_active(const FVM_SControlExchange *, FVM_SControlLimits *,
                                        FVM_SControlBoundary *);
#ifdef __cplusplus
}
#endif
#endif
