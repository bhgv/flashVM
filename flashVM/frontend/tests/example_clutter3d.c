/* Real Clutter SDK actors/projection, texture upload and tutorial primary-touch callbacks. */
#define main tutorial_main
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/clutter3d/main.c"

#undef main


static void position(Demo *d, double x, double y)
{
    double a, b;
    if (!(fvm_input_get_pointer(d->context, &a, &b) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(a > x - .01 && a < x + .01 && b > y - .01 && b < y + .01))
    {
        fprintf(stderr, "position got %.9f,%.9f expected %.9f,%.9f\n", a, b, x, y);
    }
    if (!(a > x - .01 && a < x + .01 && b > y - .01 && b < y + .01))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


int main(void)
{
    ClutterHost h;
    ClutterEvent *e;
    ClutterActorBox box = {20, 10, 180, 110};
    int first, other, previous;
    unsigned i;
    gboolean cursor;
    unsigned char pixels[DEMO_WIDTH * DEMO_HEIGHT * FVM_HOST_RGBA_BYTES];
    CoglHandle texture;
    ClutterVertex local = {40.125f, 25.0625f, 0}, projected;
    ClutterActorBox fractional = {20.25f, 10.5f, 180.75f, 110.75f};
    memset(&h, 0, sizeof(h));
    h.touch_only = 1;
    if (!(clutter_init(NULL, NULL) == CLUTTER_INIT_SUCCESS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_open(&h.demo) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.stage = clutter_stage_new();
    g_object_ref_sink(h.stage);
    h.texture = clutter_texture_new();
    clutter_actor_add_child(h.stage, h.texture);
    clutter_actor_set_size(h.stage, DEMO_WIDTH, DEMO_HEIGHT);
    clutter_actor_set_size(h.texture, DEMO_WIDTH, DEMO_HEIGHT);
    g_signal_connect(h.stage, "deactivate", G_CALLBACK(clutter_deactivate), &h);
    g_signal_connect(h.stage, "destroy", G_CALLBACK(clutter_destroy), &h);
    if (!(demo_frame(&h.demo, clutter_present, &h) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    texture = clutter_texture_get_cogl_texture(CLUTTER_TEXTURE(h.texture));
    if (!(cogl_texture_get_data(texture, COGL_PIXEL_FORMAT_RGBA_8888, DEMO_WIDTH * FVM_HOST_RGBA_BYTES, pixels) ==
          sizeof(pixels)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[(100 * DEMO_WIDTH + 160) * 4] == DEMO_RELEASED_RED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[(100 * DEMO_WIDTH + 160) * 4 + 1] == DEMO_RELEASED_GREEN))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(pixels[(100 * DEMO_WIDTH + 160) * 4 + 3] == DEMO_OPAQUE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_actor_show(h.stage);
    for (i = 0; i < 20; ++i)
    {
        while (g_main_context_iteration(NULL, FALSE))
        {
        }
        g_usleep(1000);
    }
    clutter_actor_allocate(h.texture, &box, CLUTTER_ALLOCATION_NONE);
    e = clutter_event_new(CLUTTER_TOUCH_BEGIN);
    e->touch.sequence = (ClutterEventSequence *)&first;
    e->touch.x = 60;
    e->touch.y = 35;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    e->touch.sequence = (ClutterEventSequence *)&other;
    e->touch.x = 100;
    e->touch.y = 80;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_CANCEL;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e->touch.sequence = (ClutterEventSequence *)&first;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && !h.demo.held && !h.sequence))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_BEGIN;
    e->touch.x = 60;
    e->touch.y = 35;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_signal_emit_by_name(h.stage, "deactivate");
    if (!(!h.demo.held && !h.sequence))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    /* The same inverse pipeline must work for a genuinely projected 3D actor. */
    clutter_actor_allocate(h.texture, &fractional, CLUTTER_ALLOCATION_NONE);
    clutter_actor_set_rotation(h.texture, CLUTTER_Y_AXIS, 25, 0, 0, 0);
    clutter_actor_apply_transform_to_point(h.texture, &local, &projected);
    e->type = CLUTTER_TOUCH_BEGIN;
    e->touch.x = projected.x;
    e->touch.y = projected.y;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_CANCEL;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && !h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    clutter_event_free(e);
    e = clutter_event_new(CLUTTER_BUTTON_PRESS);
    e->button.button = 1;
    e->button.x = 100;
    e->button.y = 80;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    clutter_event_free(e);
    h.touch_only = 0;
    e = clutter_event_new(CLUTTER_SCROLL);
    e->scroll.x = projected.x;
    e->scroll.y = projected.y;
    e->scroll.direction = CLUTTER_SCROLL_UP;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 1 && h.demo.last_wheel_delta == 3 && !h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    position(&h.demo, 80, 50);
    e->scroll.direction = CLUTTER_SCROLL_DOWN;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 2 && h.demo.last_wheel_delta == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e->scroll.direction = CLUTTER_SCROLL_LEFT;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE && h.demo.wheel_events == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    e->scroll.direction = CLUTTER_SCROLL_SMOOTH;
    clutter_event_set_scroll_delta(e, 1, .25);
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 3 && h.demo.last_wheel_delta == -.75))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_event_set_scroll_delta(e, 1, 0);
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE && h.demo.wheel_events == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_event_set_scroll_delta(e, 0, -.5);
    h.touch_only = 1;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE && h.demo.wheel_events == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.touch_only = 0;
    h.sequence = (ClutterEventSequence *)&first;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE && h.demo.wheel_events == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.sequence = NULL;
    h.demo.held = 1;
    if (!(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 4 && h.demo.last_wheel_delta == 1.5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.demo.held = 0;
    clutter_event_free(e);
    if (!(clutter_cursor(&h) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_get(h.stage, "cursor-visible", &cursor, NULL);
    if (!(cursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(h.demo.context, 0, &previous) == FVM_OK && previous == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_cursor(&h) == FVM_OK && !h.cursor_applied))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_get(h.stage, "cursor-visible", &cursor, NULL);
    if (!(!cursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_cursor(&h) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(h.demo.context, 1, &previous) == FVM_OK && !previous))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_cursor(&h) == FVM_OK && h.cursor_applied == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_get(h.stage, "cursor-visible", &cursor, NULL);
    if (!(cursor))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(demo_frame(&h.demo, clutter_present, &h) == FVM_OK && h.demo.frames == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_actor_destroy(h.stage);
    if (!(h.destroyed && !h.texture))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_present(&h.demo.frame, &h) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_unref(h.stage);
    demo_close(&h.demo);
    return 0;
}
