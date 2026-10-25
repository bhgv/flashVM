/* Copied player observations for resource/file clients; no live VM pointer crosses this ABI. */
#ifndef FVM_DESKTOP_RESOURCE_STATE_H
#define FVM_DESKTOP_RESOURCE_STATE_H /* Guard the additive C89 player observation interface. */
#include "playback.h"
#include "flashvm/revents.h"

#define FVM_DESKTOP_STATE_VERSION 1UL            /* Canonical S-integer payload schema. */
#define FVM_DESKTOP_STATE_SNAPSHOT 1UL           /* READ operation, empty arguments. */
#define FVM_DESKTOP_STATE_READY 1UL              /* A new root was imported; epoch advances. */
#define FVM_DESKTOP_STATE_TICK 2UL               /* A completed timeline tick and presentation. */
#define FVM_DESKTOP_STATE_POLICY 3UL             /* A TURN observed a pause/step policy change. */
#define FVM_DESKTOP_STATE_SOURCE 255U            /* Copied source-prefix byte limit excluding NUL. */
#define FVM_DESKTOP_STATE_SOURCE_TRUNCATED 1UL   /* Source has a longer byte suffix. */
#define FVM_DESKTOP_STATE_COUNTERS_TRUNCATED 2UL /* At least one counter saturated to Uint32. */
#define FVM_DESKTOP_STATE_FLAGS 3UL              /* All currently defined snapshot flags. */
#define FVM_DESKTOP_STATE_FIELDS 10U             /* Integer fields preceding counted source bytes. */
#ifdef __cplusplus
extern "C"
{
#endif
/* Independent native copy, usable after the player/context/root have been released. */
typedef struct FVM_DesktopResourceSnapshot
{
    unsigned long epoch;         /* Nonzero root incarnation in this adapter lifetime, never wraps. */
    unsigned long phase;         /* READY, TURN or TICK at the captured owner boundary. */
    unsigned long paused;        /* Exactly zero or one: observed policy, not a predicted command result. */
    unsigned long step_pending;  /* Exactly zero or one: observed pending permit. */
    unsigned long movie_ticks;   /* Completed current-root ticks, Uint32 with explicit saturation. */
    unsigned long total_ticks;   /* Completed whole-run ticks, Uint32 with explicit saturation. */
    unsigned long presentations; /* Current-root completed draws, including paused refreshes. */
    unsigned long flags;         /* SOURCE_TRUNCATED and/or COUNTERS_TRUNCATED. */
    char source[FVM_DESKTOP_STATE_SOURCE + 1]; /* Counted byte prefix, always native NUL-terminated. */
} FVM_DesktopResourceSnapshot;
typedef struct FVM_DesktopResourceState FVM_DesktopResourceState; /* Opaque native owner adapter. */


/* Owner allocates/registers READ snapshot under path; borrows dispatcher through destroy.
     * Outputs only on OK. No SDK startup, VM borrow, grant or hook installation. Host grants READ
     * and/or SUBSCRIBE explicitly; adapter must not outlive its dispatcher. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_state_create(FVM_RDispatch *,
                                                              const char *path,
                                                              FVM_DesktopResourceState **,
                                                              FVM_RHandle *);


/* Owner calls only inside a legitimate playback notify, before delivering its next command.
     * Copies scalars/source, never context/root. READY advances epoch; TURN/TICK before READY BUSY.
     * READY/TICK always publish; TURN publishes only pause/step changes. Cache commits only after
     * successful publication; failure preserves cache and returns status, with no VM rollback.
     * This function never pumps requests or changes player policy. Epoch/sequence exhaustion LIMIT.
     */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_state_observe(FVM_DesktopResourceState *,
                                                               const FVM_DesktopPlaybackState *);


/* Decode exactly one snapshot payload (inside an event or resource reply), not its S envelope.
     * Canonical S integers and counted source bytes; malformed/unknown schema INVALID, output
     * intact. Client-side operation, no VM call or borrowed storage. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_state_read(const void *, size_t, FVM_DesktopResourceSnapshot *);


/* Owner quiesces notifications/consumers before destroy. Running resource callback BUSY;
     * success unregisters old incarnation, discards watches, frees only native adapter and nulls
     * pointer. Queued old requests subsequently complete UNKNOWN; host still owns
     * dispatcher/session cleanup. */
FVM_DESKTOP_API FVM_RResult fvm_desktop_resource_state_destroy(FVM_DesktopResourceState **);
#ifdef __cplusplus
}
#endif
#endif
