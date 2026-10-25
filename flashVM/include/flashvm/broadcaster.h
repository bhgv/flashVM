/* FlashVM: explicit generic ActionScript event broadcasters, independent of host input. */
#ifndef FLASHVM_BROADCASTER_H
#define FLASHVM_BROADCASTER_H                /* Guard the C89 opt-in installation API. */
#include "flashvm.h"
#define FVM_BROADCASTER_MIN_SWF_VERSION 6U   /* First documented AsBroadcaster player version. */
#define FVM_BROADCASTER_MAX_LISTENERS 4096UL /* Bound stored registrations and dispatch snapshots. */
#ifdef __cplusplus
extern "C"
{
#endif


/* Install the SWF6--8/host AsBroadcaster namespace after fvm_standard_install.
 * Owner-thread only, outside GC/release/teardown. Existing conflicts are preserved.
 * Repeated intact installation succeeds. Does not replace Key/Mouse/Selection lists. */
FVM_API FVM_Result fvm_standard_broadcaster_install(FVM_Context *);


/* [GC:] Initialize a same-context object with native add/remove/broadcast methods and
 * a read-only _listeners binding to a live Array. Reinitializing an intact broadcaster
 * clears its registrations. Conflicting own slots reject without partial mutation.
 * Requires standard objects. Global binding validation may run getters; subsequent
 * preparation runs no callback. Preparation OOM preserves the receiver slots. */
FVM_API FVM_Result fvm_broadcaster_initialize(FVM_Context *, FVM_Object *);
#ifdef __cplusplus
}
#endif
#endif
