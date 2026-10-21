/* Bounded feedback-driven bulk profile selection; no socket, clock, pacing or VM ownership. */
#ifndef FLASHVM_SADAPT_H
#define FLASHVM_SADAPT_H /* Guard the standalone C89 adaptation policy and feedback service. */
#include "scontrol.h"
#define FVM_SADAPT_PROFILES 8U              /* Maximum copied increasing bulk-profile ladder. */
#define FVM_SADAPT_NONE FVM_SADAPT_PROFILES /* No pending candidate. */
#define FVM_SADAPT_KEEP 0U         /* No profile negotiation requested by this observation. */
#define FVM_SADAPT_SHRINK 1U       /* Propose one smaller ladder entry. */
#define FVM_SADAPT_GROW 2U         /* Propose one larger ladder entry. */
#define FVM_SADAPT_INSUFFICIENT 3U /* Accepted window below configured sample minimum. */
#define FVM_SADAPT_LOSS 1UL        /* Unusable-by-deadline percentage crossed the bad threshold. */
#define FVM_SADAPT_LATENCY 2UL     /* Known RTT crossed the high-latency threshold. */
#define FVM_SADAPT_STABLE 4UL      /* Consecutive low-loss/known-low-RTT windows permit growth. */
#define FVM_SADAPT_COOLDOWN 8UL    /* A full feedback window consumed a post-decision holdoff. */
#define FVM_SADAPT_PERCENT                                                                         \
    100UL /* Integer percent scale, compared without Uint32 multiplication overflow. */
#define FVM_SADAPT_SERVICE                                                                         \
    3UL /* Tag-zero feedback subservice; bindings=1 and limits=2 remain separate. */
#define FVM_SADAPT_REPORT 1UL /* One authenticated host-measured window report. */
#define FVM_SADAPT_FIELDS                                                                          \
    11U /* Service, command, stream, epoch, window, expected, on_time, late, damaged, known, rtt.  \
         */
#define FVM_SADAPT_BAD_PERCENT                                                                     \
    10UL /* Illustrative default threshold; host must choose link policy. */
#define FVM_SADAPT_GOOD_PERCENT                                                                    \
    1UL /* Illustrative growth loss ceiling, distinct from shrink threshold. */
#define FVM_SADAPT_BAD_RTT 250UL    /* Illustrative high RTT in milliseconds. */
#define FVM_SADAPT_GOOD_RTT 50UL    /* Illustrative low RTT in milliseconds. */
#define FVM_SADAPT_BAD_WINDOWS 2UL  /* Require repeated bad windows before a smaller proposal. */
#define FVM_SADAPT_GOOD_WINDOWS 4UL /* Require a longer stable run before a larger proposal. */
#define FVM_SADAPT_COOLDOWN_WINDOWS 2UL /* Full windows ignored after commit/reject. */
#define FVM_SADAPT_MIN_EXPECTED                                                                    \
    64UL /* Illustrative minimum useful window; no implicit aggregation. */
