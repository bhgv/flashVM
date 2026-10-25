/* Field-first listener order, source arguments, duplicates, removal and unchanged scroll suppression. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_fixture.hpp"


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
    ScrollEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    events.top = 2;
    events.horizontal = 0;
    ScrollObserver own = {&events, 10}, first = {&events, 20}, second = {&events, 30};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", scroll_observe, &own, events.field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *a = scroll_observer(c, &first), *b = scroll_observer(c, &second);
    if (!(!scroll_listen(c, events.field, 0)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(scroll_listen(c, events.field, a) && scroll_listen(c, events.field, b) && scroll_listen(c, events.field, a)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "scroll", 2);
    if (!(events.count == 3 && events.trace[0] == 10 && events.trace[1] == 20 && events.trace[2] == 30))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "scroll", 999);
    scroll_number(c, events.field, "scroll", 2);
    if (!(events.count == 3))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events.horizontal = 7;
    scroll_number(c, events.field, "hscroll", 7);
    if (!(events.count == 6 && events.trace[3] == 10 && events.trace[4] == 20 && events.trace[5] == 30))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(scroll_listen(c, events.field, a, "removeListener") && !scroll_listen(c, events.field, a, "removeListener")))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events.top = 1;
    scroll_number(c, events.field, "scroll", 1);
    if (!(events.count == 8 && events.trace[6] == 10 && events.trace[7] == 30))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
    FVM_Path *path = 0;
    if (!(fvm_textfield_path(c, events.field, &path) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_path_destroy(path);
    if (!(info.scroll == 1 && events.count == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
