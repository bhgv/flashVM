/* Single-owner FlashVM renderer and copied event source for the Dis editor. */
#ifndef DVM_EDITOR_VIEW_H
#define DVM_EDITOR_VIEW_H
#include "disvm/ui_bridge.h"


/* Configure a borrowed CLI value before initialization; returns one for a known option. */
int dvm_editor_option(const char *name, const char *value);


/* Select the packaged default font beside the entry module; explicit --font wins.
 * directory is borrowed; returns one after copying a bounded native resource path. */
int dvm_editor_default_font(const char *directory);


/* Return actual visible logical-line capacity for the current font size, bounded 1..128. */
int dvm_editor_rows(void);


/* Create the owner-local FlashVM field/window. Returns one or zero with a diagnostic. */
int dvm_editor_create(void);


/* Read the configured immutable document path; borrowed until process exit. */
const char *dvm_editor_path(void);


/* Return the explicit new-document flag; no file is created until Dis saves it. */
int dvm_editor_fresh(void);


/* Accept one copied Dis presentation frame. Returns zero on protocol/render errors. */
int dvm_editor_present(const DVM_UIMessage *message);


/* Poll one native/scripted event without waiting; 1=value, 0=idle, -1=error.
 * The caller retains rejected/backpressured records; no document is mutated here. */
int dvm_editor_event(DVM_UIMessage *message);


/* Release native/FlashVM resources after all Dis channel callbacks stop. */
void dvm_editor_destroy(void);
#endif
