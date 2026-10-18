/* Watch callback projection preserves an existing watcher at every failed allocation. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "watch_boxing_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; version += 2)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned mode = 0; mode < 3; ++mode)
    {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1000 && !success; ++boundary)
        {
            WatchBoxingFixture f; watch_box_init(rt, f, version, type, mode);
            FVM_Context *c = f.context;
            FVM_Value args[3], out, input; fvm_value_string(args, "watched");
            args[1] = class_value(f.handler); fvm_value_number(args + 2, 1);
            assert(fvm_object_call(c, f.target, "watch", 3, args, &out) == FVM_OK && out.boolean);
            args[1] = f.input; fvm_value_number(args + 2, 7);
            allowance = boundary;
            FVM_Result status = fvm_object_call(c, f.target, "watch", 3, args, &out);
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_constructor);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert((out.type == FVM_VALUE_UNDEFINED || (out.type == FVM_VALUE_BOOLEAN && !out.boolean)) && fvm_gc_collect(c, 0) == FVM_OK);
                fvm_value_number(&input, 5);
                assert(fvm_object_set(c, f.target, "watched", &input) == FVM_OK && fvm_object_get(c, f.target, "watched", &out) == FVM_OK && out.number == 6);
                assert(fvm_object_call(c, f.target, "watch", 3, args, &out) == FVM_OK);
            }
            else { assert(status == FVM_OK); success = true; }
            assert(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == !mode && f.calls);
            fvm_value_number(&input, 5);
            assert(fvm_object_set(c, f.target, "watched", &input) == FVM_OK && fvm_object_get(c, f.target, "watched", &out) == FVM_OK && out.number == (mode ? 6 : 12));
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
