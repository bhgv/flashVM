/* SetVariable this mutates the activation cell, retaining arbitrary typed values. */
#define FVM_THIS_PATH_FIXTURE_ONLY
#include "anonymous_this_paths.cpp"
static FVM_Object *assigned_this_function(FVM_Context *c, const char *name, unsigned followup)
{
    Program body; body.text(name); body.variable("replacement"); body.op(0x1d);
    if (!followup) body.variable("this");
    else if (followup == 1) body.variable("this.marker");
    else { body.number(0); body.variable("inheritedFunction"); body.text(""); body.op(0x52); }
    body.finish(); FVM_Object *fn = 0;
    assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &fn) == FVM_OK); return fn;
}
#ifndef FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned type = 0; type < 7; ++type)
    for (unsigned route = 0; route < 4; ++route)
    for (unsigned followup = 0; followup < 3; ++followup) {
        if ((route >= 2 && !fvm_jit_available()) || (followup == 2 && version < 7)) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value replacement, out; fvm_value_undefined(&replacement);
        if (type == 1) replacement.type = FVM_VALUE_NULL;
        if (type == 2) fvm_value_boolean(&replacement, 1);
        if (type == 3) fvm_value_number(&replacement, -17.5);
        if (type == 4) fvm_value_string_bytes(&replacement, "r\0s", 3);
        if (type == 5 || type == 6) { FVM_Object *o = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK); replacement = class_value(o); fvm_value_number(&out, 81); assert(fvm_object_set(c, o, "marker", &out) == FVM_OK); if (type == 6) { fvm_value_number(&replacement, 19); replacement.metatable = o; } }
        assert(fvm_set_global(c, "replacement", &replacement) == FVM_OK);
        if (followup == 2) { out = class_value(undefined_this_function(c, 5)); assert(fvm_set_global(c, "inheritedFunction", &out) == FVM_OK); }
        FVM_Object *fn = assigned_this_function(c, "this", followup);
        out = this_path_invoke(c, fn, false, route);
        if (version == 4) { if (!followup) boxing_equal(out, replacement); }
        else if (followup != 1) boxing_equal(out, replacement);
        else if (type == 5) assert(out.type == FVM_VALUE_NUMBER && out.number == 81);
        else assert(out.type == FVM_VALUE_UNDEFINED);
        if (version >= 5 && followup != 1) assert(out.metatable == replacement.metatable);
        assert(!c->script_this_cell && !c->script_this && !c->script_this_undefined && !c->gc_frames && !c->script_depth);
        fvm_context_destroy(c);
    }
    for (unsigned version = 5; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 27); assert(fvm_set_global(c, "replacement", &value) == FVM_OK);
        value = this_path_invoke(c, assigned_this_function(c, "THIS", 0), false, 0);
        assert(value.type == FVM_VALUE_OBJECT && value.object == c->global && !c->script_this_cell);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
#endif
