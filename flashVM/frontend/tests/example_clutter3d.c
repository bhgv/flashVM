/* Real Clutter SDK actors/projection, texture upload and tutorial primary-touch callbacks. */
#define main tutorial_main
#include "../../examples/frontends/clutter3d/main.c"
#undef main
#include <assert.h>
static void position(Demo *d, double x, double y)
{
    double a, b;
    assert(fvm_input_get_pointer(d->context, &a, &b) == FVM_OK);
    if (!(a > x - .01 && a < x + .01 && b > y - .01 && b < y + .01))
        fprintf(stderr, "position got %.9f,%.9f expected %.9f,%.9f\n", a, b, x, y);
    assert(a > x - .01 && a < x + .01 && b > y - .01 && b < y + .01);
}
int main(void)
{
    ClutterHost h; ClutterEvent *e; ClutterActorBox box = {20, 10, 180, 110};
    int first, other; unsigned i;
    unsigned char pixels[DEMO_WIDTH * DEMO_HEIGHT * FVM_HOST_RGBA_BYTES];
    CoglHandle texture;
    ClutterVertex local = {40.125f, 25.0625f, 0}, projected;
    ClutterActorBox fractional = {20.25f, 10.5f, 180.75f, 110.75f};
    memset(&h, 0, sizeof(h)); h.touch_only = 1;
    assert(clutter_init(NULL, NULL) == CLUTTER_INIT_SUCCESS);
    assert(demo_open(&h.demo) == FVM_OK);
    h.stage = clutter_stage_new(); g_object_ref_sink(h.stage);
    h.texture = clutter_texture_new(); clutter_actor_add_child(h.stage, h.texture);
    clutter_actor_set_size(h.stage, DEMO_WIDTH, DEMO_HEIGHT);
    clutter_actor_set_size(h.texture, DEMO_WIDTH, DEMO_HEIGHT);
    g_signal_connect(h.stage, "deactivate", G_CALLBACK(clutter_deactivate), &h);
    g_signal_connect(h.stage, "destroy", G_CALLBACK(clutter_destroy), &h);
    assert(demo_frame(&h.demo, clutter_present, &h) == FVM_OK);
    texture = clutter_texture_get_cogl_texture(CLUTTER_TEXTURE(h.texture));
    assert(cogl_texture_get_data(texture, COGL_PIXEL_FORMAT_RGBA_8888,
                                 DEMO_WIDTH * FVM_HOST_RGBA_BYTES, pixels) == sizeof(pixels));
    assert(pixels[(100 * DEMO_WIDTH + 160) * 4] == DEMO_RELEASED_RED);
    assert(pixels[(100 * DEMO_WIDTH + 160) * 4 + 1] == DEMO_RELEASED_GREEN);
    assert(pixels[(100 * DEMO_WIDTH + 160) * 4 + 3] == DEMO_OPAQUE);
    clutter_actor_show(h.stage);
    for (i = 0; i < 20; ++i) {
        while (g_main_context_iteration(NULL, FALSE)) {}
        g_usleep(1000);
    }
    clutter_actor_allocate(h.texture, &box, CLUTTER_ALLOCATION_NONE);
    e = clutter_event_new(CLUTTER_TOUCH_BEGIN);
    e->touch.sequence = (ClutterEventSequence *)&first;
    e->touch.x = 60; e->touch.y = 35;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held);
    position(&h.demo, 80, 50);
    e->touch.sequence = (ClutterEventSequence *)&other;
    e->touch.x = 100; e->touch.y = 80;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP);
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_CANCEL;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held);
    e->touch.sequence = (ClutterEventSequence *)&first;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && !h.demo.held && !h.sequence);
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_BEGIN; e->touch.x = 60; e->touch.y = 35;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held);
    g_signal_emit_by_name(h.stage, "deactivate");
    assert(!h.demo.held && !h.sequence); position(&h.demo, 80, 50);
    /* The same inverse pipeline must work for a genuinely projected 3D actor. */
    clutter_actor_allocate(h.texture, &fractional, CLUTTER_ALLOCATION_NONE);
    clutter_actor_set_rotation(h.texture, CLUTTER_Y_AXIS, 25, 0, 0, 0);
    clutter_actor_apply_transform_to_point(h.texture, &local, &projected);
    e->type = CLUTTER_TOUCH_BEGIN; e->touch.x = projected.x; e->touch.y = projected.y;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && h.demo.held);
    position(&h.demo, 80, 50);
    e->type = CLUTTER_TOUCH_CANCEL;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_STOP && !h.demo.held);
    position(&h.demo, 80, 50);
    clutter_event_free(e);
    e = clutter_event_new(CLUTTER_BUTTON_PRESS); e->button.button = 1;
    e->button.x = 100; e->button.y = 80;
    assert(clutter_input(h.stage, e, &h) == CLUTTER_EVENT_PROPAGATE);
    position(&h.demo, 80, 50); clutter_event_free(e);
    assert(demo_frame(&h.demo, clutter_present, &h) == FVM_OK && h.demo.frames == 2);
    clutter_actor_destroy(h.stage); assert(h.destroyed && !h.texture);
    assert(clutter_present(&h.demo.frame, &h) == FVM_ERROR);
    g_object_unref(h.stage); demo_close(&h.demo);
    return 0;
}
