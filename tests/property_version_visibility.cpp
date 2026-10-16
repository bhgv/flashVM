/* Version flags affect ordinary reads/calls, but not raw membership, keys or Array data. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const unsigned bits[] = {8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768,65535};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned flag = 0; flag < sizeof(bits)/sizeof(bits[0]); ++flag)
    for (unsigned kind = 0; kind < 4; ++kind)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *target = 0, *parent = 0; assert(fvm_construct(c, kind == 3 ? "Array" : "Object", 0, 0, &target) == FVM_OK && fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK);
        const char *name = kind == 3 ? "0" : "field";
        FVM_Value value, original, out; fvm_value_number(&value, 11); assert(fvm_object_set(c, parent, name, &value) == FVM_OK);
        if (kind == 0 || kind == 3) { fvm_value_number(&value, 22); assert(fvm_object_set(c, target, name, &value) == FVM_OK); }
        if (kind == 1) { FVM_Value args[3]; fvm_value_string(args, name); args[1] = class_value(version_function(c, 22)); args[2] = class_value(version_function(c, 0)); assert(fvm_object_call(c, target, "addProperty", 3, args, &out) == FVM_OK && out.boolean); }
        if (kind == 2) { FVM_Value klass, proto; assert(fvm_get_global(c, "Object", &klass) == FVM_OK && fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); target->methods[name] = proto.object->methods.find("valueOf")->second; value = class_value(version_function(c, 91)); assert(fvm_object_set(c, parent, name, &value) == FVM_OK); }
        assert(fvm_object_get(c, target, name, &original) == FVM_OK && fvm_object_set_prototype(c, parent, target->prototype) == FVM_OK && fvm_object_set_prototype(c, target, parent) == FVM_OK);
        value = class_value(target); assert(fvm_set_global(c, "versionTarget", &value) == FVM_OK);
        version_flags(c, target, name, bits[flag], 0, route);
        const bool visible = !(bits[flag] & version_masks[version-4]);
        if (!route) assert(fvm_object_get(c, target, name, &out) == FVM_OK);
        else { class_mode = route - 1; Program code; code.variable("versionTarget"); code.text(name); code.op(0x4e); code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        if (kind == 2) { assert(out.type == FVM_VALUE_OBJECT); if (visible) assert(out.object == original.object); assert(fvm_object_call(c, target, name, 0, 0, &out) == FVM_OK); if (visible) assert(out.type == FVM_VALUE_OBJECT && out.object == target); else assert(out.type == FVM_VALUE_NUMBER && out.number == 91); }
        else assert(out.type == FVM_VALUE_NUMBER && out.number == (visible ? 22 : 11));
        int own = 0; assert(fvm_object_has_own(c, target, name, &own) == FVM_OK && own); version_enumerable(c, target, name, !(bits[flag] & 1));
        fvm_value_string(&value, name); assert(fvm_object_call(c, target, "isPropertyEnumerable", 1, &value, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !!out.boolean == !(bits[flag] & 1));
        if (kind == 3) { assert(fvm_object_call(c, target, "slice", 0, 0, &out) == FVM_OK && fvm_table_get_index(c, out.object, 0, &out) == FVM_OK && out.number == 22); }
        unsigned actual = 0; assert(fvm_object_property_flags(c, target, name, &actual) == FVM_OK && actual == bits[flag]);
        /* Hidden fields remain reachable for attribute clearing, without ordinary lookup. */
        version_flags(c, target, name, 0, 65535, route); assert(fvm_object_get(c, target, name, &out) == FVM_OK && out.type == original.type); if (out.type == FVM_VALUE_NUMBER) assert(out.number == 22);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
