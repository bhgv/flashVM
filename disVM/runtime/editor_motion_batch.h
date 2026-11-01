/* Bounded copied SDL trajectories; no VM/window/document ownership. */
#ifndef DVM_EDITOR_MOTION_BATCH_H
#define DVM_EDITOR_MOTION_BATCH_H
#include <SDL.h>
#define DVM_EDITOR_MOTION_POINTS 64U /* Fits one existing bounded UI envelope. */


/* Copy the removed head and consume only contiguous same-device motion. Keep
 * turns and final position, compact <=1px/2ms redundant jitter. Never consume a
 * release/focus/other device. Owner thread; caller supplies64 event slots. */
unsigned int dvm_editor_motion_batch(const SDL_Event *head, SDL_Event *points);


/* Return whether an event can change document input/lifecycle. Pure native
 * classification; ignored OS/expose notifications must not end a live stroke. */
int dvm_editor_motion_barrier(const SDL_Event *event);
#endif
