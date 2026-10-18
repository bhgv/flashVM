/* Test SDL event translation and focus reset without opening a window or audio endpoint. */
#include "../common/sdl_window.c"
#include <assert.h>

static void key(FVM_DesktopWindow *view, Uint32 type, SDL_Keycode symbol, Uint16 mods)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.keysym.sym = symbol;
    event.key.keysym.mod = mods;
    assert(handle_event(view, &event));
}

int main(void)
{
    FVM_DesktopWindow view;
    SDL_Event event;
    memset(&view, 0, sizeof(view));
    key(&view, SDL_KEYDOWN, SDLK_LEFT, 0);
    key(&view, SDL_KEYDOWN, SDLK_RIGHT, 0);
    assert(view.input.keyboard.down[FVM_KEY_LEFT] && view.input.keyboard.down[FVM_KEY_RIGHT]);
    key(&view, SDL_KEYUP, SDLK_LEFT, 0);
    assert(!view.input.keyboard.down[FVM_KEY_LEFT] && view.input.keyboard.down[FVM_KEY_RIGHT]);
    assert(view.input.keyboard.last_code == FVM_KEY_RIGHT);
    key(&view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT);
    assert(view.input.keyboard.last_code == 'A' && view.input.keyboard.last_ascii == 'A');
    key(&view, SDL_KEYDOWN, SDLK_a, KMOD_CAPS | KMOD_SHIFT);
    assert(view.input.keyboard.last_ascii == 'a' && view.input.keyboard.caps_lock);
    key(&view, SDL_KEYDOWN, SDLK_2, KMOD_SHIFT);
    assert(view.input.keyboard.last_ascii == '@');
    key(&view, SDL_KEYDOWN, SDLK_BACKSLASH, KMOD_SHIFT);
    assert(view.input.keyboard.last_ascii == '|' && view.input.keyboard.down[FVM_KEY_BACKSLASH]);
    key(&view, SDL_KEYUP, SDLK_LSHIFT, KMOD_RSHIFT);
    assert(view.input.keyboard.down[FVM_KEY_SHIFT]);
    key(&view, SDL_KEYDOWN, SDLK_F12, KMOD_NUM);
    assert(view.input.keyboard.last_code == FVM_KEY_F1 + 11 && view.input.keyboard.num_lock);
    memset(&event, 0, sizeof(event));
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    assert(handle_event(&view, &event));
    assert(!view.input.keyboard.down[FVM_KEY_RIGHT] && !view.input.keyboard.last_code);
    assert(view.input.keyboard.struct_size == sizeof(view.input.keyboard));
    event.type = SDL_APP_WILLENTERBACKGROUND;
    assert(handle_event(&view, &event) && view.background);
    return 0;
}
