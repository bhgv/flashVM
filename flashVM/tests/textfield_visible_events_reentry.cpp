/* Bottom-only outer edits keep nested text/current style and snapshot recipient identities. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"

#ifdef FVM_HAS_FONTS
struct VisibleReentry
{
    GeometryEvents *events;
    FVM_Object *listener;
    bool nested;
    unsigned first_bottom;
};


static FVM_Result
visible_mutate(FVM_Context *c, FVM_Object *field, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    VisibleReentry *job = (VisibleReentry *)raw;
    if (!(field == job->events->field && n == 1 && args[0].object == field))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    job->events->order[job->events->count++] = job->nested ? 2 : 1;
    if (!job->nested)
    {
        if (!(scroll_info(c, field).bottom_scroll == job->first_bottom))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        job->nested = true;
        if (!(scroll_listen(c, field, job->listener, "removeListener") && edit_text(c, field, "A\nA", 3) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
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
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        GeometryEvents events;
        events.field = scroll_field(c);
        events.count = 0;
        if (mode == 2)
        {
            FVM_Value value;
            fvm_value_boolean(&value, 1);
            if (!(fvm_object_set(c, events.field, "wordWrap", &value) == FVM_OK &&
                  fvm_object_set(c, events.field, "autoSize", &value) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            scroll_number(c, events.field, "_width", 60);
            scroll_number(c, events.field, "_height", 200);
        }
        GeometryObserver observer = {&events, 3};
        FVM_Object *listener = geometry_listener(c, &observer), *format = edit_format(c, 20);
        VisibleReentry job = {&events, listener, false, mode == 2 ? 3U : 1U};
        if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", visible_mutate, &job, events.field) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!((mode == 0   ? edit_text(c, events.field, "A", 1)
               : mode == 1 ? edit_replace(c, events.field, 0, 8, "A", 1)
                           : edit_apply(c, events.field, format)) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        const unsigned expected[] = {1, 2, 3};
        if (!(events.count == 3 && !std::memcmp(events.order, expected, sizeof(expected)) &&
              events.last.bottom_scroll == 2 && events.last.maxscroll == 1 && !events.last.maxhscroll))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        counted_text(c, events.field, "A\nA", 3);
        FVM_Value current;
        if (!(fvm_object_call(c, events.field, "getTextFormat", 0, 0, &current) == FVM_OK &&
              counted_metric(c, current.object, "size") == 40))
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
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
