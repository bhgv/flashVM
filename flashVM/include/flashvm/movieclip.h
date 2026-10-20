/* FlashVM: Public embedding API declarations: movieclip.
 * File: include/flashvm/movieclip.h.
 */

#ifndef FLASHVM_MOVIECLIP_H
/* Define FLASHVM MOVIECLIP H constant or compilation switch. */
#define FLASHVM_MOVIECLIP_H
#include "value.h"
#include "raster.h"
#include "path.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Host event notification at actual root frame entry, before its DoAction blocks.
     * Ordinal selects the matching context SWF's root StartSound. Return a VM status.
     * No callback pointers/userdata cross serialization; host must register on the receiving VM. */
    typedef FVM_Result (*FVM_MovieClipSoundCallback)(FVM_Context *, FVM_Object *, size_t, void *);
    /* Borrow a synchronous host callback/userdata; null disables delivery, not sound validation.
     * No device is opened. A sound frame without a callback returns UNSUPPORTED. */
    FVM_API FVM_Result fvm_movieclip_set_sound_callback(FVM_Context *, FVM_MovieClipSoundCallback,
                                                        void *);
    /* Neutral SWF document metadata; strings are borrowed from the rooted clip graph. */
    typedef struct FVM_SWFDocumentInfo
    {
        unsigned long struct_size; /* Exact structure size required by this API. */
        int file_attributes_present; /* Distinguish absent attributes from explicit zero flags. */
        unsigned long file_attributes; /* Supported flags are zero or HasMetadata (0x10). */
        FVM_Value metadata; /* Undefined if absent; otherwise counted context-owned UTF-8. */
    } FVM_SWFDocumentInfo;
    /* Read without script execution; errors leave output unchanged. [GC:] Root the clip or retain
       the returned string before collection. Metadata is not parsed or executed as XML. */
    FVM_API FVM_Result fvm_movieclip_swf_document_info(FVM_Context *, FVM_Object *,
                                                       FVM_SWFDocumentInfo *);
    /* Create movieclip. */
    FVM_API FVM_Result fvm_movieclip_create(FVM_Context *, const char *name, unsigned int frames,
                                            FVM_Object **);
    /* Import root/sprite DoAction/FrameLabel timelines and ExportAssets from SWF 1..8.
       FWS/CWS decoding is bounded; unimplemented scene/resource tags return FVM_UNSUPPORTED.
       Loading runs no actions. Exported sprites can be instantiated through attachMovie.
       On failure *out is null. [GC:] The returned clip and resource library must be rooted by their
       host owner. */
    FVM_API FVM_Result fvm_movieclip_load_swf(FVM_Context *, const void *data, size_t size,
                                              const char *name, FVM_Object **out);
    /* Query the local affine transform in pixels, preserving authored shear/reflection. */
    FVM_API FVM_Result fvm_movieclip_get_matrix(FVM_Context *, FVM_Object *, FVM_Affine *);
    /* Atomically set a finite local transform and corresponding display components. */
    FVM_API FVM_Result fvm_movieclip_set_matrix(FVM_Context *, FVM_Object *, const FVM_Affine *);
    /* Copy a validated AVM1 frame action and optional label. Frames start at 1. */
    /* Define movieclip frame. */
    FVM_API FVM_Result fvm_movieclip_define_frame(FVM_Context *, FVM_Object *, unsigned int,
                                                  const char *label, const void *actions, size_t);
    /* Owner-thread synchronous ticks, including onEnterFrame on stopped/single-frame clips.
       Optional handlers use receiver/defining-owner method lookup; missing/noncallable is ignored.
       All ticks, callback AVM1 and charged native traversal share the outer instruction/work limit
       (zero disables the numeric limit, not cooperative worker cancellation). On error/OOM/exception
       completed actions, playhead changes and callbacks remain committed; later work is skipped.
       After parent delivery, capture child identities and sort ascending native depth under budget.
       Later same-depth replacements/newborns wait for the next snapshot; depth changes do not reorder it.
       Validate local snapshots before child callbacks; detached/moved children are skipped.
       A malformed/aliased/foreign or oversized child table fails after any committed parent work.
       No rendering or timer polling occurs. [GC:] The native barrier retains the traversed graph;
       explicit collect returns INVALID_ARGUMENT and gc_step does no collection until return.
       Null/foreign clips, lifecycle phases, sound
       callback reentry and excessive tick counts return INVALID_ARGUMENT. Zero ticks does no work. */
    FVM_API FVM_Result fvm_movieclip_advance(FVM_Context *, FVM_Object *, unsigned int ticks);
    /* Render owner-thread geometry into the caller-owned raster surface (no timeline tick).
       Children use ascending signed depth and a bounded local identity snapshot after parent
       drawing. Replacements/newborns from child getters wait; detached children are skipped.
       Each node accepts at most 65536 child slots and 64 descendant levels. Invalid/foreign/
       aliased private children return INVALID_ARGUMENT; prior pixels/getter effects remain.
       OUT_OF_MEMORY and getter errors propagate; disabled raster backends may return UNSUPPORTED.
       Sorting uses O(n log n) time and O(n) scratch without consuming instruction units.
       [GC:] The native barrier retains captured objects; caller owns surface storage throughout.
       See doc/RASTER_DEPTH_ORDER.md for lifetime, partial completion and limitations. */
    FVM_API FVM_Result fvm_movieclip_render(FVM_Context *, FVM_Object *, FVM_RasterSurface *);
    /* Render through a finite host viewport without changing script-visible clip transforms.
       Viewport applies once to every world-space node, preserving root visibility/color. */
    FVM_API FVM_Result fvm_movieclip_render_view(FVM_Context *, FVM_Object *, FVM_RasterSurface *,
                                                const FVM_Affine *viewport);
    /* Explicit import policy; no other flag bits are accepted. */
#define FVM_SWF_IMPORT_MUTE_ROOT_STREAMS 1UL /* Validate and omit root streaming sound controls. */
    /* Import with optional root-stream muting; zero flags retain load_swf behavior.
     * Muting requires input to match the context's current fvm_load_memory source
     * after FWS/CWS decoding, so root stream validation uses the exact same bytes.
     * Validates header ordering, frame/count bounds and PCM block alignment before
     * scene allocation; does not decode compressed sound or support sprite streams.
     * Event sounds retain their existing explicit host callback policy. Runs no actions.
     * On failure *out is null. Owner thread, outside GC/teardown. [GC:] Root returned clip. */
    FVM_API FVM_Result fvm_movieclip_load_swf_ex(FVM_Context *, const void *, size_t,
                                                 const char *, unsigned long flags, FVM_Object **out);
#ifdef __cplusplus
}
#endif
#endif
