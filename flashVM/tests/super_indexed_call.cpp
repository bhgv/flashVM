/* Indexed parent methods retain defining owner for nested super and noncallable shadowing. */
#define FVM_SUPER_WRITE_FIXTURE_ONLY
#include "super_property_write.cpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"0", "5", "2147483647", "+5", "n\0m"}; const size_t sizes[] = {1,1,10,2,3};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array)
    for (unsigned key = 0; key < 5; ++key)
    for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *middle = 0, *far = 0, *method = 0;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, array ? "Array" : "Object", 0, 0, &middle) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        FVM_Value value = class_value(version_function(c, 91)), out;
        assert(fvm_object_set_bytes(c, far, keys[key], sizes[key], &value) == FVM_OK);
        Program body; body.variable("this"); body.text("calls"); body.variable("this"); body.text("calls"); body.op(0x4e); body.number(1); body.op(0x0a); body.op(0x4f);
        body.number(0); body.variable("super"); body.variable("superCallKey"); body.op(0x52); body.finish();
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &method) == FVM_OK);
        value = class_value(method); if (kind == 2) fvm_value_number(&value, 17);
        assert(fvm_object_set_bytes(c, middle, keys[key], sizes[key], &value) == FVM_OK);
        if (kind == 1) middle->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4];
        /* Stored numeric values, including noncallables, must suppress __resolve. */
        Program resolve; resolve.variable("this"); resolve.text("resolved"); resolve.number(1); resolve.op(0x4f); resolve.number(91); resolve.finish();
        FVM_Object *resolver = 0; assert(fvm_function_create_avm1(c, resolve.bytes.begin(), resolve.bytes.size(), 0, 0, 0, &resolver) == FVM_OK);
        value = class_value(resolver); assert(fvm_object_set(c, middle, "__resolve", &value) == FVM_OK);
        fvm_value_number(&value, 777); assert(fvm_object_set_bytes(c, receiver, keys[key], sizes[key], &value) == FVM_OK);
        fvm_value_number(&value, 0); assert(fvm_object_set(c, receiver, "calls", &value) == FVM_OK);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, middle, far) == FVM_OK && fvm_object_set_prototype(c, receiver, middle) == FVM_OK);
        FVM_Object *facade = super_write_escape(c, receiver); value = class_value(facade); assert(fvm_set_global(c, "superCallTarget", &value) == FVM_OK);
        fvm_value_string_bytes(&value, keys[key], sizes[key]); assert(fvm_set_global(c, "superCallKey", &value) == FVM_OK);
        if (!route) assert(fvm_object_call_bytes(c, facade, keys[key], sizes[key], 0, 0, &out) == (kind == 2 ? FVM_UNSUPPORTED : FVM_OK));
        else { Program code; code.number(0); code.variable("superCallTarget"); code.variable("superCallKey"); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        if (kind != 2) assert(out.type == FVM_VALUE_NUMBER && out.number == 91);
        else if (route) assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get(c, receiver, "calls", &out) == FVM_OK && out.number == (kind == 0 ? 1 : 0));
        int resolved = 1; assert(fvm_object_has_own(c, receiver, "resolved", &resolved) == FVM_OK && !resolved);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
