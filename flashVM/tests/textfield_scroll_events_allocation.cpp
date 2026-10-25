/* Registration rollback and notification allocation failures distinguish precommit from committed state. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "textfield_scroll_events_fixture.hpp"


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
    ScrollEvents events;
    events.field = scroll_field(c);
    events.count = 0;
    events.top = 2;
    events.horizontal = 0;
    ScrollObserver own = {&events, 10}, first = {&events, 20}, second = {&events, 30};
    FVM_Object *a = scroll_observer(c, &first), *b = scroll_observer(c, &second);
    if (!(fvm_gc_pin(c, a) == FVM_OK && fvm_gc_pin(c, b) == FVM_OK &&
          fvm_object_define_method_rooted(c, events.field, "onScroller", scroll_observe, &own, events.field) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (argc == 1)
    {
        bool success = false;
        unsigned failed = 0;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary)
        {
            FVM_Value arg, out;
            fvm_value_undefined(&arg);
            arg.type = FVM_VALUE_OBJECT;
            arg.object = a;
            allowance = boundary;
            FVM_Result result = fvm_object_call(c, events.field, "addListener", 1, &arg, &out);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            FVM_Object *registry = 0;
            if (!success)
            {
                ++failed;
                if (!(fvm_textfield_listener_registry(c, events.field, &registry) == FVM_NOT_FOUND))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            else
            {
                if (!(out.boolean))
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
        if (!(success && failed))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(scroll_listen(c, events.field, a)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(scroll_listen(c, events.field, b)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (argc == 1)
    {
        bool success = false;
        unsigned before = 0, after = 0;
        for (long boundary = 0; boundary < 4096 && !success; ++boundary)
        {
            events.top = 1;
            scroll_number(c, events.field, "scroll", 1);
            events.top = 2;
            events.count = 0;
            FVM_Value value;
            fvm_value_number(&value, 2);
            allowance = boundary;
            FVM_Result result = fvm_object_set(c, events.field, "scroll", &value);
            allowance = -1;
            if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            success = result == FVM_OK;
            FVM_TextFieldScrollInfo info = scroll_info(c, events.field);
            if (!(info.scroll == 1 || info.scroll == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!success)
            {
                if (info.scroll == 1)
                {
                    ++before;
                    if (!(!events.count))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    ++after;
                }
            }
            else
            {
                if (!(info.scroll == 2 && events.count == 3))
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
        if (!(success && before && after))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        scroll_number(c, events.field, "scroll", 2);
        if (!(events.count == 3))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
#else
    (void)argc;
#endif
    return 0;
}
