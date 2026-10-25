/* Inherited handlers detect ranges; nested dimensions remain authoritative and recipient roots survive removal. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_geometry_events_fixture.hpp"

#ifdef FVM_HAS_FONTS
struct GeometryReentry
{
    GeometryEvents *events;
    FVM_Object *listener;
    bool nested;
};


static FVM_Result
geometry_mutate(FVM_Context *c, FVM_Object *field, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    GeometryReentry *job = (GeometryReentry *)raw;
    if (!(field == job->events->field && n == 1 && args[0].object == field))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    job->events->order[job->events->count++] = job->nested ? 2 : 1;
    if (!job->nested)
    {
        job->nested = true;
        if (!(scroll_listen(c, field, job->listener, "removeListener")))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        scroll_number(c, field, "_height", 200);
    }
    fvm_value_undefined(out);
    return FVM_OK;
}
#endif


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
    GeometryObserver observer = {&events, 3};
    FVM_Object *listener = geometry_listener(c, &observer), *prototype = 0;
    GeometryReentry job = {&events, listener, false};
    if (!(fvm_object_create(c, &prototype) == FVM_OK &&
          fvm_object_define_method_rooted(c, prototype, "onScroller", geometry_mutate, &job, events.field) == FVM_OK &&
          fvm_object_set_prototype(c, events.field, prototype) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "_width", 60);
    const unsigned expected[] = {1, 2, 3};
    if (!(events.count == 3 && !std::memcmp(events.order, expected, sizeof(expected)) && events.last.maxscroll == 1 &&
          events.last.maxhscroll == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(counted_metric(c, events.field, "_width") == 60 && counted_metric(c, events.field, "_height") == 200 &&
          scroll_info(c, events.field).maxscroll == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events.count = 0;
    scroll_number(c, events.field, "_height", 48);
    if (!(events.count == 1 && events.order[0] == 2))
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
