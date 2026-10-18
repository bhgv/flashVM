/* Reloading SWF4–8 preserves explicitly changed builtin flags and object identities. */
#include "builtin_reload_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned source = 4; source <= 8; ++source)
    for (unsigned destination = 4; destination <= 8; ++destination)
    for (unsigned field = 0; field < sizeof(reload_fields) / sizeof(reload_fields[0]); ++field)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        reload_load(c, source); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *target = reload_target(c, field);
        FVM_Value original, value, args[4], out;
        assert(fvm_object_get(c, target, reload_fields[field], &original) == FVM_OK);
        if (original.type == FVM_VALUE_OBJECT) assert(fvm_gc_pin(c, original.object) == FVM_OK);
        args[0] = class_value(target); fvm_value_string(args + 1, reload_fields[field]); fvm_value_number(args + 2, 1); fvm_value_number(args + 3, 7);
        if (!route) assert(fvm_call(c, "ASSetPropFlags", 4, args, &out) == FVM_OK);
        else {
            assert(fvm_set_global(c, "reloadTarget", args) == FVM_OK);
            class_mode = route - 1; Program code; code.number(7); code.number(1); code.text(reload_fields[field]); code.variable("reloadTarget"); code.number(4); code.text("ASSetPropFlags"); code.op(0x3d); code.finish(); out = class_run(c, code);
        }
        reload_flags(c, target, reload_fields[field], 1);
        reload_load(c, destination);
        assert(c->swf_info.version == destination && reload_target(c, field) == target);
        reload_flags(c, target, reload_fields[field], 1);
        assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, target, reload_fields[field], &value) == FVM_OK && value.type == original.type);
        if (value.type == FVM_VALUE_OBJECT) assert(value.object == original.object); else assert(value.number == original.number);
        /* A second reload must not reinterpret user state as untouched host defaults either. */
        reload_load(c, source); reload_flags(c, target, reload_fields[field], 1);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_constructor);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
