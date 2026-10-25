/* Copied wheel events share FIFO, entry batches, cancellation, capacity and error semantics. */
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
    int kind[64];
    double delta[64], x[64];
    unsigned calls;
    bool append, cancel;
    FVM_Result status;
};


struct Binding
{
    Trace *trace;
    int kind;
};


static FVM_Result receive(FVM_Context *c, FVM_Object *, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    Binding *b = (Binding *)raw;
    Trace *t = b->trace;
    unsigned at = t->calls++;
    if (!(at < 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->kind[at] = b->kind;
    t->x[at] = c->pointer_world_x;
    if (b->kind == 2)
    {
        if (!(n == 1 && args[0].type == FVM_VALUE_NUMBER))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        t->delta[at] = args[0].number;
    }
    else
    {
        if (!(!n))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(c->pumping_input && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK))
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
    unsigned processed = 99;
    if (!(fvm_input_pump(c, 0, &processed) == FVM_INVALID_ARGUMENT && !processed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (t->cancel)
    {
        t->cancel = false;
        if (!(fvm_input_clear_queue(c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (t->append)
    {
        if (!(fvm_input_enqueue_mouse_wheel(c, 0.125, 111, 222) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_undefined(out);
    return t->status;
}


static void gesture(FVM_Context *c, const FVM_InputEvent *, FVM_InputPhase, FVM_MouseButton, void *raw)
{
    Trace *t = (Trace *)raw;
    unsigned at = t->calls++;
    if (!(at < 64))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t->kind[at] = 4;
    t->x[at] = c->pointer_world_x;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    FVM_Context *c = 0;
    FVM_Object *o = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK &&
          fvm_context_create(r, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_standard_install(c) == FVM_OK && fvm_object_create(c, &o) == FVM_OK && fvm_gc_pin(c, o) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Trace t;
    std::memset(&t, 0, sizeof(t));
    t.status = FVM_OK;
    const char *names[] = {"onMouseDown", "onMouseWheel", "onKeyDown"};
    Binding b[3];
    for (unsigned i = 0; i < 3; ++i)
    {
        b[i].trace = &t;
        b[i].kind = i + 1;
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
    if (!(fvm_gc_unpin(c, o) == FVM_OK && fvm_input_set_callback(c, gesture, &t) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_KeyboardState key;
    std::memset(&key, 0, sizeof(key));
    key.struct_size = sizeof(key);
    key.down[FVM_KEY_LETTER_A] = 1;
    key.last_code = FVM_KEY_LETTER_A;
    FVM_InputEvent e = {FVM_GESTURE_TAP, 1, 0, 7, 8};
    unsigned count = 99, pending = 99;
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_DOWN, 1, 2) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, -1.5, 3, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_keyboard(c, &key, 1, 5, 6) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue(c, &e, FVM_INPUT_END) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, 2.5, 9, 10) == FVM_OK && !t.calls && c->pointer_world_x == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && fvm_input_pump(c, 2, &count) == FVM_OK && count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.calls == 2 && t.kind[0] == 1 && t.kind[1] == 2 && t.delta[1] == -1.5 && t.x[1] == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 3 && t.calls == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(t.kind[2] == 3 && t.x[2] == 5 && t.kind[3] == 4 && t.x[3] == 7 && t.kind[4] == 2 && t.delta[4] == 2.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.append = true;
    if (!(fvm_input_enqueue_mouse_wheel(c, 1, 11, 12) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 1 && t.calls == 6))
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
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 1 && t.delta[6] == 0.125))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.append = t.cancel = true;
    if (!(fvm_input_enqueue_mouse_wheel(c, 2, 13, 14) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse(c, FVM_MOUSE_EVENT_DOWN, 15, 16) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 1 && t.calls == 8))
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
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 1 && t.x[8] == 111))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.status = FVM_UNSUPPORTED;
    if (!(fvm_input_enqueue_mouse_wheel(c, 0, 17, 18) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, 1, 19, 20) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pump(c, 0, &count) == FVM_UNSUPPORTED && !count && !c->pumping_input))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == 1 && c->pointer_world_x == 17))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    t.status = FVM_OK;
    if (!(fvm_input_pump(c, 0, &count) == FVM_OK && count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const double bad[] = {std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()};
    for (unsigned i = 0; i < 2; ++i)
    {
        if (!(fvm_input_enqueue_mouse_wheel(c, bad[i], 0, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_enqueue_mouse_wheel(c, 1, bad[i], 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_input_enqueue_mouse_wheel(c, 1, 0, bad[i]) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    c->gc_collecting = true;
    if (!(fvm_input_enqueue_mouse_wheel(c, 1, 0, 0) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    if (!(fvm_input_pending(c, &pending) == FVM_OK && !pending))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < FVM_INPUT_MAX_PENDING_EVENTS; ++i)
    {
        if (!(fvm_input_enqueue_mouse_wheel(c, 1, i, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(fvm_input_enqueue_mouse_wheel(c, 1, 0, 0) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_pending(c, &pending) == FVM_OK && pending == FVM_INPUT_MAX_PENDING_EVENTS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_clear_queue(c) == FVM_OK && fvm_input_pump(c, 0, &count) == FVM_OK && !count))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
}
