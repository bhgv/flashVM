/* SDL window lifecycle translation into copied IDE requests; no document access. */
#ifndef DVM_EDITOR_WINDOW_EVENTS_H
#define DVM_EDITOR_WINDOW_EVENTS_H
#include <SDL.h>
#include "disvm/ui_bridge.h"


/* Translate close/resize/maximize/restore for window into message without waiting.
 * event/window are borrowed SDL values. Returns 1 only after writing a complete request;
 * unrelated/foreign/invalid events return 0 without touching message. Dis decides dirty-close policy. */
int dvm_editor_window_event(const SDL_Event *event, SDL_Window *window, DVM_UIMessage *message);
#endif
