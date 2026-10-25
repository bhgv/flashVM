/* Allocation failures unwind method/owner/work scopes while committed timeline state remains usable. */
#define main tick_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "movieclip_tick_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool success = false;
    unsigned failures = 0;
    for (long boundary = 0; boundary < 512 && !success; ++boundary)
    {
        FVM_Context *c = tick_context(rt);
        FVM_Object *root = tick_root(c, 2);
        TickProbe p;
        for (unsigned i = 0; i < 3; ++i)
        {
            FVM_Object *ch = tick_child(c, root, "child", i);
            if (!(fvm_object_define_method_rooted(c, ch, "onEnterFrame", tick_notify, &p, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_object_define_method_rooted(c, root, "onEnterFrame", tick_notify, &p, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = boundary;
        FVM_Result r = fvm_movieclip_advance(c, root, 1);
        allowance = -1;
        tick_clean(c);
        if (r == FVM_OK)
        {
            success = true;
            if (!(p.calls == 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
            unsigned before = p.calls;
            if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.calls == before + 4))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
}
