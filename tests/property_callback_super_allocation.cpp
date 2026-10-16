/* Getter/setter OOM restores fixed-depth call state and permits retry of partial callback effects. */
#define main property_depth_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_PROPERTY_CALLBACK_SUPER_FIXTURE_ONLY
#include "property_callback_super.cpp"
#include "../src/core/call_scope_constants.h"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char key[] = "n\0m";
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned setter = 0; setter < 2; ++setter) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0, *write = 0; assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            Program body; body.variable("this"); body.text("savedSuper"); body.variable("super"); body.op(0x4f); body.number(1); body.finish();
            assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &write) == FVM_OK);
            callback_super_property(c, receiver, key, sizeof(key) - 1, adapter_super_function(c, 0), write);
            assert(fvm_object_set_prototype(c, receiver, 0) == FVM_OK);
            FVM_Value value, out; fvm_value_string_bytes(&value, "r\0s", 3); allowance = boundary;
            const FVM_Result status = setter ? fvm_object_set_bytes(c, receiver, key, sizeof(key) - 1, &value) : fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out); allowance = -1;
            assert(c->calling_super_depth == FVM_CALL_SCOPE_INFER_SUPER_DEPTH && !c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_owner && !c->calling_setter_owner && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK);
                assert((setter ? fvm_object_set_bytes(c, receiver, key, sizeof(key) - 1, &value) : fvm_object_get_bytes(c, receiver, key, sizeof(key) - 1, &out)) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            if (setter) {
                assert(fvm_object_get(c, receiver, "savedSuper", &out) == FVM_OK);
                FVM_Value data; assert(fvm_base_accessor_data(receiver->accessors.find(fvm_detail::string(key, sizeof(key) - 1))->second, data));
                assert(data.type == FVM_VALUE_STRING && fvm_value_string_size(&data) == 3 && !std::memcmp(data.string, "r\0s", 3));
            }
            assert(out.type == FVM_VALUE_OBJECT && out.object->resource_kind == FVM_RESOURCE_SUPER && out.object->resource_result.object->properties["depth"].number == 1);
            assert(fvm_gc_pin(c, out.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
