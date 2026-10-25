/* Registration rollback and notification allocation failures distinguish precommit from committed state. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "textfield_geometry_events_fixture.hpp"


int main(int argc, char **)
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
    GeometryObserver own = {&events, 1}, first = {&events, 2};
    if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", geometry_observe, &own, events.field) ==
          FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    geometry_listener(c, &first);
    bool success = false;
    unsigned before = 0, after = 0;
    for (long boundary = 0; boundary < 4096 && !success; ++boundary)
    {
        scroll_number(c, events.field, "_width", 40);
        events.count = 0;
        FVM_Value value;
        fvm_value_number(&value, 60);
        allowance = argc > 1 ? -1 : boundary;
        FVM_Result result = fvm_object_set(c, events.field, "_width", &value);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        success = result == FVM_OK;
        double width = counted_metric(c, events.field, "_width");
        FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
        if (!success)
        {
            if (width == 40)
            {
                ++before;
                if (!(!events.count && info.maxhscroll == 12))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                ++after;
                if (!(width == 60 && !info.maxhscroll && events.count < 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
        }
        else
        {
            if (!(width == 60 && !info.maxhscroll && events.count == 2))
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
    }
    if (!(success && (argc > 1 || (before && after))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#else
    (void)argc;
#endif
    return 0;
}
