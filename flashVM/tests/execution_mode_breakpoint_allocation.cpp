#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "execution_services_allocation.cpp"

#undef main
#include "../src/jit/mode_internal.hpp"


static FVM_Result stop(FVM_Context *, const FVM_DebugBreakpoint *, void *)
{
    return FVM_ERROR;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt;
    FVM_Context *c;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool success = false;
    unsigned failures = 0;


    fvm_detail::string name("breakpoint-allocation-owned-function-name");
    for (long boundary = 0; boundary < 100 && !success; ++boundary)
    {
        allowance = boundary;
        FVM_Result r = fvm_debug_set_breakpoint(c, name.c_str(), 0, 1);
        allowance = -1;
        if (r == FVM_OK)
        {
            success = true;
        }
        else
        {
            if (!(r == FVM_OUT_OF_MEMORY && !fvm_debug_has_breakpoints(c, name)))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            ++failures;
        }
    }
    if (!(success && failures))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Removing a missing point must not create a body exclusion. */
    if (!(fvm_debug_set_breakpoint(c, name.c_str(), 0, 0) == FVM_OK && !fvm_debug_has_breakpoints(c, name)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_set_breakpoint(c, "", 0, 1) == FVM_OK && fvm_debug_set_breakpoint_handler(c, stop, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    const unsigned char body[] = {0};
    if (!(fvm_avm1_execute(c, body, sizeof(body)) == FVM_ERROR && !c->execution_mode.breakpoint_busy &&
          !c->avm1_execution_depth))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_debug_set_breakpoint_handler(c, 0, 0) == FVM_OK && fvm_avm1_execute(c, body, sizeof(body)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
