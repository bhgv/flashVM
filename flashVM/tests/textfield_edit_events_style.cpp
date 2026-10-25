/* Current formats notify changed ranges; future formats and decoration-only changes stay silent. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"


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
    FVM_Object *format = edit_format(c, 20);
    if (!(edit_apply(c, events.field, format) == FVM_OK && events.count == 2 && events.last.maxscroll == 1 &&
          events.last.maxhscroll == 0 && events.last.scroll == 1 && events.last.hscroll == 0 &&
          counted_metric(c, events.field, "textWidth") == 24))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, format, "size", 60);
    if (!(edit_apply(c, events.field, format) == FVM_OK && events.count == 4 && events.last.maxscroll == 3 &&
          events.last.maxhscroll == 36 && counted_metric(c, events.field, "textWidth") == 72))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, format, "size", 40);
    if (!(edit_apply(c, events.field, format, "setNewTextFormat") == FVM_OK && events.count == 4 &&
          counted_metric(c, events.field, "textWidth") == 72))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(edit_text(c, events.field, "AV\nAV\nAV", 8) == FVM_OK && events.count == 6 && events.last.maxscroll == 2 &&
          events.last.maxhscroll == 12))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, format, "color", 255);
    FVM_Value value;
    fvm_value_boolean(&value, 1);
    if (!(fvm_object_set(c, format, "underline", &value) == FVM_OK && edit_apply(c, events.field, format) == FVM_OK &&
          events.count == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_object_set(c, format, "kerning", &value) == FVM_OK && edit_apply(c, events.field, format) == FVM_OK &&
          events.count == 8 && events.last.maxhscroll == 9))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, format, "leading", 50);
    if (!(edit_apply(c, events.field, format) == FVM_OK && events.count == 10 && events.last.maxscroll == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
