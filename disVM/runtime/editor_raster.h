/* Native, VM-independent editor frame ownership; prepared by the UI owner. */
#ifndef DVM_EDITOR_RASTER_H
#define DVM_EDITOR_RASTER_H
#include "flashvm/path.h"

#define DVM_EDITOR_FRAME_MAX_PATHS 8192U   /* Bound independent painted paths per frame. */
#define DVM_EDITOR_FRAME_MAX_VERBS 262144U /* Bound copied contour commands per frame. */
#define DVM_EDITOR_FRAME_EXTENT 2048U      /* Match the admitted editor canvas limit. */


typedef struct DVM_EditorFrame DVM_EditorFrame; /* Owned native contours and private RGBA buffer. */


/* Allocate a white-background frame with bounded nonzero dimensions; null on failure.
 * No VM/font/window handles are retained. Owner fills paths before transferring it. */
DVM_EditorFrame *dvm_editor_frame_create(unsigned int width, unsigned int height);


/* Transfer path on success only, preserving paint order and copied color.
 * Frame/path must be exclusively owner-held. Zero on allocation/command limit failure. */
int dvm_editor_frame_path(DVM_EditorFrame *frame, FVM_Path *path, FVM_RasterColor color);


/* Borrow the frame's independent surface for owner presentation after queue take.
 * The returned pointer remains valid until destroy; do not read while rendering. */
const FVM_RasterSurface *dvm_editor_frame_surface(const DVM_EditorFrame *frame);


/* Copy an owner-rendered MovieClip base into an exclusively owned frame before
 * submission. Dimensions must match; later caller mutation cannot affect it.
 * Return zero on invalid input. No VM reference or caller pixels are retained. */
int dvm_editor_frame_background(DVM_EditorFrame *frame, const FVM_RasterSurface *source);


/* Queue execute callback: rasterize only this frame's contours into its private buffer.
 * Return first native draw failure. No VM, font, SDL, global view state or file I/O. */
FVM_Result dvm_editor_frame_render(void *frame);


/* Queue release callback: destroy native contours and pixel storage, accepting null.
 * Safe on an owner or worker with exclusive frame ownership; no VM/GC dependencies. */
void dvm_editor_frame_destroy(void *frame);
#endif
