/* SWF5 this keyword folding follows the innermost scope class, not activation presence. */
#define FVM_THIS_ASSIGNMENT_FIXTURE_ONLY
#include "this_cell_assignment.cpp"
static void with_case_program(Program &out, const char *key, bool local_child)
{
    out.text("this"); out.number(17); out.op(0x1d);
    Program body;
    if (local_child) { body.number(0); body.text("child"); body.op(0x3d); }
    else body.variable(key);
    body.op(0x3e);
    out.variable("owner"); out.op(0x94); out.op(2); out.op(0); out.op((unsigned char)body.bytes.size()); out.op((unsigned char)(body.bytes.size() >> 8));
    for (size_t i = 0; i < body.bytes.size(); ++i) out.op(body.bytes[i]);
    out.number(99); out.finish();
}
static FVM_Value with_case_invoke(FVM_Context *c, FVM_Object *fn, const Program &body, unsigned mode, FVM_Result expected = FVM_OK)
{
    FVM_JitCode *native = 0; FVM_Value out;
    assert(fvm_gc_pin(c, fn) == FVM_OK && fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    if (mode == 1) { assert(fvm_jit_compile(body.bytes.begin(), body.bytes.size(), &native) == FVM_OK && fvm_function_set_body(c, fn, fvm_jit_body, native, 0) == FVM_OK); }
    unsigned long entries = c->execution_mode.stats.native_entries;
    assert(fvm_function_invoke(c, fn, c->global, 0, 0, &out) == expected);
    if (mode) assert(c->execution_mode.stats.native_entries > entries);
    if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
    assert(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK);
    if (native) assert(fvm_function_set_body(c, fn, 0, 0, 0) == FVM_OK && fvm_jit_destroy(native) == FVM_OK);
    assert(fvm_gc_unpin(c, fn) == FVM_OK && !c->script_this_cell && !c->script_this && !c->gc_frames && !c->script_depth);
    return out;
}
#ifndef FVM_WITH_THIS_CASE_FIXTURE_ONLY
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"this", "THIS", "ThIs", "TH\xc4\xb0S", "thisSuffix"};
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned local_child = 0; local_child < 2; ++local_child)
    for (unsigned key = 0; key < 5; ++key)
    for (unsigned transport = 0; transport < 2; ++transport)
    for (unsigned mode = 0; mode < 3; ++mode) {
        if ((version && version < 4) || (local_child && version < 5) || (mode && !fvm_jit_available())) continue;
        FVM_Context *c = 0, *source = 0; assert(fvm_context_create(rt, &c) == FVM_OK); if (version) version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *owner = 0, *bundle = 0; assert(fvm_construct(c, "Object", 0, 0, &owner) == FVM_OK && fvm_object_create(c, &bundle) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 91); assert(fvm_object_set(c, owner, keys[key], &value) == FVM_OK);
        value = class_value(owner); assert(fvm_object_set(c, bundle, "owner", &value) == FVM_OK);
        if (local_child) {
            FVM_Object *scope = 0, *child = 0; assert(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK);
            Program body; body.variable(keys[key]); body.finish(); assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, scope, &child) == FVM_OK);
            value = class_value(child); assert(fvm_object_set(c, bundle, "child", &value) == FVM_OK);
        }
        Program body; with_case_program(body, keys[key], local_child != 0); FVM_Object *function = 0;
        assert(fvm_function_create_avm1(c, body.bytes.begin(), body.bytes.size(), 0, 0, 0, &function) == FVM_OK); value = class_value(function); assert(fvm_object_set(c, bundle, "function", &value) == FVM_OK);
        if (transport) {
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK); if (version) version_load(other, version); assert(fvm_standard_install(other) == FVM_OK);
            value = class_value(bundle); unsigned char *wire = 0; size_t size = 0; assert(fvm_value_serialize(c, &value, &wire, &size) == FVM_OK && fvm_value_deserialize(other, wire, size, &value) == FVM_OK); fvm_serialized_free(wire); bundle = value.object; source = c; c = other;
        }
        value = class_value(bundle); assert(fvm_set_global(c, "caseBundle", &value) == FVM_OK && fvm_object_get(c, bundle, "owner", &value) == FVM_OK && fvm_set_global(c, "owner", &value) == FVM_OK);
        if (local_child) assert(fvm_object_get(c, bundle, "child", &value) == FVM_OK && fvm_set_global(c, "child", &value) == FVM_OK);
        assert(fvm_object_get(c, bundle, "function", &value) == FVM_OK); value = with_case_invoke(c, value.object, body, mode);
        bool keyword = local_child ? key == 0 || (version == 6 && key < 4) : version >= 5 && (key == 0 || (version <= 6 && key < 4));
        if (keyword && local_child) assert(value.type == FVM_VALUE_OBJECT && value.object == c->global);
        else assert(value.type == FVM_VALUE_NUMBER && value.number == (keyword ? 17 : 91));
        assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c); if (source) fvm_context_destroy(source);
    }
    fvm_runtime_destroy(rt);
}
#endif
