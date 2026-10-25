/* Four OS workers own separate VM/listener graphs across 128 synchronous scroll event epochs. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_fixture.hpp"
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
    ScrollEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    events.top = 1;
    events.horizontal = 0;
    ScrollObserver own = {&events, 10}, observer = {&events, 20};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", scroll_observe, &own, events.field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *listener = scroll_observer(c, &observer);
    if (!(scroll_listen(c, events.field, listener)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned epoch = 0; epoch < 32; ++epoch)
    {
        events.count = 0;
        events.top = 1 + epoch % 2;
        scroll_number(c, events.field, "scroll", events.top);
        events.horizontal = 1 + epoch % 11;
        scroll_number(c, events.field, "hscroll", events.horizontal);
        if (!(events.count == (epoch ? 4U : 2U)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        for (unsigned i = 0; i < events.count; i += 2)
        {
            if (!(events.trace[i] == 10 && events.trace[i + 1] == 20))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
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