#ifdef __cplusplus
extern "C"
{
#endif
    /* Exclusive counts measured by an authenticated host, not by trusting a remote CRC alone. */
    typedef struct FVM_SAdaptFeedback
    {
        unsigned long stream; /* Nonzero direction/stream identity, fixed in policy config. */
        unsigned long
            epoch; /* Nonzero incarnation; old-session reports cannot tune a new stream. */
        unsigned long window;   /* Strictly increasing nonzero Uint32 observation ID. */
        unsigned long expected; /* Known expected parts before the observation deadline, >0. */
        unsigned long on_time;  /* Distinct usable parts accepted before that deadline. */
        unsigned long late;     /* Distinct parts arriving late, excluding on_time/damaged. */
        unsigned long
            damaged; /* Distinct rejected parts, excluding on_time/late; missing is inferred. */
        unsigned long rtt_known; /* Exactly zero or one; unknown RTT requires rtt_ms=0. */
        unsigned long
            rtt_ms; /* Measured RTT in milliseconds, Uint32; never an implicit clock sample. */
    } FVM_SAdaptFeedback;
    /* Initialization descriptor borrows ladder only during init; state copies all selected entries.
     */
    typedef struct FVM_SAdaptConfig
    {
        size_t struct_size; /* Exact native ABI initialized by config_init. */
        const FVM_SControlLimits
            *profiles;  /* Strictly increasing slot/part sizes, same nonzero tag. */
        size_t count;   /* One through PROFILES supported bulk geometries. */
        size_t initial; /* Currently committed ladder index, not a speculative proposed profile. */
        unsigned long stream, epoch; /* Expected report identity; both nonzero Uint32. */
        unsigned long bad_percent, good_percent; /* 1..100 bad, 0..bad-1 good. */
        unsigned long bad_rtt_ms, good_rtt_ms;   /* Nonzero bad RTT, good strictly below bad. */
        unsigned long bad_windows,
            good_windows;               /* Positive Uint32 consecutive-window requirements. */
        unsigned long cooldown_windows; /* Uint32 full-window holdoff after commit or rejection. */
        unsigned long
            min_expected; /* Positive Uint32 expected count; smaller windows break streaks. */
    } FVM_SAdaptConfig;
    /* Owned copied proposal hint: host must negotiate it before committing policy state. */
    typedef struct FVM_SAdaptDecision
    {
        unsigned int action;       /* KEEP, SHRINK, GROW or INSUFFICIENT. */
        unsigned long reasons;     /* LOSS/LATENCY/STABLE/COOLDOWN evidence for this observation. */
        size_t current, candidate; /* Current index and proposed index, or current for KEEP. */
        FVM_SControlLimits
            limits; /* Copied candidate geometry, suitable for a later control offer. */
    } FVM_SAdaptDecision;
    /* Single-owner native policy; fields private after init, no allocation or borrowed VM pointers.
     */
    typedef struct FVM_SAdapt
    {
        size_t struct_size; /* Exact ABI size. */
        FVM_SAdaptConfig
            config; /* Copied policy; profiles points into this state's own copied table. */
        FVM_SControlLimits profiles[FVM_SADAPT_PROFILES]; /* Fixed bounded supported ladder. */
        size_t current, pending;         /* Committed index and optional in-flight proposal. */
        unsigned long last_window;       /* Greatest consumed report ID; never wraps/recycles. */
        unsigned long bad_run, good_run; /* Bounded consecutive qualifying windows. */
        unsigned long cooldown;          /* Remaining full observation windows in holdoff. */
    } FVM_SAdapt;
    /* Fill illustrative defaults without allocation; caller supplies identity, ladder and initial
     * index. */
    FVM_API FVM_SResult fvm_sadapt_config_init(FVM_SAdaptConfig *);
    /* Validate/copy complete ladder and policy; errors preserve state and borrow nothing. State
     * must remain at a stable address after init (config.profiles references its internal table).
     */
    FVM_API FVM_SResult fvm_sadapt_init(FVM_SAdapt *, const FVM_SAdaptConfig *);
    /* Consume one validated report atomically. Duplicate/old/wrong identity or pending proposal
     * CONFLICT; malformed counts INVALID. Unknown RTT forbids growth. Missing+late+damaged all
     * count as unusable by deadline. INSUFFICIENT resets streaks but does not spend cooldown.
     * Errors leave state/decision unchanged. A proposal never changes current geometry or sends
     * control traffic. */
    FVM_API FVM_SResult fvm_sadapt_observe(FVM_SAdapt *, const FVM_SAdaptFeedback *,
                                           FVM_SAdaptDecision *);
    /* Confirm the exact pending ladder geometry only after the external control ACK committed.
     * Clamped/different ACK CONFLICT; host rejects/rebuilds ladder for unsupported selections.
     * Successful commit starts holdoff; no payload/frame reinterpretation is performed here. */
    FVM_API FVM_SResult fvm_sadapt_commit(FVM_SAdapt *, const FVM_SControlLimits *selected);
    /* Abandon a pending proposal after control rejection/cancel; preserve current, start holdoff.
     */
    FVM_API FVM_SResult fvm_sadapt_reject(FVM_SAdapt *);
    /* Compact tag-zero report codec, no padding. Unknown subservice/command UNKNOWN; bad metadata
     * INVALID, checksum/prefix errors forwarded. Outputs atomic; read consumes one record only. */
    FVM_API FVM_SResult fvm_sadapt_feedback_write(const FVM_SAdaptFeedback *, void *, size_t,
                                                  size_t *used);
    FVM_API FVM_SResult fvm_sadapt_feedback_read(const void *, size_t, FVM_SAdaptFeedback *,
                                                 size_t *used);
#ifdef __cplusplus
}
#endif
#endif
