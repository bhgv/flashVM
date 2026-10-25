/* Real SDL adapter: namespace publication, retained clip input, primary touch, errors and FIFO deferral. */
#include <stdio.h>
#include <stdlib.h>
#include "../common/sdl_window.c"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/gc.h"
#include "flashvm/stage.h"
#include "clip_input_movie.h"


/* Borrowed host owners and bounded observations for one synchronous SDL test. */
struct Trace
{
    FVM_DesktopWindow *view;     /* Native window owned by main until test cleanup. */
    FVM_Object *root;            /* [GC:] Borrowed root held by main's pin. */
    unsigned count;              /* Retained callback entries. */
    unsigned namespace_count;    /* Native namespace observer entries before records. */
    unsigned method_count;       /* Optional MovieClip method entries after records. */
    unsigned last_method_record; /* Last retained entry matched by a method. */
    unsigned events[32];         /* Independent wire event selectors seen by records. */
    unsigned append;             /* Append one transition from a retained callback. */
    unsigned fail;               /* Retained callback ordinal that returns an error, or zero. */
    unsigned method_fail;        /* Optional method ordinal that returns an error, or zero. */
    unsigned method_append;      /* Append one transition from an optional method. */
    double xs[32], ys[32];       /* Event-time world coordinates sampled in records. */
    unsigned keys[32];           /* Event-time held left-arrow state sampled in records. */
};


