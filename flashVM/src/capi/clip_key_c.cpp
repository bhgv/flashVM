/* FlashVM: pure copied-keyboard transition mapping to the SWF KeyPress selector space. */
#include "flashvm/clip_actions.h"
#include "../core/keyboard_state.hpp"

/* Prefer special-key identities to ASCII; never mutate or publish the supplied snapshot. */
extern "C" FVM_Result fvm_input_clip_key_code(const FVM_KeyboardState *state, int pressed,
                                              unsigned int *out)
{
    if (!out || (pressed != 0 && pressed != 1) || !fvm_keyboard_state_valid(state))
        return FVM_INVALID_ARGUMENT;
    unsigned int code = FVM_CLIP_KEY_NONE;
    if (pressed && state->last_code && state->down[state->last_code])
    {
        switch (state->last_code)
        {
        case FVM_KEY_LEFT: code = FVM_CLIP_KEY_LEFT; break;
        case FVM_KEY_RIGHT: code = FVM_CLIP_KEY_RIGHT; break;
        case FVM_KEY_HOME: code = FVM_CLIP_KEY_HOME; break;
        case FVM_KEY_END: code = FVM_CLIP_KEY_END; break;
        case FVM_KEY_INSERT: code = FVM_CLIP_KEY_INSERT; break;
        case FVM_KEY_DELETE: code = FVM_CLIP_KEY_DELETE; break;
        case FVM_KEY_BACKSPACE: code = FVM_CLIP_KEY_BACKSPACE; break;
        case FVM_KEY_ENTER: code = FVM_CLIP_KEY_ENTER; break;
        case FVM_KEY_UP: code = FVM_CLIP_KEY_UP; break;
        case FVM_KEY_DOWN: code = FVM_CLIP_KEY_DOWN; break;
        case FVM_KEY_PGUP: code = FVM_CLIP_KEY_PGUP; break;
        case FVM_KEY_PGDN: code = FVM_CLIP_KEY_PGDN; break;
        case FVM_KEY_TAB: code = FVM_CLIP_KEY_TAB; break;
        case FVM_KEY_ESCAPE: code = FVM_CLIP_KEY_ESCAPE; break;
        default:
            if (state->last_ascii >= FVM_CLIP_KEY_ASCII_FIRST &&
                state->last_ascii <= FVM_CLIP_KEY_ASCII_LAST)
                code = state->last_ascii;
            break;
        }
    }
    *out = code;
    return FVM_OK;
}
