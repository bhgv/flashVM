/* __resolve and primitive projection skip version-hidden local callbacks without invoking getters. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned kind = 0; kind < 3; ++kind)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *child = 0, *parent = 0; assert(fvm_construct(c, "Object", 0, 0, &child) == FVM_OK && fvm_construct(c, "Object", 0, 0, &parent) == FVM_OK && fvm_object_set_prototype(c, child, parent) == FVM_OK);
        const char *name = kind == 2 ? "valueOf" : "__resolve"; FVM_Value value = class_value(version_function(c, 91)), out; assert(fvm_object_set(c, parent, name, &value) == FVM_OK); value = class_value(version_function(c, 22)); assert(fvm_object_set(c, child, name, &value) == FVM_OK);
        if (kind == 1) { FVM_Value args[3]; fvm_value_string(args, name); args[1] = class_value(version_function(c, 55)); args[2] = class_value(version_function(c, 0)); assert(fvm_object_call(c, child, "addProperty", 3, args, &out) == FVM_OK && out.boolean); }
        version_flags(c, child, name, 0x1000, 0); value = class_value(child); assert(fvm_set_global(c, "versionTarget", &value) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        if (!route) { if (kind == 2) assert(fvm_exec_to_number(c, 0, &value, &out) == FVM_OK); else assert(fvm_object_get(c, child, "missing", &out) == FVM_OK); }
        else { Program code; code.variable("versionTarget"); if (kind == 2) code.op(0x4a); else { code.text("missing"); code.op(0x4e); } code.finish(); out = boxing_run(c, code, route - 1, FVM_OK); }
        assert(out.type == FVM_VALUE_NUMBER && out.number == (version < 8 ? 91 : 22));
        if (!route) {
            FVM_Context *other = 0; unsigned char *data = 0; size_t bytes = 0; assert(fvm_context_create(rt, &other) == FVM_OK); version_load(other, version);
            assert(fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &value) == FVM_OK); fvm_serialized_free(data);
            assert(fvm_gc_pin(other, value.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK); if (kind == 2) assert(fvm_exec_to_number(other, 0, &value, &out) == FVM_OK); else assert(fvm_object_get(other, value.object, "missing", &out) == FVM_OK);
            assert(out.type == FVM_VALUE_NUMBER && out.number == (version < 8 ? 91 : 22)); fvm_context_destroy(other);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
