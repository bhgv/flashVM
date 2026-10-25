/* Width/height notify range-only changes, publish clamped offsets and suppress unchanged ranges. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_geometry_events_fixture.hpp"


int main()
{
#ifdef FVM_HAS_FONTS
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    GeometryEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    scroll_number(c, events.field, "scroll", 2);
    scroll_number(c, events.field, "hscroll", 7);
    GeometryObserver own = {&events, 1}, listener = {&events, 2};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", geometry_observe, &own, events.field) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry_listener(c, &listener);
    scroll_number(c, events.field, "_width", 60);
    if (!(events.count == 2 && events.last.scroll == 2 && !events.last.hscroll && !events.last.maxhscroll &&
          events.order[0] == 1 && events.order[1] == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "_height", 200);
    if (!(events.count == 4 && events.last.scroll == 1 && events.last.maxscroll == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "_width", 40);
    if (!(events.count == 6 && events.last.scroll == 1 && events.last.maxhscroll == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "_width", 40);
    scroll_number(c, events.field, "_height", 201);
    scroll_number(c, events.field, "_x", 9);
    scroll_number(c, events.field, "textColor", 255);
    if (!(events.count == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "_height", 48);
    if (!(events.count == 8 && events.last.maxscroll == 3 && events.last.scroll == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(c, events.field, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(events.count == 8 && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth &&
          !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
