/* Shared input order and primary-finger routing using real SDL with an explicit dummy video driver. */
#include "../common/sdl_window.c"
#include "flashvm/standard.h"
#include <assert.h>
#include <math.h>
#define TRACE_LIMIT 64U
#define TRACE_DOWN 1U
#define TRACE_MOVE 2U
#define TRACE_UP 3U
#define TRACE_KEY_DOWN 4U
#define TRACE_KEY_UP 5U
struct Trace {
    FVM_DesktopWindow *view;
    unsigned int count, kinds[TRACE_LIMIT], fail_at;
    double xs[TRACE_LIMIT], ys[TRACE_LIMIT];
    int append, reset;
};
struct Receiver { struct Trace *trace; unsigned int kind; };
static void motion(FVM_DesktopWindow *v, int x, int y) {
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = SDL_MOUSEMOTION;
    e.motion.x = x; e.motion.y = y; assert(handle_event(v, &e));
}
static void button(FVM_DesktopWindow *v, Uint32 type, int x, int y) {
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = type;
    e.button.button = SDL_BUTTON_LEFT; e.button.x = x; e.button.y = y; assert(handle_event(v, &e));
}
static void key(FVM_DesktopWindow *v, Uint32 type, SDL_Keycode code) {
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = type;
    e.key.keysym.sym = code; assert(handle_event(v, &e));
}
static void finger(FVM_DesktopWindow *v, Uint32 type, SDL_TouchID device,
                    SDL_FingerID id, float x, float y) {
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = type;
    e.tfinger.touchId = device; e.tfinger.fingerId = id;
    e.tfinger.x = x; e.tfinger.y = y; assert(handle_event(v, &e));
}
static void lost(FVM_DesktopWindow *v) {
    SDL_Event e; memset(&e, 0, sizeof(e)); e.type = SDL_WINDOWEVENT;
    e.window.event = SDL_WINDOWEVENT_FOCUS_LOST; assert(handle_event(v, &e));
}
static FVM_Result receive(FVM_Context *c, FVM_Object *self, unsigned int n,
                          const FVM_Value *args, FVM_Value *out, void *raw) {
    struct Receiver *receiver = (struct Receiver *)raw;
    struct Trace *t = receiver->trace;
    unsigned int i = t->count++;
    (void)self; (void)args; assert(!n && i < TRACE_LIMIT);
    t->kinds[i] = receiver->kind;
    assert(fvm_input_get_pointer(c, t->xs + i, t->ys + i) == FVM_OK);
    assert(fvm_desktop_window_pump_input(t->view, c, 0, 0) == FVM_INVALID_ARGUMENT);
    if (t->append) { t->append = 0; motion(t->view, 70, 80); }
    if (t->reset) { t->reset = 0; lost(t->view); key(t->view, SDL_KEYDOWN, SDLK_a); }
    fvm_value_undefined(out);
    return t->fail_at == t->count ? FVM_UNSUPPORTED : FVM_OK;
}
static void expect(struct Trace *t, unsigned int i, unsigned int kind, double x, double y) {
    assert(i < t->count && t->kinds[i] == kind);
    assert(fabs(t->xs[i] - x) < 0.00001 && fabs(t->ys[i] - y) < 0.00001);
}
int main(void) {
    FVM_DesktopWindow *v;
    FVM_DesktopWindow invalid_view;
    FVM_Config config; FVM_Runtime *runtime = 0; FVM_Context *context = 0;
    FVM_Object *mouse = 0, *keyboard = 0;
    struct Trace t; struct Receiver receivers[5];
    const char *names[] = {"onMouseDown", "onMouseMove", "onMouseUp", "onKeyDown", "onKeyUp"};
    unsigned int i, start; SDL_Event synthetic;
    memset(&invalid_view, 0, sizeof(invalid_view));
    invalid_view.logical_width = 200; invalid_view.logical_height = 100;
    finger(&invalid_view, SDL_FINGERDOWN, 10, 111, 1.5f, .5f);
    assert(invalid_view.event_error == FVM_INVALID_ARGUMENT && !invalid_view.touch_active &&
           !invalid_view.event_count && invalid_view.input.x == 0 && invalid_view.input.y == 0);
    assert(SDL_setenv("SDL_VIDEODRIVER", "dummy", 1) == 0);
    v = fvm_desktop_window_open("primary-contact-fixture", 200, 100); assert(v);
    assert(fvm_desktop_window_events(v, 0)); v->event_count = v->event_head = 0;
    memset(&t, 0, sizeof(t)); t.view = v;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK && fvm_standard_install(context) == FVM_OK);
    assert(fvm_standard_mouse_install(context) == FVM_OK && fvm_standard_keyboard_install(context) == FVM_OK);
    assert(fvm_object_create(context, &mouse) == FVM_OK && fvm_object_create(context, &keyboard) == FVM_OK);
    for (i = 0; i < 5; ++i) {
        receivers[i].trace = &t; receivers[i].kind = i + 1;
        assert(fvm_object_define_method_rooted(context, i < 3 ? mouse : keyboard, names[i],
                                               receive, receivers + i, 0) == FVM_OK);
    }
    assert(fvm_input_add_mouse_listener(context, mouse) == FVM_OK);
    assert(fvm_input_add_key_listener(context, keyboard) == FVM_OK);
    motion(v, 10, 20); button(v, SDL_MOUSEBUTTONDOWN, 12, 22);
    key(v, SDL_KEYDOWN, SDLK_LEFT); motion(v, 30, 40); key(v, SDL_KEYUP, SDLK_LEFT);
    button(v, SDL_MOUSEBUTTONUP, 30, 40);
    assert(v->event_count == 6 && !t.count);
    assert(fvm_desktop_window_pump_input(v, context, HUGE_VAL, 0) == FVM_INVALID_ARGUMENT);
    assert(v->event_count == 6 && !t.count);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 6);
    expect(&t, 0, TRACE_MOVE, 15, 27); expect(&t, 1, TRACE_DOWN, 17, 29);
    expect(&t, 2, TRACE_KEY_DOWN, 17, 29); expect(&t, 3, TRACE_MOVE, 35, 47);
    expect(&t, 4, TRACE_KEY_UP, 35, 47); expect(&t, 5, TRACE_UP, 35, 47);
    finger(v, SDL_FINGERDOWN, 10, 111, .25f, .5f);
    finger(v, SDL_FINGERDOWN, 10, 222, .9f, .9f);
    memset(&synthetic, 0, sizeof(synthetic)); synthetic.type = SDL_MOUSEBUTTONDOWN;
    synthetic.button.which = SDL_TOUCH_MOUSEID; synthetic.button.button = SDL_BUTTON_LEFT;
    assert(handle_event(v, &synthetic)); motion(v, 199, 99);
    finger(v, SDL_FINGERMOTION, 10, 111, .5f, .25f);
    finger(v, SDL_FINGERUP, 10, 222, .9f, .9f);
    assert(v->touch_active);
    finger(v, SDL_FINGERUP, 10, 111, .75f, .5f);
    finger(v, SDL_FINGERUP, 10, 111, .75f, .5f);
