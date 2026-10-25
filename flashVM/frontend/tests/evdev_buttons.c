/* Real tutorial decoder: combine native mouse/touch bits into one primary edge. */
#define main linux_tutorial_entry
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/linux_fb/main.c"

#undef main
#include "flashvm/value.h"

static unsigned down, up;


static FVM_Result on_down(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *r, void *u)
{
    (void)c;
    (void)o;
    (void)a;
    (void)u;
    if (!(n == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++down;
    fvm_value_undefined(r);
    return FVM_OK;
}


static FVM_Result on_up(FVM_Context *c, FVM_Object *o, unsigned n, const FVM_Value *a, FVM_Value *r, void *u)
{
    (void)c;
    (void)o;
    (void)a;
    (void)u;
    if (!(n == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++up;
    fvm_value_undefined(r);
    return FVM_OK;
}


static void key(PointerDevice *p, Demo *d, unsigned short code, int value)
{
    struct input_event e;
    memset(&e, 0, sizeof(e));
    e.type = EV_KEY;
    e.code = code;
    e.value = value;
    if (!(evdev_event(p, d, &e)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    Demo d;
    PointerDevice p;
    FVM_Object *listener;
    int removed;
    memset(&p, 0, sizeof(p));
    p.x = 100;
    p.y = 100;
    if (!(demo_open(&d) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_create(d.context, &listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(d.context, listener, "onMouseDown", on_down, NULL, NULL) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_define_method_rooted(d.context, listener, "onMouseUp", on_up, NULL, NULL) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_add_mouse_listener(d.context, listener) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 0);
    if (!(!down && !up && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 1);
    if (!(down == 1 && !up && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 1);
    key(&p, &d, BTN_LEFT, 2);
    if (!(down == 1 && !up))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_TOUCH, 1);
    if (!(down == 1 && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 0);
    if (!(!up && d.held && p.touch_down))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 0);
    if (!(!up && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_TOUCH, 0);
    if (!(up == 1 && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_TOUCH, 0);
    if (!(up == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_TOUCH, 1);
    key(&p, &d, BTN_LEFT, 1);
    if (!(down == 2 && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_TOUCH, 0);
    if (!(up == 1 && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_RIGHT, 0);
    key(&p, &d, KEY_A, 1);
    if (!(down == 2 && up == 1 && d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    key(&p, &d, BTN_LEFT, 0);
    if (!(up == 2 && !d.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_remove_mouse_listener(d.context, listener, &removed) == FVM_OK && removed))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    demo_close(&d);
    return 0;
}
