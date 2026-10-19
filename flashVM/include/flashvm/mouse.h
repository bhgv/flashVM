/* FlashVM: explicit primary-pointer notifications for desktop, stylus and touch hosts. */
#ifndef FLASHVM_MOUSE_H
#define FLASHVM_MOUSE_H /* Guard the public C89 Mouse listener ABI. */
#include "flashvm.h"
#define FVM_MOUSE_MIN_SWF_VERSION 6U /* First supported Mouse listener namespace. */
#define FVM_MOUSE_MAX_LISTENERS 1024U /* Maximum unique registered objects per context. */
#ifdef __cplusplus
extern "C" {
#endif
/* Each event is explicit: a touch host maps its chosen primary contact to down/move/up. */
typedef enum FVM_MouseEventType
{
    FVM_MOUSE_EVENT_MOVE = 1, /* Pointer moved, with or without a held primary button. */
    FVM_MOUSE_EVENT_DOWN = 2, /* Primary button/contact began. */
    FVM_MOUSE_EVENT_UP = 3    /* Primary button/contact ended. */
} FVM_MouseEventType;
/* [GC:] Retain a same-context listener once; duplicate registration succeeds unchanged. */
FVM_API FVM_Result fvm_input_add_mouse_listener(FVM_Context *, FVM_Object *listener);
/* Remove by identity; require removed, set it to 0/1 on success and preserve it on error. */
FVM_API FVM_Result fvm_input_remove_mouse_listener(FVM_Context *, FVM_Object *listener,
                                                  int *removed);
/* Deliver one event synchronously on the VM owner thread, using finite world/stage pixels.
 * Snapshot OOM preserves position; after publication drag/handler errors keep position.
 * Nested Mouse dispatch is rejected. Listener edits affect later events; no device is opened. */
FVM_API FVM_Result fvm_input_dispatch_mouse(FVM_Context *, FVM_MouseEventType,
                                           double world_x, double world_y);
/* Deliver a finite signed wheel delta in host-normalized scroll lines (+up, -down),
 * at finite world coordinates. All finite deltas, including zero, are explicit events.
 * Publish position/update drag, then call onMouseWheel(delta) on rooted Mouse listeners.
 * Same lifecycle, snapshot/OOM, error and cross-event recursion rules as pointer dispatch.
 * No hit-test target is supplied, no focus/scroll action or OS wheel scaling is inferred. */
FVM_API FVM_Result fvm_input_dispatch_mouse_wheel(FVM_Context *, double delta,
                                                 double world_x, double world_y);
/* Atomically install the SWF6--8 addListener/removeListener/_listeners subset after standard
 * objects. Existing conflicting bindings are preserved. Does not install Key or Selection. */
FVM_API FVM_Result fvm_standard_mouse_install(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
