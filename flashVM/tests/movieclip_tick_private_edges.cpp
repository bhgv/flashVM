/* Native timeline edges reject null/foreign state before dereference or receiver callbacks. */
#include <stdio.h>
#include <stdlib.h>
#include "movieclip_tick_order_fixture.hpp"


/* Snapshot-value validation and initial frame/child-container validation have distinct commit points. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned mode = 0; mode < 6; ++mode)
    {
        FVM_Context *c = tick_context(runtime), *other = tick_context(runtime);
        FVM_Object *root = tick_root(c), *foreign = tick_root(other), *child = ordered_child(c, root, "child", 1);
        TickProbe parent_probe, child_probe;
        if (!(fvm_object_define_method_rooted(c, root, "onEnterFrame", tick_notify, &parent_probe, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_define_method_rooted(c, child, "onEnterFrame", tick_notify, &child_probe, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *data = root->resource_result.object;
        FVM_Value saved, value;
        const char *key = mode < 2 ? "frames" : "children";
        FVM_Object *damaged = mode == 5 ? child : root;
        if (mode < 4)
        {
            saved = data->properties[key];
            value = tick_object(mode % 2 ? foreign : 0);
            data->properties[key] = value;
        }
        else
        {
            saved = damaged->resource_result;
            damaged->resource_result = tick_object(foreign->resource_result.object);
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(parent_probe.calls == (mode == 5 ? 1U : 0U) && !child_probe.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (mode < 4)
        {
            data->properties[key] = saved;
        }
        else
        {
            damaged->resource_result = saved;
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && child_probe.calls == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_context_destroy(other);
    }
    fvm_runtime_destroy(runtime);
}
