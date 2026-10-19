/* Full input shares capture/record/method work; completed callbacks survive later exhaustion. */
#include "clip_input_methods_fixture.hpp"
/* Observe bounded partial delivery without asking native callbacks to consume artificial work. */
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    unsigned partial = 0, failed = 0, complete = 0;
    for (unsigned limit = 1; limit <= 300; ++limit)
    {
        FVM_Context *c = clip_context(runtime);
        FVM_Object *root = method_root(c), *actor = get(c, root, "actor").object;
        assert(fvm_object_define_method_rooted(c, actor, "onMouseMove", method_marker, 0, 0) == FVM_OK);
        FVM_Object *second = input_duplicate(c, actor, "second", 3);
        assert(fvm_object_define_method_rooted(c, second, "onMouseMove", method_marker, 0, 0) == FVM_OK);
        runtime->config.instruction_limit = limit;
        FVM_Result result = fvm_movieclip_dispatch_input(c, root, 8);
        assert(result == FVM_OK || result == FVM_ERROR);
        FVM_Value first = get(c, actor, "seen"), last = get(c, second, "seen");
        if (result == FVM_OK) ++complete;
        else
        {
            ++failed;
            if (first.type == FVM_VALUE_NUMBER && first.number == 84 &&
                (last.type != FVM_VALUE_NUMBER || last.number != 84)) ++partial;
        }
        input_clean(c);
        runtime->config.instruction_limit = 0;
        assert(fvm_movieclip_dispatch_input(c, root, 8) == FVM_OK);
        number(c, actor, "seen", 84);
        number(c, second, "seen", 84);
        input_clean(c);
        fvm_context_destroy(c);
    }
    assert(failed && partial && complete);
    fvm_runtime_destroy(runtime);
}
