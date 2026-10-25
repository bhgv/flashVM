/* replaceText handles counted NUL, aliased source strings and range-neutral/no-op edits. */
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
    GeometryObserver own = {&events, 1}, listener = {&events, 2};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", geometry_observe, &own, events.field) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry_listener(c, &listener);
    if (!(edit_replace(c, events.field, 0, 8, "A", 1) == FVM_OK && events.count == 2 && events.last.maxscroll == 1 &&
          events.last.maxhscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, events.field, "A", 1);
    if (!(edit_replace(c, events.field, 1, 1, "\0BBBBBBBB", 9) == FVM_OK && events.count == 4 &&
          events.last.maxhscroll > 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, events.field, "A\0BBBBBBBB", 10);
    FVM_Value args[3], out;
    fvm_value_number(args, 0);
    fvm_value_number(args + 1, 10);
    if (!(fvm_object_get(c, events.field, "text", args + 2) == FVM_OK &&
          fvm_object_call(c, events.field, "replaceText", 3, args, &out) == FVM_OK && events.count == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, events.field, "A\0BBBBBBBB", 10);
    if (!(edit_replace(c, events.field, 0, 0, "", 0) == FVM_OK && events.count == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(edit_replace(c, events.field, 0, 10, "", 0) == FVM_OK && events.count == 6 && events.last.maxscroll == 1 &&
          !events.last.maxhscroll))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    counted_text(c, events.field, "", 0);
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
