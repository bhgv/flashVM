/* Focused real-SDK wheel/cursor acceptance. Integer actor positions keep this
 * auxiliary test independent of the system SDK's known fractional inverse bug;
 * example_clutter3d.c retains the strict patched-fork projection regression. */
#define main tutorial_main
#include <stdio.h>
#include <stdlib.h>
#include "../../examples/frontends/clutter3d/main.c"

#undef main


int main(void)
{
    ClutterHost h;
    ClutterEvent *event;
    gboolean visible;
    int previous;
    unsigned i;
    double x, y;
    memset(&h, 0, sizeof(h));
    h.cursor_applied = -1;
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
    clutter_actor_show(h.stage);
    for (i = 0; i < 20; ++i)
    {
        while (g_main_context_iteration(NULL, FALSE))
        {
        }
        g_usleep(1000);
    }
    event = clutter_event_new(CLUTTER_SCROLL);
    event->scroll.x = 80;
    event->scroll.y = 50;
    event->scroll.direction = CLUTTER_SCROLL_UP;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_STOP))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 1 && h.demo.last_wheel_delta == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_get_pointer(h.demo.context, &x, &y) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(x > 79.99 && x < 80.01 && y > 49.99 && y < 50.01))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event->scroll.direction = CLUTTER_SCROLL_DOWN;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_STOP && h.demo.last_wheel_delta == -3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event->scroll.direction = CLUTTER_SCROLL_SMOOTH;
    clutter_event_set_scroll_delta(event, 0, .25);
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_STOP && h.demo.last_wheel_delta == -.75))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_event_set_scroll_delta(event, 1, 0);
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_PROPAGATE && h.demo.wheel_events == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event->scroll.direction = CLUTTER_SCROLL_LEFT;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_PROPAGATE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    event->scroll.direction = CLUTTER_SCROLL_UP;
    h.touch_only = 1;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_PROPAGATE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.touch_only = 0;
    h.sequence = (ClutterEventSequence *)&previous;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_PROPAGATE))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.sequence = NULL;
    h.demo.held = 1;
    if (!(clutter_input(h.stage, event, &h) == CLUTTER_EVENT_STOP && h.demo.held))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(h.demo.wheel_events == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    h.demo.held = 0;
    if (!(clutter_cursor(&h) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_get(h.stage, "cursor-visible", &visible, NULL);
    if (!(visible))
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
    g_object_get(h.stage, "cursor-visible", &visible, NULL);
    if (!(!visible))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_cursor(&h) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_input_set_cursor_visible(h.demo.context, 1, &previous) == FVM_OK && previous == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(clutter_cursor(&h) == FVM_OK && h.cursor_applied == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    g_object_get(h.stage, "cursor-visible", &visible, NULL);
    if (!(visible))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    clutter_event_free(event);
    clutter_actor_destroy(h.stage);
    g_object_unref(h.stage);
    h.destroyed = 1;
    h.texture = NULL;
    if (!(clutter_cursor(&h) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    demo_close(&h.demo);
    return 0;
}
