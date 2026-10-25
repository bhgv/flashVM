/* Hash-copy, identity validation and sorted workspace allocation failures unwind tick scopes. */
#define main tick_depth_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "movieclip_tick_order_fixture.hpp"


/* Repeat from a fresh graph, checking callback order on every partially completed tick. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool complete = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 2048 && !complete; ++boundary)
    {
        FVM_Context *c = tick_context(runtime);
        FVM_Object *root = tick_root(c);
        FVM_Object *high = ordered_child(c, root, "high", 10), *low = ordered_child(c, root, "low", 1),
                   *middle = ordered_child(c, root, "middle", 5);
        OrderTrace trace = {{root, low, middle, high, 0, 0, 0, 0}, 0, 4};
        for (unsigned i = 0; i < trace.total; ++i)
        {
            if (!(fvm_object_define_method_rooted(c, trace.expected[i], "onEnterFrame", ordered, &trace, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        allowance = boundary;
        FVM_Result result = fvm_movieclip_advance(c, root, 1);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        if (result == FVM_OK)
        {
            complete = true;
            if (!(trace.count == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            ++failures;
        }
        trace.count = 0;
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && trace.count == 4))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(complete && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(runtime);
}
