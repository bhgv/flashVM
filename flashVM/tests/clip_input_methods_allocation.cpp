/* Capture, retained-code and optional-lookup allocation failures restore shared guards. */
#define main input_method_allocation_fixture_main
#include "swf_sprite_allocation.cpp"
#undef main
#include "clip_input_methods_fixture.hpp"
/* Sweep every allocation until the combined tree and method route succeeds. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    bool complete = false;
    unsigned failures = 0;
    for (long n = 0; n < 4096 && !complete; ++n)
    {
        FVM_Context *c = clip_context(runtime);
        FVM_Object *root = method_root(c), *actor = get(c, root, "actor").object;
        assert(fvm_object_define_method_rooted(c, actor, "onMouseMove", method_marker, 0, 0) == FVM_OK);
        FVM_Object *second = input_duplicate(c, actor, "second", 3);
        assert(fvm_object_define_method_rooted(c, second, "onMouseMove", method_marker, 0, 0) == FVM_OK);
        allowance = n;
        FVM_Result result = fvm_movieclip_dispatch_input(c, root, 8);
        allowance = -1;
        assert(result == FVM_OK || result == FVM_OUT_OF_MEMORY);
        input_clean(c);
        if (result == FVM_OK) complete = true;
        else
        {
            ++failures;
            assert(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK);
        }
        number(c, actor, "seen", 84);
        number(c, get(c, root, "second").object, "seen", 84);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    assert(complete && failures);
    fvm_runtime_destroy(runtime);
}