/* Feed a copied mouse movement through the production SDL event decoder. */
static void motion(FVM_DesktopWindow *v, int x, int y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = SDL_MOUSEMOTION;
    e.motion.x = x;
    e.motion.y = y;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Feed a primary-button transition through the production decoder. */
static void button(FVM_DesktopWindow *v, Uint32 type, int x, int y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.button.button = SDL_BUTTON_LEFT;
    e.button.x = x;
    e.button.y = y;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Feed a supported left-arrow key transition. */
static void key(FVM_DesktopWindow *v, Uint32 type)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.key.keysym.sym = SDLK_LEFT;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Feed the primary touch contact without generated mouse duplicates. */
static void finger(FVM_DesktopWindow *v, Uint32 type, float x, float y)
{
    SDL_Event e;
    memset(&e, 0, sizeof(e));
    e.type = type;
    e.tfinger.touchId = 10;
    e.tfinger.fingerId = 111;
    e.tfinger.x = x;
    e.tfinger.y = y;
    if (!(handle_event(v, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


/* Observe namespace delivery before any retained record of the same transition. */
static FVM_Result
namespace_event(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    (void)c;
    (void)self;
    (void)a;
    if (!(!n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++t->namespace_count;
    fvm_value_undefined(out);
    return FVM_OK;
}


/* Record state and test retained-stage append/error/reentry behavior. */
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    unsigned i = t->count++;
    FVM_KeyboardState state;
    if (!(t->namespace_count == t->count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    (void)self;
    if (!(n == 1 && a[0].type == FVM_VALUE_NUMBER && i < 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->events[i] = (unsigned)a[0].number;
    if (!(fvm_input_get_pointer(c, t->xs + i, t->ys + i) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    if (!(fvm_input_get_keyboard(c, &state) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->keys[i] = state.down[FVM_KEY_LEFT];
    if (!(fvm_gc_collect(c, 0) == FVM_INVALID_ARGUMENT &&
          fvm_desktop_window_pump_clip_input(t->view, c, t->root, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->append)
    {
        t->append = 0;
        motion(t->view, 70, 80);
    }
    fvm_value_undefined(out);
    return t->fail == t->count ? FVM_ERROR : FVM_OK;
}


/* Optional clip methods follow the retained callback and observe the same event-time snapshot. */
static FVM_Result
clip_method(FVM_Context *c, FVM_Object *self, unsigned n, const FVM_Value *a, FVM_Value *out, void *raw)
{
    struct Trace *t = (struct Trace *)raw;
    double x, y;
    FVM_KeyboardState state;
    (void)self;
    (void)a;
    if (!(!n && t->count && t->last_method_record < t->count && t->namespace_count == t->count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->last_method_record = t->count;
    ++t->method_count;
    if (!(fvm_input_get_pointer(c, &x, &y) == FVM_OK && x == t->xs[t->count - 1] && y == t->ys[t->count - 1]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memset(&state, 0, sizeof(state));
    state.struct_size = sizeof(state);
    if (!(fvm_input_get_keyboard(c, &state) == FVM_OK && state.down[FVM_KEY_LEFT] == t->keys[t->count - 1]))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->method_append)
    {
        t->method_append = 0;
        motion(t->view, 90, 60);
    }
    if (!(fvm_call(c, "updateAfterEvent", 0, 0, out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return t->method_fail == t->method_count ? FVM_ERROR : FVM_OK;
}


/* Exercise actual adapter composition and release every native/managed owner. */
int main(void)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *root = 0, *mouse = 0, *keyboard = 0, *actor = 0;
    FVM_Value actor_value;
    unsigned i;
    const char *names[] = {"onMouseMove", "onMouseDown", "onMouseUp", "onKeyDown", "onKeyUp"};
    FVM_DesktopWindow *view;
    struct Trace t;
    unsigned before;
    int pending;
    if (!(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view = fvm_desktop_window_open("clip-input", 200, 100);
    if (!(view && fvm_desktop_window_events(view, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    view->event_count = view->event_head = 0;
    memset(&t, 0, sizeof(t));
    t.view = view;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_load_swf(c, input_movie, sizeof(input_movie), "root", &root) == FVM_OK &&
          fvm_gc_pin(c, root) == FVM_OK && fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.root = root;
    if (!(fvm_object_get(c, root, "actor", &actor_value) == FVM_OK && actor_value.type == FVM_VALUE_OBJECT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    actor = actor_value.object;
    if (!(fvm_input_set_keyboard_focus(c, actor) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 5; ++i)
    {
        if (!(fvm_object_define_method_rooted(c, actor, names[i], clip_method, &t, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_register_function(c, "clipInput", receive, &t) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(c, &mouse) == FVM_OK && fvm_object_create(c, &keyboard) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(c, mouse, "onMouseMove", namespace_event, &t, 0) == FVM_OK &&
          fvm_object_define_method_rooted(c, mouse, "onMouseDown", namespace_event, &t, 0) == FVM_OK &&
          fvm_object_define_method_rooted(c, mouse, "onMouseUp", namespace_event, &t, 0) == FVM_OK &&
          fvm_object_define_method_rooted(c, keyboard, "onKeyDown", namespace_event, &t, 0) == FVM_OK &&
          fvm_object_define_method_rooted(c, keyboard, "onKeyUp", namespace_event, &t, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(c, mouse) == FVM_OK && fvm_input_add_key_listener(c, keyboard) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }


    motion(view, 10, 20);
    button(view, SDL_MOUSEBUTTONDOWN, 12, 22);
    key(view, SDL_KEYDOWN);
    key(view, SDL_KEYUP);
    button(view, SDL_MOUSEBUTTONUP, 30, 40);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 5, 7) == FVM_OK && t.count == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.events[0] == 8 && t.xs[0] == 15 && t.ys[0] == 27 && t.events[1] == 16 && t.events[2] == 64 && t.keys[2] &&
          t.events[3] == 128 && !t.keys[3] && t.events[4] == 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    finger(view, SDL_FINGERDOWN, .25f, .5f);
    finger(view, SDL_FINGERMOTION, .5f, .25f);
    finger(view, SDL_FINGERUP, .5f, .25f);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 5, 7) == FVM_OK && t.count == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.events[5] == 16 && t.xs[5] == 55 && t.ys[5] == 57 && t.events[6] == 8 && t.events[7] == 32))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    before = t.count;
    t.append = 1;
    motion(view, 1, 2);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.count == before + 1 &&
          view->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.count == before + 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.fail = t.count + 1;
    motion(view, 2, 3);
    motion(view, 4, 5);
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
    before = t.count;
    t.method_append = 1;
    motion(view, 3, 4);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.count == before + 1 &&
          view->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && t.count == before + 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.method_fail = t.method_count + 1;
    motion(view, 5, 6);
    motion(view, 7, 8);
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_ERROR && view->event_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.method_fail = 0;
    if (!(fvm_desktop_window_pump_clip_input(view, c, root, 0, 0) == FVM_OK && !view->event_count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.method_count == t.count - 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_stage_take_update(c, &pending) == FVM_OK && pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_desktop_window_close(view);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
