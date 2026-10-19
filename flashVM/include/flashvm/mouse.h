/* FlashVM: explicit primary-pointer notifications for desktop, stylus and touch hosts. */
#ifndef FLASHVM_MOUSE_H
#define FLASHVM_MOUSE_H /* Guard the public C89 Mouse listener ABI. */
#include "flashvm.h"
#define FVM_MOUSE_CURSOR_MIN_SWF_VERSION 5U /* First supported Mouse.hide/show script version. */
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
/* Registration and dispatch share the context work budget with nested AVM1.
 * Exhaustion/cancellation returns FVM_ERROR; preparation preserves published state,
 * while delivery failure preserves new input and completed callbacks. */
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
/* Read the context's requested cursor visibility (initially 1). Require non-null output.
 * Owner-thread only, outside GC/release/teardown. Invalid calls preserve output.
 * No allocation, script, device query or guarantee that a physical cursor exists. */
FVM_API FVM_Result fvm_input_get_cursor_visible(FVM_Context *, int *visible);
/* Request visible=0/1 and return the previous request through mandatory previous output.
 * Invalid arguments preserve state/output; no allocation/callback/OS operation occurs.
 * A host applies the queried request to its own window on its native owner thread.
 * Position, button, focus, listener registrations and pending input remain unchanged. */
FVM_API FVM_Result fvm_input_set_cursor_visible(FVM_Context *, int visible, int *previous);
/* Atomically install Mouse.hide/show for SWF5--8 and the SWF6--8 listener subset after standard
 * objects. Listener members are present but return UNSUPPORTED in SWF5, including preinstalled
 * namespaces. Conflicts are preserved. No physical cursor is owned; does not install Key/Selection. */
FVM_API FVM_Result fvm_standard_mouse_install(FVM_Context *);
#ifdef __cplusplus
}
#endif
#endif
