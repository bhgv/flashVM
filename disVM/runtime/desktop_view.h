/* MovieClip scene adapter; all objects stay on the FlashVM/context owner. */
#ifndef DVM_DESKTOP_VIEW_H
#define DVM_DESKTOP_VIEW_H
#include "disvm/ui_bridge.h"
#include "flashvm/flashvm.h"
#include "flashvm/path.h"
#define DVM_DESK_BEGIN 65569UL   /* width/height plus ticket/count/scroll words. */
#define DVM_DESK_ITEM 65570UL    /* One copied bounded MovieClip descriptor. */
#define DVM_DESK_END 65571UL     /* Commit the declared complete scene. */
#define DVM_DESK_POINTER 65572UL /* Raw checked pixel proposal with phase/code words. */
#define DVM_DESK_RESIZE 65573UL  /* Native pixel resize proposal. */


/* Bind borrowed initialized context/face bytes; no cross-heap pointer is published.
 * Return zero on invalid input. destroy must run before context destruction. */
int dvm_desktop_bind(FVM_Context *context, const unsigned char *font, size_t size, int trace);
/* Stage a bounded copied frame; return 1 valid, 0 malformed. End returns a complete
 * scene ready for draw. No filesystem, document or Limbo process is accessed. */
int dvm_desktop_message(const DVM_UIMessage *message);
/* Paint actual MovieClip drawing state and owned TextFields into owner-held RGBA.
 * VM access stays here; the caller copies the result into a detached raster job.
 * Return zero on layout, VM, allocation or protocol failure. */
int dvm_desktop_draw(FVM_RasterSurface *surface);
/* Return committed input/view state, or zero before any desktop scene. */
int dvm_desktop_active(void);
unsigned long dvm_desktop_ticket(void); /* Equality token, never a heap address. */
/* Release pinned owner objects/font before the borrowed context is destroyed. */
void dvm_desktop_destroy(void);
#endif
