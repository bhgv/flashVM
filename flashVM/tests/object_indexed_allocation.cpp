/* Indexed member call OOM restores roots; argument/output aliases retain counted strings on retry. */
#define main ordinary_indexed_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0, *parent = 0, *method = 0;
            assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &parent) == FVM_OK);
            Program body; body.variable("input"); body.finish(); const char *parameters[] = {"input"};
            assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &method) == FVM_OK);
            FVM_Value value = class_value(method), out; assert(fvm_table_set_index(c, parent, 5, &value) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
            fvm_value_string_bytes(&value, "r\0s", 3); allowance = boundary;
            const FVM_Result status = fvm_object_call(c, receiver, "5", 1, &value, &value); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && !c->exception_pending);
            if (status == FVM_OUT_OF_MEMORY) {
                failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_value_string_bytes(&value, "r\0s", 3);
                assert(fvm_object_call(c, receiver, "5", 1, &value, &value) == FVM_OK);
            } else { assert(status == FVM_OK); success = true; }
            assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "r\0s", 3));
            assert(fvm_set_global(c, "ordinaryAliasedResult", &value) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_get_global(c, "ordinaryAliasedResult", &out) == FVM_OK && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "r\0s", 3));
            int own = 1; assert(fvm_object_has_own(c, receiver, "5", &own) == FVM_OK && !own);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