#ifdef SDL_MOUSE_TOUCHID
    finger(v, SDL_FINGERDOWN, SDL_MOUSE_TOUCHID, 1, .5f, .5f);
#endif
    assert(!v->touch_active && !v->input.buttons && v->input.clicks == 2 && v->event_count == 3);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 9);
    expect(&t, 6, TRACE_DOWN, 55, 57); expect(&t, 7, TRACE_MOVE, 105, 32);
    expect(&t, 8, TRACE_UP, 155, 57);
    /* A physical held button owns the primary contact until release; focus loss cancels the FIFO. */
    button(v, SDL_MOUSEBUTTONDOWN, 1, 2); finger(v, SDL_FINGERDOWN, 10, 111, .5f, .5f);
    assert(!v->touch_active && v->event_count == 1); lost(v);
    assert(!v->event_count && !v->input.buttons);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 9);
    /* Callback enqueues are deferred to the next batch. */
    t.append = 1; button(v, SDL_MOUSEBUTTONDOWN, 2, 3);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 10 && v->event_count == 1);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 11);
    expect(&t, 10, TRACE_MOVE, 75, 87);
    /* Real handler errors consume only that event and leave later events available. */
    motion(v, 1, 2); button(v, SDL_MOUSEBUTTONDOWN, 3, 4); button(v, SDL_MOUSEBUTTONUP, 5, 6);
    t.fail_at = t.count + 1;
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_UNSUPPORTED && v->event_count == 2);
    t.fail_at = 0;
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 14);
    /* Focus loss during delivery cancels the old batch without consuming newly enqueued keys. */
    t.reset = 1; button(v, SDL_MOUSEBUTTONDOWN, 2, 3); button(v, SDL_MOUSEBUTTONUP, 4, 5);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 15 && v->event_count == 1);
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == 16);
    assert(t.kinds[15] == TRACE_KEY_DOWN);
    /* Exercise SDL's actual resize/letterbox event filter, not a second manual coordinate transform. */
    lost(v); SDL_SetWindowSize(v->window, 400, 100); assert(fvm_desktop_window_events(v, 0));
    v->event_count = v->event_head = 0; start = t.count;
    memset(&synthetic, 0, sizeof(synthetic)); synthetic.type = SDL_FINGERDOWN;
    synthetic.tfinger.touchId = 10; synthetic.tfinger.fingerId = 333;
    synthetic.tfinger.x = .375f; synthetic.tfinger.y = .5f;
    assert(SDL_PushEvent(&synthetic) == 1 && fvm_desktop_window_events(v, 0));
    memset(&synthetic, 0, sizeof(synthetic)); synthetic.type = SDL_FINGERUP;
    synthetic.tfinger.touchId = 10; synthetic.tfinger.fingerId = 333;
    synthetic.tfinger.x = .375f; synthetic.tfinger.y = .5f;
    assert(SDL_PushEvent(&synthetic) == 1 && fvm_desktop_window_events(v, 0));
    assert(fvm_desktop_window_pump_input(v, context, 5, 7) == FVM_OK && t.count == start + 2);
    expect(&t, start, TRACE_DOWN, 55, 57); expect(&t, start + 1, TRACE_UP, 55, 57);
    for (i = 0; i <= FVM_DESKTOP_PENDING_EVENTS; ++i) motion(v, 1, 2);
    assert(v->event_count == FVM_DESKTOP_PENDING_EVENTS && v->event_error == FVM_ERROR);
    start = t.count;
    assert(fvm_desktop_window_pump_input(v, context, 0, 0) == FVM_ERROR && t.count == start);
    fvm_context_destroy(context); fvm_runtime_destroy(runtime); fvm_desktop_window_close(v);
    return 0;
}
