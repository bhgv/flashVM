/* Test SDL event translation and focus reset without opening a window or audio endpoint. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"
#include "flashvm/standard.h"


static void key(FVM_DesktopWindow *view, Uint32 type, SDL_Keycode symbol, Uint16 mods)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.keysym.sym = symbol;
    event.key.keysym.mod = mods;
    if (!(handle_event(view, &event)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* A real native listener observes both halves of a tap queued before a movie tick. */
static FVM_Result receive_key(
    FVM_Context *context, FVM_Object *self, unsigned int count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    unsigned int *calls = (unsigned int *)raw;
    FVM_KeyboardState state;
    (void)self;
    (void)count;
    (void)args;
    state.struct_size = sizeof(state);
    if (!(fvm_input_get_keyboard(context, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(state.down[FVM_KEY_LEFT] == (*calls == 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++*calls;
    fvm_value_undefined(out);
    return FVM_OK;
}


int main(void)
{
    FVM_DesktopWindow view;
    SDL_Event event;
    FVM_Config config;
    FVM_Runtime *runtime;
    FVM_Context *context;
    FVM_Object *listener;
    unsigned int calls = 0, i;
    memset(&view, 0, sizeof(view));
    key(&view, SDL_KEYDOWN, SDLK_LEFT, 0);
    key(&view, SDL_KEYDOWN, SDLK_RIGHT, 0);
    if (!(view.input.keyboard.down[FVM_KEY_LEFT] && view.input.keyboard.down[FVM_KEY_RIGHT]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYUP, SDLK_LEFT, 0);
    if (!(!view.input.keyboard.down[FVM_KEY_LEFT] && view.input.keyboard.down[FVM_KEY_RIGHT]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(view.input.keyboard.last_code == FVM_KEY_RIGHT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT);
    if (!(view.input.keyboard.last_code == 'A' && view.input.keyboard.last_ascii == 'A'))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_a, KMOD_CAPS | KMOD_SHIFT);
    if (!(view.input.keyboard.last_ascii == 'a' && view.input.keyboard.caps_lock))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_2, KMOD_SHIFT);
    if (!(view.input.keyboard.last_ascii == '@'))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_BACKSLASH, KMOD_SHIFT);
    if (!(view.input.keyboard.last_ascii == '|' && view.input.keyboard.down[FVM_KEY_BACKSLASH]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYUP, SDLK_LSHIFT, KMOD_RSHIFT);
    if (!(view.input.keyboard.down[FVM_KEY_SHIFT]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_F12, KMOD_NUM);
    if (!(view.input.keyboard.last_code == FVM_KEY_F1 + 11 && view.input.keyboard.num_lock))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&event, 0, sizeof(event));
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    if (!(handle_event(&view, &event)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!view.input.keyboard.down[FVM_KEY_RIGHT] && !view.input.keyboard.last_code))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(view.input.keyboard.struct_size == sizeof(view.input.keyboard)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.type = SDL_APP_WILLENTERBACKGROUND;
    if (!(handle_event(&view, &event) && view.background))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_LEFT, 0);
    if (!(!view.event_count && !view.input.keyboard.down[FVM_KEY_LEFT]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event.type = SDL_APP_DIDENTERFOREGROUND;
    if (!(handle_event(&view, &event) && !view.background))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(context, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(context, listener, "onKeyDown", receive_key, &calls, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(context, listener, "onKeyUp", receive_key, &calls, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_key_listener(context, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&view, SDL_KEYDOWN, SDLK_LEFT, 0);
    key(&view, SDL_KEYUP, SDLK_LEFT, 0);
    if (!(view.event_count == 2 && !view.input.keyboard.down[FVM_KEY_LEFT] && calls == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_keyboard(&view, context) == FVM_OK && calls == 2 && !view.event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_keyboard(&view, context) == FVM_OK && calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i <= FVM_DESKTOP_PENDING_EVENTS; ++i)
    {
        key(&view, SDL_KEYDOWN, SDLK_LEFT, 0);
    }
    if (!(view.event_count == FVM_DESKTOP_PENDING_EVENTS && view.event_error == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_keyboard(&view, context) == FVM_ERROR && calls == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_keyboard(0, context) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    return 0;
}
