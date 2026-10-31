/* Owner-local SDL queue policy; never a document or cross-heap interface. */
#ifndef DVM_EDITOR_DESKTOP_INPUT_H
#define DVM_EDITOR_DESKTOP_INPUT_H
#include <SDL.h>


/* event is the removed FIFO head, captured marks an active paint/pen gesture.
 * Preserve every captured sample. Otherwise combine only contiguous matching
 * device/button motion. Owner-only; never consume a button/focus/lifecycle event. */
void dvm_editor_coalesce_motion(SDL_Event *event, int captured);


/* Classify an initial button/modifier chord. A gesture latches this result until
 * release, so releasing a modifier cannot turn table motion into source edits. */
int dvm_editor_pan_button(unsigned int button, unsigned int modifiers);
#endif
