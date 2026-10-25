/* Native work limits share preparation/delivery budgets and preserve precommit versus committed semantics. */
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
    if (!(scroll_listen(c, events.field, a) && scroll_listen(c, events.field, b)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    unsigned precommit = 0, committed = 0, success = 0;
    for (unsigned limit = 1; limit <= 32; ++limit)
    {
        rt->config.instruction_limit = 0;
        events.top = 1;
        scroll_number(c, events.field, "scroll", 1);
        events.top = 2;
        events.count = 0;
        FVM_Value value;
        fvm_value_number(&value, 2);
        rt->config.instruction_limit = limit;
        FVM_Result result = fvm_object_set(c, events.field, "scroll", &value);
        rt->config.instruction_limit = 0;
        if (!(result == FVM_OK || result == FVM_ERROR))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
        if (result == FVM_OK)
        {
            ++success;
            if (!(info.scroll == 2 && events.count == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else if (info.scroll == 1)
        {
            ++precommit;
            if (!(!events.count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            ++committed;
            if (!(info.scroll == 2 && events.count < 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(precommit && committed && success))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Value arg, out;
    fvm_value_undefined(&arg);
    arg.type = FVM_VALUE_OBJECT;
    arg.object = a;
    rt->config.instruction_limit = 1;
    if (!(fvm_object_call(c, events.field, "removeListener", 1, &arg, &out) == FVM_ERROR))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    rt->config.instruction_limit = 0;
    FVM_Object *registry = 0;
    if (!(fvm_textfield_listener_registry(c, events.field, &registry) == FVM_OK && registry->indexed.size() == 2))
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
