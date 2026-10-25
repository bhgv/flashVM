/* Four OS workers own independent text/layout/listener graphs across 128 edit and GC epochs. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"
#include "../ports/thread_port.h"

#ifdef FVM_HAS_FONTS
struct ScrollEventJob
{
    unsigned done;
};


static void event_worker(void *raw)
{
    ScrollEventJob *job = (ScrollEventJob *)raw;
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
    scroll_number(c, events.field, "_width", 60);
    scroll_number(c, events.field, "_height", 200);
    GeometryObserver own = {&events, 1}, observer = {&events, 2};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", geometry_observe, &own, events.field) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry_listener(c, &observer);
    for (unsigned epoch = 0; epoch < 32; ++epoch)
    {
        events.count = 0;
        if (!(edit_text(c, events.field, epoch % 2 ? "AV\nAV\nAV" : "A", epoch % 2 ? 8 : 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(events.count == 2 && events.order[0] == 1 && events.order[1] == 2 && !events.last.maxhscroll &&
              events.last.maxscroll == 1 && events.last.bottom_scroll == (epoch % 2 ? 3UL : 1UL)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        ++job->done;
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
#endif


int main()
{
#ifdef FVM_HAS_FONTS
    ScrollEventJob jobs[4];
    FVM_PortThread *threads[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].done = 0;
        threads[i] = fvm_port_thread_start(event_worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(jobs[i].done == 32))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
#endif
    return 0;
}
