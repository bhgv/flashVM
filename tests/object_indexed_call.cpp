/* C and AVM1 member calls agree on indexed storage, defining owner and resolver suppression. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"0", "5", "2147483647", "4294967295", "+5", "n\0m"}; const size_t sizes[] = {1,1,10,10,2,3};
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned array = 0; array < 2; ++array)
    for (unsigned inherited = 0; inherited < 2; ++inherited)
    for (unsigned key = 0; key < 6; ++key)
    for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned route = 0; route < 5; ++route) {
        if ((version && version < 4) || (route >= 3 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = 0, *parent = 0, *far = 0, *method = 0;
        assert(fvm_construct(c, !inherited && array ? "Array" : "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_construct(c, inherited && array ? "Array" : "Object", 0, 0, &parent) == FVM_OK && fvm_construct(c, "Object", 0, 0, &far) == FVM_OK);
        FVM_Object *owner = inherited ? parent : receiver, *next = inherited ? far : parent;
        FVM_Value value = class_value(version_function(c, 91)), out, name;
        assert(fvm_object_set_bytes(c, next, keys[key], sizes[key], &value) == FVM_OK);
        Program body; body.variable("this"); body.text("calls"); body.variable("this"); body.text("calls"); body.op(0x4e); body.number(1); body.op(0x0a); body.op(0x4f);
        body.number(0); body.variable("super"); body.variable("ordinaryCallKey"); body.op(0x52); body.finish();
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &method) == FVM_OK);
        value = class_value(method); if (kind == 2) fvm_value_number(&value, 17);
        assert(fvm_object_set_bytes(c, owner, keys[key], sizes[key], &value) == FVM_OK);
        if (kind == 1 && version) owner->attributes[fvm_detail::string(keys[key], sizes[key])] = version_masks[version - 4];
        Program resolve; resolve.variable("this"); resolve.text("resolved"); resolve.number(1); resolve.op(0x4f); resolve.number(91); resolve.finish(); FVM_Object *resolver = 0;
        assert(fvm_function_create_avm1(c, resolve.bytes.begin(), resolve.bytes.size(), 0, 0, 0, &resolver) == FVM_OK);
        value = class_value(resolver); assert(fvm_object_set(c, owner, "__resolve", &value) == FVM_OK);
        fvm_value_number(&value, 0); assert(fvm_object_set(c, receiver, "calls", &value) == FVM_OK);
        assert(fvm_object_set_prototype(c, far, receiver->prototype) == FVM_OK && fvm_object_set_prototype(c, parent, far) == FVM_OK && fvm_object_set_prototype(c, receiver, parent) == FVM_OK);
        value = class_value(receiver); assert(fvm_set_global(c, "ordinaryCallTarget", &value) == FVM_OK);
        fvm_value_string_bytes(&name, keys[key], sizes[key]); assert(fvm_set_global(c, "ordinaryCallKey", &name) == FVM_OK);
        if (!route) assert(fvm_object_call_bytes(c, receiver, keys[key], sizes[key], 0, 0, &out) == (kind == 2 ? FVM_UNSUPPORTED : FVM_OK));
        else if (route == 1) assert(fvm_exec_invoke_member(c, receiver, &name, 0, 0, &out) == FVM_OK);
        else { Program code; code.number(0); code.variable("ordinaryCallTarget"); code.variable("ordinaryCallKey"); code.op(0x52); code.finish(); out = boxing_run(c, code, route - 2, !version && kind == 2 ? FVM_UNSUPPORTED : FVM_OK); }
        if (kind != 2) assert(out.type == FVM_VALUE_NUMBER && out.number == 91); else if (route && (version || route == 1)) assert(out.type == FVM_VALUE_UNDEFINED);
        assert(fvm_object_get(c, receiver, "calls", &out) == FVM_OK && out.number == (kind == 0 || (kind == 1 && !version) ? 1 : 0));
        int resolved = 1; assert(fvm_object_has_own(c, receiver, "resolved", &resolved) == FVM_OK && !resolved);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
