/* SDK-free mixed delivery: order, snapshots, entry bounds, cancellation, errors, GC and teardown. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/core/runtime.hpp"
#include "flashvm/input_queue.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cstring>
#include <limits>


struct Trace
{
    int kinds[64];
    double x[64], y[64];
    unsigned n;
    bool append, cancel;
    FVM_Result error;
};


struct Binding
{
    Trace *trace;
    int kind;
};


static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *raw)
{
    Binding *b = (Binding *)raw;
    Trace *t = b->trace;
    if (!(t->n < 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned at = t->n++;
    t->kinds[at] = b->kind;
    if (!(fvm_input_get_pointer(c, t->x + at, t->y + at) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned nested = 99;
    if (!(c->pumping_input && fvm_input_pump(c, 0, &nested) == FVM_INVALID_ARGUMENT && nested == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    if (!(c->pumping_input))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (b->kind == 2)
    {
        FVM_KeyboardState s;
        s.struct_size = sizeof(s);
        if (!(fvm_input_get_keyboard(c, &s) == FVM_OK && s.down[FVM_KEY_LETTER_A] && s.last_ascii == 'A'))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->cancel)
    {
        t->cancel = false;
        if (!(fvm_input_clear_queue(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, 999, 888) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->append)
    {
        if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, 111, 222) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return t->error;
}


static void gesture(FVM_Context *c, const FVM_InputEvent *, FVM_InputPhase, FVM_MouseButton, void *raw)
{
    Binding *b = (Binding *)raw;
    Trace *t = b->trace;
    unsigned at = t->n++;
    t->kinds[at] = 3;
    if (!(fvm_input_get_pointer(c, t->x + at, t->y + at) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Trace t;
    std::memset(&t, 0, sizeof(t));
    t.error = FVM_OK;
    Binding b[5];
    const char *names[] = {"onMouseDown", "onKeyDown", "onMouseUp", "onMouseMove"};
    if (!(fvm_object_create(c, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        b[i].trace = &t;
        b[i].kind = i == 1 ? 2 : i == 2 ? 4 : 1;
        if (!(fvm_object_define_method_rooted(c, o, names[i], receive, b + i, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        o->methods[names[i]].collector_entry = true;
    }
    if (!(fvm_input_add_mouse_listener(c, o) == FVM_OK && fvm_input_add_key_listener(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_unpin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    b[4].trace = &t;
    if (!(fvm_input_set_callback(c, gesture, b + 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_KeyboardState key;
    std::memset(&key, 0, sizeof(key));
    key.struct_size = sizeof(key);
    key.down[FVM_KEY_LETTER_A] = 1;
    key.last_code = FVM_KEY_LETTER_A;
    key.last_ascii = 'A';
    FVM_InputEvent e = {FVM_GESTURE_TAP, 1, 0, 50, 60};
    unsigned n = 99, pending = 0;
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_DOWN, 10, 20) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_keyboard(c, &key, 1, 30, 40) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue(c, &e, FVM_INPUT_END) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_UP, 70, 80) == FVM_OK && !t.n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key.down[FVM_KEY_LETTER_A] = 0;
    key.last_ascii = 0;
    e.x = -999;
    if (!(fvm_input_pump(c, 2, &n) == FVM_OK && n == 2 && t.n == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 2 && t.n == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        if (!(t.kinds[i] == (int)i + 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(t.x[i] == 10 + 20 * i))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    t.append = true;
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, 1, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 1 && t.n == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.append = false;
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 1 && t.x[5] == 111))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.cancel = true;
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_DOWN, 3, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_UP, 5, 6) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 1 && t.x[7] == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.error = FVM_UNSUPPORTED;
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_DOWN, 7, 8) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_UP, 9, 10) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_UNSUPPORTED && n == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.error = FVM_OK;
    if (!(fvm_input_pump(c, 0, &n) == FVM_OK && n == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, (FVM_MouseEventType)99, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_keyboard(c, &key, 2, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, std::numeric_limits<double>::quiet_NaN(), 0) ==
          FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key.down[1] = 2;
    if (!(fvm_input_enqueue_keyboard(c, &key, 1, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = true;
    pending = 99;
    if (!(fvm_input_pending(c, &pending) == FVM_INVALID_ARGUMENT && pending == 99))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_clear_queue(c) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &n) == FVM_INVALID_ARGUMENT && !n))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    for (unsigned i = 0; i < FVM_INPUT_MAX_PENDING_EVENTS; ++i)
    {
        if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_MOVE, i, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_UP, 0, 0) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == FVM_INPUT_MAX_PENDING_EVENTS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_clear_queue(c) == FVM_OK && fvm_input_pending(c, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
