/* Primitive projection and bare fallback in isPrototypeOf unwind each allocation failure. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "primitive_boxing_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 6; version <= 8; version += 2)
    for (unsigned type = 0; type < 3; ++type)
    for (unsigned kind = 0; kind < 3; ++kind)
    {
        bool success = false, failed = false;
        for (long boundary = 0; boundary < 1000 && !success; ++boundary)
        {
            BoxingFixture f; boxing_init(rt, f, version, type, kind == 2 ? 5 : kind);
            FVM_Context *c = f.context;
            FVM_Value proto; assert(fvm_object_get(c, f.factory, "prototype", &proto) == FVM_OK);
            FVM_Object *receiver = kind == 2 ? proto.object : f.prototype;
            FVM_Value out; fvm_value_number(&out, 91);
            allowance = boundary;
            FVM_Result status = fvm_object_call(c, receiver, "isPrototypeOf", 1, &f.input, &out);
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_constructor);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(out.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(c, 0) == FVM_OK);
                assert(fvm_object_call(c, receiver, "isPrototypeOf", 1, &f.input, &out) == FVM_OK);
            }
            else { assert(status == FVM_OK); success = true; }
            assert(out.type == FVM_VALUE_BOOLEAN && !!out.boolean == (kind != 1));
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
