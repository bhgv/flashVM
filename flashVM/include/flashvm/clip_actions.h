/* FlashVM: explicit owner-thread delivery of retained SWF PlaceObject2 ClipActions. */
#ifndef FLASHVM_CLIP_ACTIONS_H
#define FLASHVM_CLIP_ACTIONS_H /* Public ClipActions constants and C89 ABI guard. */
#include "value.h"
#define FVM_CLIP_EVENT_LOAD 0x00001UL /* Clip loading notification, explicitly driven by the host. */
#define FVM_CLIP_EVENT_ENTER_FRAME 0x00002UL /* Timeline tick; also delivered by movieclip_advance. */
#define FVM_CLIP_EVENT_UNLOAD 0x00004UL /* Explicit notification; does not detach or destroy a clip. */
#define FVM_CLIP_EVENT_MOUSE_MOVE 0x00008UL /* Selected clip pointer movement, without hit testing. */
#define FVM_CLIP_EVENT_MOUSE_DOWN 0x00010UL /* Selected clip primary-button press. */
#define FVM_CLIP_EVENT_MOUSE_UP 0x00020UL /* Selected clip primary-button release. */
#define FVM_CLIP_EVENT_KEY_DOWN 0x00040UL /* Selected clip key transition; Key state is not updated. */
#define FVM_CLIP_EVENT_KEY_UP 0x00080UL /* Selected clip key release; Key state is not updated. */
#define FVM_CLIP_EVENT_DATA 0x00100UL /* Explicit data notification; no loader or network operation. */
#define FVM_CLIP_EVENT_INITIALIZE 0x00200UL /* SWF6+ explicit initialization notification. */
#define FVM_CLIP_EVENT_PRESS 0x00400UL /* SWF6+ selected button-like clip press. */
#define FVM_CLIP_EVENT_RELEASE 0x00800UL /* SWF6+ selected button-like clip release inside. */
#define FVM_CLIP_EVENT_RELEASE_OUTSIDE 0x01000UL /* SWF6+ release outside a host-selected clip. */
#define FVM_CLIP_EVENT_ROLL_OVER 0x02000UL /* SWF6+ selected pointer entry. */
#define FVM_CLIP_EVENT_ROLL_OUT 0x04000UL /* SWF6+ selected pointer exit. */
#define FVM_CLIP_EVENT_DRAG_OVER 0x08000UL /* SWF6+ selected held-pointer entry. */
#define FVM_CLIP_EVENT_DRAG_OUT 0x10000UL /* SWF6+ selected held-pointer exit. */
#define FVM_CLIP_EVENT_KEY_PRESS 0x20000UL /* SWF6+ exact raw SWF key-code match. */
#define FVM_CLIP_EVENT_CONSTRUCT 0x40000UL /* SWF7+ explicit construction notification. */
#ifdef __cplusplus
extern "C" {
#endif
/* Execute matching retained bytecode records in source order for exactly one event bit.
 * key_code is 0..255 for KEY_PRESS and must be zero otherwise. No native onX method,
 * input-state update, hit test, automatic load/unload, timeline advance or render occurs.
 * Borrow a same-context MovieClip on its owner thread; host must retain it between calls.
 * [GC:] A native barrier prevents collection during the retained snapshot's execution.
 * All records and AVM1 share the outer execution budget; worker stops are cooperative.
 * Missing records succeed. Errors/OOM/exceptions preserve completed effects and skip the rest.
 * Null/foreign/nonclip, lifecycle/sound phases, invalid event/key or malformed retained data
 * return INVALID_ARGUMENT. Events unavailable in the clip's SWF version return UNSUPPORTED.
 * Calling again is a new delivery, not continuation or replay of a stored dispatch cursor. */
FVM_API FVM_Result fvm_movieclip_dispatch_clip_actions(FVM_Context *, FVM_Object *,
                                                       unsigned long event, unsigned int key_code);
/* Broadcast one global retained input event through a selected root's native descendants.
 * Accept only MOUSE_MOVE/DOWN/UP or KEY_DOWN/UP. No key selector or native onX delivery.
 * Host publishes pointer/keyboard state separately before calling; visibility and focus do not
 * filter these global records. No Press/Roll/Drag hit testing, wheel or lifecycle scheduling.
 * Prepare recipients in root-first native depth order before callbacks. New children wait for
 * the next broadcast; removed/reparented captured branches are skipped before later delivery.
 * Each receiver prepares its own selector/code snapshot at its turn, so earlier callbacks may
 * replace a later receiver's handler table. Older-than-SWF5 nodes are traversed without handlers.
 * Borrow/pin the same-context root on its owner thread. [GC:] Collection is barred throughout.
 * One work budget covers capture, ancestry checks and retained bytecode; stop is cooperative.
 * Invalid root/type/phase/event/reentry or malformed/cyclic topology -> INVALID_ARGUMENT;
 * exhausted work/worker stop -> ERROR; OOM -> OUT_OF_MEMORY. Depth/recipient bounds reject
 * preparation before callbacks. Later errors preserve completed callbacks and input state.
 * The context-local reentry flag and active recipient batch are not serialized. */
FVM_API FVM_Result fvm_movieclip_dispatch_clip_input(FVM_Context *, FVM_Object *root,
                                                    unsigned long event);
/* Broadcast the same five global input events, including optional MovieClip methods.
 * Uses the retained-only API's topology, work, lifetime, phase and error contract.
 * For each receiver: run retained ClipActions, recheck captured ancestry/removal, then
 * resolve onMouseMove/Down/Up or onKeyDown/Up once with this=receiver and no arguments.
 * Named methods run for source SWF6–8 and host-created version zero, not source SWF1–5.
 * Missing/noncallable members succeed; inherited/accessor/resolver methods use ordinary
 * method lookup with no global-function fallback. Function-valued handlers preserve their
 * defining owner; native entries keep the ordinary native-call contract. Lookup happens
 * at each turn, after retained records; mutations can affect this and later receivers.
 * Methods/getters allow updateAfterEvent; this does not advance or render a frame.
 * Visibility, playback and focus do not filter these global notifications. No native
 * namespace listeners, input-state publication, targeted events or lifecycle are added.
 * Retained-only callers keep fvm_movieclip_dispatch_clip_input instead. */
FVM_API FVM_Result fvm_movieclip_dispatch_input(FVM_Context *, FVM_Object *root,
                                               unsigned long event);
#ifdef __cplusplus
}
#endif
#endif
