/* SDL text bursts stay value-only and preserve command/lifecycle boundaries. */
#ifndef DVM_EDITOR_TEXT_EVENTS_H
#define DVM_EDITOR_TEXT_EVENTS_H
#include <SDL.h>
#include "disvm/ui_bridge.h"


/* Return one for a plain letter key with no Ctrl/Alt/GUI/IME mode shortcut.
 * Shift is handled by SDL TextInput and remains a separately ordered event. */
int dvm_editor_plain_letter(const SDL_KeyboardEvent *event);


/* Copy a first TextInput and bounded FIFO continuations into one UTF-8 message.
 * context owns keyboard snapshots; no Dis document or raster is touched.
 * Consume only same-window text/plain-letter transitions, preserve all command,
 * focus, pointer, IME and capacity boundaries. Return1 or0 on invalid/native error.
 * Owner-thread only, nonblocking; no codepoint or SDL text record is split. */
int dvm_editor_text_batch(FVM_Context *context, const SDL_TextInputEvent *first, DVM_UIMessage *message);
#endif
