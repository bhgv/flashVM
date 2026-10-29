/* Merge already queued native typing without waiting for a render or frame. */
#include "editor_text_events.h"
#include "flashvm/keyboard.h"
#include <string.h>

#define DVM_TEXT_EVENT_BUDGET 256U /* Bound work even during sustained event arrival. */


/* Classify ignored document-level letter transitions, retaining native key state.
 * event is borrowed SDL state; return0 for modifiers, commands and IME modes. */
int dvm_editor_plain_letter(const SDL_KeyboardEvent *event)
{
    return event->keysym.sym >= SDLK_a && event->keysym.sym <= SDLK_z &&
           !(event->keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI | KMOD_MODE));
}


/* Measure only the bounded SDL record; missing NUL is invalid, never strlen it. */
static size_t text_size(const SDL_TextInputEvent *event)
{
    const char *end = memchr(event->text, 0, sizeof(event->text));
    return end ? (size_t)(end - event->text) : sizeof(event->text);
}


/* Fold current queued text/ordinary-letter transitions into a single edit.
 * The first record is already removed. Subsequent records stay queued unless
 * they fit entirely and share its window. Native key snapshots remain accurate;
 * Dis receives text, not no-op alphabetic key round trips. No waiting/painting. */
int dvm_editor_text_batch(FVM_Context *context, const SDL_TextInputEvent *first, DVM_UIMessage *message)
{
    SDL_Event next;
    unsigned count;
    size_t bytes = text_size(first);
    if (bytes >= sizeof(first->text))
    {
        return 0;
    }
    message->kind = DVM_UI_TEXT;
    message->size = bytes;
    memcpy(message->text, first->text, bytes);
    for (count = 0; count < DVM_TEXT_EVENT_BUDGET; count++)
    {
        if (SDL_PeepEvents(&next, 1, SDL_PEEKEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
        {
            break;
        }
        if (next.type == SDL_TEXTINPUT && next.text.windowID == first->windowID)
        {
            bytes = text_size(&next.text);
            if (bytes >= sizeof(next.text.text) || bytes > DVM_UI_TEXT_MAX - message->size)
            {
                break;
            }
            if (SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
            {
                break;
            }
            memcpy(message->text + message->size, next.text.text, bytes);
            message->size += bytes;
        }
        else if ((next.type == SDL_KEYDOWN || next.type == SDL_KEYUP) &&
                 next.key.windowID == first->windowID && dvm_editor_plain_letter(&next.key))
        {
            FVM_KeyboardState state;
            unsigned code = (unsigned)(next.key.keysym.sym - SDLK_a + 'A');
            memset(&state, 0, sizeof(state));
            state.struct_size = sizeof(state);
            if (fvm_input_get_keyboard(context, &state) != FVM_OK)
            {
                return 0;
            }
            state.down[code] = next.type == SDL_KEYDOWN;
            state.last_code = code;
            state.last_ascii = 0;
            if (fvm_input_set_keyboard(context, &state) != FVM_OK ||
                SDL_PeepEvents(&next, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) != 1)
            {
                return 0;
            }
        }
        else
        {
            break;
        }
    }
    return 1;
}
