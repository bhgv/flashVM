/* Native live-stroke ownership. Tokens are bounded values, never heap addresses. */
#ifndef DVM_EDITOR_INK_H
#define DVM_EDITOR_INK_H
#include "flashvm/path.h"
#include <stddef.h>
#define DVM_INK_POINTS 4096U /* Matches the portable vector path admission bound. */
#define DVM_INK_SLOTS 32U    /* Ordered unvectorized gestures; never latest-wins data loss. */


/* Begin a captured Brush/Pen with copied style/packed1001*y+x. Return a unique
 * positive token, or zero on invalid/capacity refusal. Sole native owner only. */
unsigned long dvm_ink_begin(unsigned long widget, unsigned long color, unsigned long width, long point);


/* Retain bounded samples without any Dis/channel/managed allocation. Suppress
 * duplicate points. Return one on success, zero on stale token/full storage. */
/* Copy checked tool identity on the native owner; no VM references are retained. */
void dvm_ink_tool(unsigned long token, unsigned long tool);
/* Return whether a live geometric preview needs destination-dependent XOR. */
int dvm_ink_has_xor(void);
int dvm_ink_point(unsigned long token, long point);


/* Freeze one native trajectory for the Limbo geometry worker. Native preview
 * remains visible until an explicitly acknowledged vector stream is committed. */
int dvm_ink_end(unsigned long token, long point);


/* Worker bridge: borrow immutable counted-hex points for a frozen token. Return
 * an empty string for stale/live tokens. No native address is sent to Limbo. */
const char *dvm_ink_read(unsigned long token);


/* Mark this and earlier same-document strokes for atomic visual replacement by
 * exactly this complete vector stream. Drawing bytes are borrowed only here. */
void dvm_ink_ack(unsigned long token, const char *drawing, size_t size);


/* Retire acknowledged previews only when the matching complete wire is accepted.
 * Called on the native UI owner; outstanding copied raster jobs are unaffected. */
void dvm_ink_commit(unsigned long widget, const char *drawing, size_t size);


/* Emit one immutable physical path per retained stroke in this pane's paint
 * order. Callback borrows path until return; caller copies it into a raster job.
 * Negative callback width requests XOR for a live primitive; frozen paths use color.
 * Return zero on geometry/callback failure. No VM or document access occurs. */
typedef int (*DVM_InkEmit)(void *user, const FVM_Path *path, FVM_RasterColor color, double width);
int dvm_ink_preview(unsigned long widget, const FVM_Affine *world, double scale, DVM_InkEmit emit, void *user);


/* Drop a cancelled capture; reset drops all native state after callback teardown. */
int dvm_ink_last(unsigned long token, unsigned long *widget, long *point);
void dvm_ink_cancel(unsigned long token);
/* Retire previews for a document removed from the complete scene. */
void dvm_ink_drop(unsigned long widget);
void dvm_ink_reset(void);


/* Return whether the native owner has any live/frozen preview for this widget.
 * Scalar query only; no point arrays, allocation or VM access occurs. */
int dvm_ink_has_preview(unsigned long widget);
#endif
