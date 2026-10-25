/* SDL2-to-Flash keyboard translation, independent of the VM and operating-system hooks. */
#ifndef FVM_DESKTOP_KEYBOARD_SDL_H
#define FVM_DESKTOP_KEYBOARD_SDL_H /* Private main-thread keyboard translation guard. */
#include "flashvm/keyboard.h"
#include <SDL.h>
#include <string.h>


/* Translate supported SDL key symbols to Flash/Windows-style virtual-key slots. */
static unsigned int fvm_desktop_key_code(SDL_Keycode key)
{
    if (key >= SDLK_a && key <= SDLK_z)
    {
        return FVM_KEY_LETTER_A + (unsigned int)(key - SDLK_a);
    }
    if (key >= SDLK_0 && key <= SDLK_9)
    {
        return FVM_KEY_DIGIT0 + (unsigned int)(key - SDLK_0);
    }
    if (key >= SDLK_F1 && key <= SDLK_F12)
    {
        return FVM_KEY_F1 + (unsigned int)(key - SDLK_F1);
    }
    if (key >= SDLK_KP_1 && key <= SDLK_KP_9)
    {
        return FVM_KEY_NUMPAD0 + (unsigned int)(key - SDLK_KP_1) + 1;
    }
    switch (key)
    {
    case SDLK_KP_0:
        return FVM_KEY_NUMPAD0;
    case SDLK_BACKSPACE:
        return FVM_KEY_BACKSPACE;
    case SDLK_TAB:
        return FVM_KEY_TAB;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        return FVM_KEY_ENTER;
    case SDLK_LSHIFT:
    case SDLK_RSHIFT:
        return FVM_KEY_SHIFT;
    case SDLK_LCTRL:
    case SDLK_RCTRL:
        return FVM_KEY_CONTROL;
    case SDLK_LALT:
    case SDLK_RALT:
        return FVM_KEY_ALT;
    case SDLK_CAPSLOCK:
        return FVM_KEY_CAPSLOCK;
    case SDLK_NUMLOCKCLEAR:
        return FVM_KEY_NUMLOCK;
    case SDLK_ESCAPE:
        return FVM_KEY_ESCAPE;
    case SDLK_SPACE:
        return FVM_KEY_SPACE;
    case SDLK_PAGEUP:
        return FVM_KEY_PGUP;
    case SDLK_PAGEDOWN:
        return FVM_KEY_PGDN;
    case SDLK_END:
        return FVM_KEY_END;
    case SDLK_HOME:
        return FVM_KEY_HOME;
    case SDLK_LEFT:
        return FVM_KEY_LEFT;
    case SDLK_UP:
        return FVM_KEY_UP;
    case SDLK_RIGHT:
        return FVM_KEY_RIGHT;
    case SDLK_DOWN:
        return FVM_KEY_DOWN;
    case SDLK_INSERT:
        return FVM_KEY_INSERT;
    case SDLK_DELETE:
        return FVM_KEY_DELETE;
    case SDLK_SEMICOLON:
        return FVM_KEY_SEMICOLON;
    case SDLK_EQUALS:
        return FVM_KEY_EQUALS;
    case SDLK_COMMA:
        return FVM_KEY_COMMA;
    case SDLK_MINUS:
        return FVM_KEY_MINUS;
    case SDLK_PERIOD:
        return FVM_KEY_PERIOD;
    case SDLK_SLASH:
        return FVM_KEY_SLASH;
    case SDLK_BACKQUOTE:
        return FVM_KEY_BACKQUOTE;
    case SDLK_LEFTBRACKET:
        return FVM_KEY_LEFTBRACKET;
    case SDLK_BACKSLASH:
        return FVM_KEY_BACKSLASH;
    case SDLK_RIGHTBRACKET:
        return FVM_KEY_RIGHTBRACKET;
    case SDLK_QUOTE:
        return FVM_KEY_QUOTE;
    default:
        return 0;
    }
}


/* Derive English ASCII for polling; SDL text input/IME composition is deliberately separate. */
static unsigned int fvm_desktop_key_ascii(SDL_Keycode key, SDL_Keymod mods)
{
    int shift = (mods & KMOD_SHIFT) != 0;
    if (key >= SDLK_a && key <= SDLK_z)
    {
        return (unsigned int)((shift != ((mods & KMOD_CAPS) != 0)) ? key - SDLK_a + 'A' : key);
    }
    if (key >= SDLK_0 && key <= SDLK_9 && shift)
    {
        return (unsigned int)")!@#$%^&*("[key - SDLK_0];
    }
    if (shift)
    {
        const char *plain = "`;=,-./[\\]'";
        const char *shifted = "~:+<_>?{|}\"";
        const char *found = key > 0 && (unsigned int)key <= FVM_KEYBOARD_ASCII_MAX ? strchr(plain, (int)key) : 0;
        if (found)
        {
            return (unsigned int)shifted[found - plain];
        }
    }
    return key > 0 && (unsigned int)key <= FVM_KEYBOARD_ASCII_MAX ? (unsigned int)key : 0;
}


/* Apply one main-thread transition; repeats update codes but do not create extra held keys. */
static void fvm_desktop_keyboard_event(FVM_KeyboardState *state, const SDL_KeyboardEvent *event)
{
    unsigned int code = fvm_desktop_key_code(event->keysym.sym);
    int down = event->type == SDL_KEYDOWN;
    SDL_Keymod mods = (SDL_Keymod)event->keysym.mod;
    state->struct_size = sizeof(*state);
    if (code)
    {
        state->down[code] = (unsigned char)down;
        if (down)
        {
            state->last_code = code;
        }
    }
    state->last_ascii = fvm_desktop_key_ascii(event->keysym.sym, mods);
    state->caps_lock = (mods & KMOD_CAPS) != 0;
    state->num_lock = (mods & KMOD_NUM) != 0;
    /* The modifier mask preserves the other physical modifier when its twin is released. */
    state->down[FVM_KEY_SHIFT] = (unsigned char)((mods & KMOD_SHIFT) != 0);
    state->down[FVM_KEY_CONTROL] = (unsigned char)((mods & KMOD_CTRL) != 0);
    state->down[FVM_KEY_ALT] = (unsigned char)((mods & KMOD_ALT) != 0);
}
#endif
