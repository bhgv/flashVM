/* Recipient snapshots survive live-list edits, while handler lookup and nested state remain live. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_scroll_events_fixture.hpp"

#ifdef FVM_HAS_FONTS
struct ScrollReentry
{
    ScrollEvents *events;
    FVM_Object *removed, *added;
    ScrollObserver *replacement;
    bool nested;
};


static FVM_Result
scroll_mutate(FVM_Context *c, FVM_Object *field, unsigned n, const FVM_Value *args, FVM_Value *out, void *raw)
{
    ScrollReentry *job = (ScrollReentry *)raw;
    if (!(n == 1 && args[0].object == field && field == job->events->field))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    job->events->trace[job->events->count++] = job->nested ? 2 : 1;
    if (!job->nested)
    {
        job->nested = true;
        if (!(scroll_listen(c, field, job->removed, "removeListener") && scroll_listen(c, field, job->added)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, job->removed, "onScroller", scroll_observe, job->replacement, field) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        job->events->horizontal = 5;
        scroll_number(c, field, "hscroll", 5);
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
    ScrollEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    events.top = 2;
    events.horizontal = 0;
    ScrollObserver first = {&events, 20}, second = {&events, 30}, third = {&events, 40}, replacement = {&events, 31};
    FVM_Object *a = scroll_observer(c, &first), *b = scroll_observer(c, &second), *added = scroll_observer(c, &third);
    if (!(fvm_gc_pin(c, added) == FVM_OK && scroll_listen(c, events.field, a) && scroll_listen(c, events.field, b)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ScrollReentry job;
    job.events = &events;
    job.removed = b;
    job.added = added;
    job.replacement = &replacement;
    job.nested = false;
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", scroll_mutate, &job, events.field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    scroll_number(c, events.field, "scroll", 2);
    const unsigned expected[] = {1, 2, 20, 40, 20, 31};
    if (!(events.count == 6 && !std::memcmp(events.trace, expected, sizeof(expected))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
    if (!(info.scroll == 2 && info.hscroll == 5))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    events.count = 0;
    events.top = 1;
    scroll_number(c, events.field, "scroll", 1);
    const unsigned later[] = {2, 20, 40};
    if (!(events.count == 3 && !std::memcmp(events.trace, later, sizeof(later))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
