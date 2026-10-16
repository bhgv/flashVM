/* Allocation failures during super setter dispatch unwind roots and preserve inert facade storage. */
#define main super_allocation_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include "super_property_write.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char key[] = "target\0field";
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array) {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 1024 && !success; ++boundary) {
            FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
            FVM_Object *receiver = 0, *parent = 0, *set = 0;
            assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
            assert(fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
            Program body; body.variable("this"); body.text("called"); body.variable("input"); body.op(0x4f); body.number(1); body.finish(); const char *parameters[] = {"input"};
            assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 1, parameters, 0, &set) == FVM_OK);
            super_write_property(c, parent, key, sizeof(key) - 1, version_function(c, 42), set);
            assert(fvm_object_set_prototype(c, parent, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
            FVM_Object *facade = super_write_escape(c, receiver); assert(fvm_gc_pin(c, facade) == FVM_OK);
            FVM_Value input, out; fvm_value_string_bytes(&input, "r\0s", 3);
            allowance = boundary; const FVM_Result status = fvm_object_set_bytes(c, facade, key, sizeof(key) - 1, &input); allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner && !c->calling_owner && !c->exception_pending && c->watch_running.empty());
            if (status == FVM_OUT_OF_MEMORY) { failed = true; assert(fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_object_set_bytes(c, facade, key, sizeof(key) - 1, &input) == FVM_OK); }
            else { assert(status == FVM_OK); success = true; }
            assert(fvm_gc_collect(c, 0) == FVM_OK);
            assert(fvm_object_get(c, receiver, "called", &out) == FVM_OK && out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "r\0s", 3));
            int own = 1; assert(fvm_object_has_own_bytes(c, receiver, key, sizeof(key) - 1, &own) == FVM_OK && !own);
            assert(facade->properties.empty() && facade->indexed.empty());
            if (array) assert(fvm_object_get(c, receiver, "length", &out) == FVM_OK && out.number == 0);
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
