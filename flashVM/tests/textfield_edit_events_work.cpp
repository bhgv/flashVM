/* One work budget covers preparation and callback delivery for each edit operation. */
#include <stdio.h>
#include <stdlib.h>
#include "textfield_edit_events_fixture.hpp"


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
        GeometryObserver own = {&events, 1}, listener = {&events, 2};
        if (!(fvm_object_define_method_rooted(c, events.field, "onScroller", geometry_observe, &own, events.field) ==
              FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        geometry_listener(c, &listener);
        FVM_Object *format = edit_format(c, 20);
        FVM_Value value, args[3], style, out;
        if (!(fvm_value_string(&value, "A") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args, 0);
        fvm_value_number(args + 1, 8);
        args[2] = value;
        fvm_value_undefined(&style);
        style.type = FVM_VALUE_OBJECT;
        style.object = format;
        unsigned before = 0, after = 0, successes = 0;
        for (unsigned limit = 1; limit <= 64; ++limit)
        {
            if (!(edit_text(c, events.field, "AV\nAV\nAV", 8) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            events.count = 0;
            rt->config.instruction_limit = limit;
            FVM_Result result = mode == 0   ? fvm_object_set(c, events.field, "text", &value)
                                : mode == 1 ? fvm_object_call(c, events.field, "replaceText", 3, args, &out)
                                            : fvm_object_call(c, events.field, "setTextFormat", 1, &style, &out);
            rt->config.instruction_limit = 0;
            if (!(result == FVM_OK || result == FVM_ERROR))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            bool success = result == FVM_OK;
            if (success)
            {
                ++successes;
            }
            double width = counted_metric(c, events.field, "textWidth");
            if (width == 48)
            {
                if (!(!success && !events.count))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                ++before;
                counted_text(c, events.field, "AV\nAV\nAV", 8);
            }
            else
            {
                if (!(width == 24 && events.count <= 2))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                counted_text(c, events.field, mode == 2 ? "AV\nAV\nAV" : "A", mode == 2 ? 8 : 1);
                if (success)
                {
                    if (!(events.count == 2))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    ++after;
                    if (!(events.count < 2))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
            }
            if (!(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(successes && before && after))
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
