/* Native, VM-independent editor frame ownership; prepared by the UI owner. */
#ifndef DVM_EDITOR_RASTER_H
#define DVM_EDITOR_RASTER_H
#include "flashvm/path.h"

#define DVM_EDITOR_SCENE_NODES 64U                   /* Stable retained layer IDs; zero is the composition root. */
#define DVM_EDITOR_NODE_OPACITY 1000U                /* Fixed-point opacity:1000 opaque,900 exactly90 percent. */
#define DVM_EDITOR_NODE_CONTENT 1U                   /* Native local commands changed: repaint associated bitmap. */
#define DVM_EDITOR_NODE_TRANSFORM 2U                 /* Placement/opacity/order changed: compose old/new bounds. */
#define DVM_EDITOR_NODE_COMPOSITE 4U                 /* A descendant changed: propagate composition damage upward. */
#define DVM_EDITOR_SCENE_BYTES (64U * 1024U * 1024U) /* Maximum associated bitmap bytes per frame. */

#define DVM_EDITOR_FRAME_MAX_PATHS 8192U             /* Bound independent painted paths per frame. */
#define DVM_EDITOR_FRAME_MAX_VERBS 262144U           /* Bound copied contour commands per frame. */
#define DVM_EDITOR_FRAME_MAX_TILES 1024U             /* Maximum 32 by 32 canvas tile grid. */
#define DVM_EDITOR_TILE_SIZE 64U                     /* Independent dirty raster tile edge in pixels. */
#define DVM_EDITOR_RASTER_WORKERS 4U                 /* Parallel native tile workers per detached desktop job. */
#define DVM_EDITOR_FRAME_EXTENT 2048U                /* Match the admitted editor canvas limit. */


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


/* Copy a borrowed contour and its explicit canvas clip into frame. Parameters
 * are native values; no VM handle is retained. Return zero without taking path
 * ownership on failure. Empty clips are successful no-ops. Owner preparation only. */
int dvm_editor_frame_command(DVM_EditorFrame *frame,
                             const FVM_Path *path,
                             FVM_RasterColor color,
                             double width,
                             int even_odd,
                             int left,
                             int top,
                             int right,
                             int bottom);


/* Append a bounded opacity-group boundary. begin=1 saves the underlying tile;
 * begin=0 composites this group's painted pixels at 90 percent. Groups cannot
 * nest; caller supplies matching canvas bounds. Returns zero on allocation error. */
int dvm_editor_frame_group(DVM_EditorFrame *frame, int begin, int left, int top, int right, int bottom);


/* Compare exact native paint commands with the last displayed frame, copy its
 * pixels and mark changed old/new bounds. prior is borrowed only during this call;
 * null or different dimensions require all tiles. Return zero on invalid input.
 * Must run before submission, outside any worker using frame. */
int dvm_editor_frame_incremental(DVM_EditorFrame *frame, const DVM_EditorFrame *prior);


/* Return one dirty tile rectangle by row-major index, zero for clean/out of range.
 * Output values are canvas pixels. frame is immutable after preparation; safe on
 * the owner after queue take. Nonincremental frames report one full-canvas tile. */
int dvm_editor_frame_damage(const DVM_EditorFrame *frame,
                            unsigned int index,
                            unsigned int *x,
                            unsigned int *y,
                            unsigned int *width,
                            unsigned int *height);


/* Queue execute callback: rasterize only this frame's contours into its private buffer.
 * Return first native draw failure. No VM, font, SDL, global view state or file I/O. */
FVM_Result dvm_editor_frame_render(void *frame);


/* Queue release callback: destroy native contours and pixel storage, accepting null.
 * Safe on an owner or worker with exclusive frame ownership; no VM/GC dependencies. */
void dvm_editor_frame_destroy(void *frame);

/* Begin one retained object in painter order. id is1..63 and parent is0..63.
 * Geometry is initially captured in world coordinates. end supplies its actual
 * world matrix and logical extent; nesting is represented by parent IDs, not
 * nested command recording. Zero rejects malformed state; discard that frame. */
int dvm_editor_frame_node_begin(DVM_EditorFrame *frame, unsigned int id, unsigned int parent);


/* Finish the active object. Matrix/extent/opacity are copied native values.
 * Uniform scale/rotation and translation are composed from the associated bitmap.
 * opacity is0..DVM_EDITOR_NODE_OPACITY; modifiers are already composed native values.
 * Commands become node-local at incremental preparation; no VM handle is kept. */
int dvm_editor_frame_node_end(
    DVM_EditorFrame *frame, const FVM_Affine *world, unsigned int width, unsigned int height, unsigned int opacity);


/* Copy diagnostic counters after rendering only: bitmap builds/reuses and dirty
 * nodes. Never read a frame while its worker owns it. Null outputs are allowed. */
void dvm_editor_frame_scene_stats(const DVM_EditorFrame *frame,
                                  unsigned int *built,
                                  unsigned int *reused,
                                  unsigned int *dirty);


/* Return1 only for associated-image budget exhaustion; malformed forests have
 * another failure reason. Caller can discard and retry ordinary vector capture. */
int dvm_editor_frame_scene_budget(const DVM_EditorFrame *frame);


/* Return actual local bitmap pixels repainted by this completed job; unchanged
 * cached images report zero. Root composition and buffer copies are separate. */
size_t dvm_editor_frame_scene_pixels(const DVM_EditorFrame *frame);


/* Return pending node-local/descendant bits on an owner-held snapshot; never read
 * during worker ownership. Successful composition clears them, failure does not. */
unsigned int dvm_editor_frame_node_flags(const DVM_EditorFrame *frame, unsigned int id);


/* Return the protected physical image count across all live native frames. */
unsigned int dvm_editor_scene_live_images(void);


/* Copy an initialized owner content key into the active node; size1..4096.
 * Failure preserves its previous key; no caller byte storage is retained. */
int dvm_editor_frame_node_key(DVM_EditorFrame *frame, const void *key, size_t size);


/* Borrow a completed displayed frame and exact key; return1 only for an admitted
 * cached identity. Never inspect a frame while a rendering worker owns it. */
int dvm_editor_frame_node_matches(const DVM_EditorFrame *prior, unsigned int id, const void *key, size_t size);


/* Share immutable local commands and pixels from completed prior into an empty active
 * node. Caller supplies new placement with node_end; failure discards candidate. */
int dvm_editor_frame_node_replay(DVM_EditorFrame *frame, const DVM_EditorFrame *prior, unsigned int id);


/* Owner-only completed-frame transfer. complete=1 initializes a fresh buffer;
 * otherwise destination must contain this frame's exact preparation baseline.
 * Surfaces must be disjoint or exactly identical. Padding is untouched. Failure
 * validates before writing; copied is optional and reports actual pixel bytes. */
int dvm_editor_frame_present(const DVM_EditorFrame *frame,
                             FVM_RasterSurface *destination,
                             int complete,
                             size_t *copied);


/* Return clean baseline pixel bytes copied while preparing this native frame.
 * Read only on its owner, outside worker execution; null returns zero. */
size_t dvm_editor_frame_baseline_bytes(const DVM_EditorFrame *frame);


/* Check whether prior is this candidate's exact preparation baseline. Both
 * frames are borrowed live immutable snapshots; no pointer is retained. A
 * mismatched displayed baseline requires complete=1 on presentation. */
int dvm_editor_frame_has_baseline(const DVM_EditorFrame *frame, const DVM_EditorFrame *prior);

#endif
