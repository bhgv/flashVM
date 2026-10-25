/* Actual SDL adapter: event-copy selector mapping, key-stage order, callback errors and FIFO reset. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/stage.h"
#include "clip_keypress_movie.h"


/* Native observation state lives until the window and managed context have been released. */
struct Trace
{
    FVM_DesktopWindow *view; /* Native test window owned by main. */
    unsigned namespaces;     /* Explicit Key listener entries. */
    unsigned downs;          /* Retained global KeyDown entries. */
    unsigned methods;        /* Optional onKeyDown method entries. */
    unsigned presses;        /* Matching retained KeyPress entries. */
    unsigned keys[32];       /* Exact raw selectors independently sent by the fixture. */
    unsigned fail;           /* KeyPress ordinal that returns ERROR, or zero. */
    int append;              /* Append an A press from a KeyPress callback. */
    int reset;               /* Reset native input from the next method, canceling KeyPress and old FIFO. */
};


/* Deliver synthetic native key messages to the actual SDL decoder. */
static void key(FVM_DesktopWindow *view, Uint32 type, SDL_Keycode symbol, Uint16 mods, int repeat)
{
    SDL_Event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.key.keysym.sym = symbol;
    event.key.keysym.mod = mods;
    event.key.repeat = (Uint8)repeat;
    if (!(handle_event(view, &event)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Explicit namespace observers precede the retained scene KeyDown. */
static FVM_Result
observe(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    (void)c;
    (void)self;
    (void)args;
    if (!(!count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++t->namespaces;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* A retained KeyDown has already received the current namespace notification. */
static FVM_Result
down(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    (void)c;
    (void)self;
    (void)args;
    if (!(!count && t->namespaces == t->downs + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++t->downs;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Mutate VM state deliberately: KeyPress must still translate the original copied SDL transition. */
static FVM_Result
method(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    FVM_KeyboardState state;
    (void)self;
    (void)args;
    if (!(!count && t->downs == t->methods + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++t->methods;
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    state.last_code = 'Z';
    state.last_ascii = 'Z';
    state.down['Z'] = 1;
    if (!(fvm_input_set_keyboard(c, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->reset)
    {
        SDL_Event event;
        t->reset = 0;
        memset(&event, 0, sizeof(event));
        event.type = SDL_WINDOWEVENT;
        event.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
        if (!(handle_event(t->view, &event)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Record exact selector and reject recursive pumping while preserving pending redraw on errors. */
static FVM_Result
press(FVM_Context *c, FVM_Object *self, unsigned count, const FVM_Value *args, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    (void)self;
    if (!(count == 1 && args[0].type == FVM_VALUE_NUMBER))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t->methods == t->downs && t->presses < 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->keys[t->presses++] = (unsigned)args[0].number;
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_clip_input(t->view, c, 0, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->append)
    {
        t->append = 0;
        key(t->view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT, 0);
    }
    if (!(fvm_call(c, "updateAfterEvent", 0, 0, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return t->fail == t->presses ? FVM_ERROR : FVM_OK;
}


/* Build one real dummy-video host and exercise the production FIFO pipeline. */
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0, *listener = 0;
    FVM_Value actor;
    FVM_DesktopWindow *view;
    struct Trace t;
    unsigned before;
    int pending;
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view = fvm_desktop_window_open("KeyPress", 32, 32);
    if (!(view && fvm_desktop_window_events(view, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view->event_count = view->event_head = 0;
    memset(&t, 0, sizeof(t));
    t.view = view;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_load_swf(c, keypress_movie, sizeof(keypress_movie), "root", &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_get(c, root, "actor", &actor) == FVM_OK && actor.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_register_function(c, "keyDownHost", down, &t) == FVM_OK &&
          fvm_register_function(c, "keyPressHost", press, &t) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, actor.object, "onKeyDown", method, &t, 0) == FVM_OK &&
          fvm_input_set_keyboard_focus(c, actor.object) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &listener) == FVM_OK &&
          fvm_object_define_method_rooted(c, listener, "onKeyDown", observe, &t, 0) == FVM_OK &&
          fvm_input_add_key_listener(c, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(view, SDL_KEYDOWN, SDLK_LEFT, 0, 0);
    key(view, SDL_KEYDOWN, SDLK_LEFT, 0, 1);
    key(view, SDL_KEYUP, SDLK_LEFT, 0, 0);
    key(view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT, 0);
    key(view, SDL_KEYDOWN, SDLK_a, KMOD_CAPS | KMOD_SHIFT, 0);
    key(view, SDL_KEYDOWN, SDLK_F1, 0, 0);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.presses == 4 && t.methods == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.keys[0] == 1 && t.keys[1] == 1 && t.keys[2] == 65 && t.keys[3] == 97))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = t.presses;
    t.append = 1;
    key(view, SDL_KEYDOWN, SDLK_LEFT, 0, 0);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.presses == before + 1 &&
          view->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.presses == before + 2 &&
          t.keys[t.presses - 1] == 65))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.fail = t.presses + 1;
    key(view, SDL_KEYDOWN, SDLK_LEFT, 0, 0);
    key(view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT, 0);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_ERROR && view->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.fail = 0;
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && !view->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = t.presses;
    t.reset = 1;
    key(view, SDL_KEYDOWN, SDLK_LEFT, 0, 0);
    key(view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT, 0);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.presses == before &&
          !view->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(view, SDL_KEYDOWN, SDLK_a, KMOD_SHIFT, 0);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.presses == before + 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stage_take_update(c, &pending) == FVM_OK && pending && fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_window_close(view);
    fvm_context_destroy(c);
    fvm_runtime_destroy(runtime);
    return 0;
}
