/* Root path recognition must preserve ordinary getter callbacks, captures and exceptional cleanup. */
#define FVM_PATH_ROOT_CASE_FIXTURE_ONLY
#include "path_root_case.cpp"
static FVM_Object *root_case_owner, *root_case_shadow;
static const char *root_case_key;
static unsigned root_case_calls, root_case_failure;
static FVM_Result root_case_getter(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++root_case_calls; assert(frame->receiver == root_case_owner && frame->capture == root_case_shadow);
    int removed = 0;
    assert(fvm_object_delete(c, root_case_owner, root_case_key, &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "owner", &removed) == FVM_OK && removed);
    assert(fvm_object_set_prototype(c, root_case_owner, 0) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) alive |= c->objects[i] == root_case_shadow; assert(alive);
    fvm_value_number(out, 77);
    if (root_case_failure == 1) return fvm_exception_raise(c, out);
    if (root_case_failure == 2) return FVM_ERROR;
    *out = class_value(frame->capture); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *keys[] = {"_root", "_ROOT", "_Root", "_rootSuffix"};
    for (unsigned version = 5; version <= 8; ++version)
    for (unsigned key = 0; key < 4; ++key)
    for (unsigned operation = 0; operation < 3; ++operation)
    for (root_case_failure = 0; root_case_failure < 3; ++root_case_failure)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &root_case_owner) == FVM_OK && fvm_construct(c, "Object", 0, 0, &root_case_shadow) == FVM_OK);
        root_case_global(c, root_case_owner); FVM_Value value; fvm_value_number(&value, 91); assert(fvm_object_set(c, root_case_shadow, "marker", &value) == FVM_OK);
        value = class_value(root_case_method(c)); assert(fvm_object_set(c, root_case_shadow, "method", &value) == FVM_OK);
        FVM_Object *getter = version_function(c, 0); assert(fvm_function_set_body(c, getter, root_case_getter, 0, root_case_shadow) == FVM_OK);
        FVM_Value args[3], out; root_case_key = keys[key]; fvm_value_string(args, root_case_key); args[1] = class_value(getter); args[2] = class_value(version_function(c, 0));
        assert(fvm_object_call(c, root_case_owner, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        char path[64]; std::sprintf(path, "%s:%s", root_case_key, operation == 2 ? "method" : "marker");
        const bool keyword = !key; const FVM_Result expected = keyword || !root_case_failure ? FVM_OK : root_case_failure == 1 ? FVM_EXCEPTION : FVM_ERROR;
        root_case_calls = 0;
        if (!route) {
            FVM_Object *scope = 0; value = class_value(root_case_owner); assert(fvm_exec_with_scope(c, 0, &value, &scope) == FVM_OK); fvm_value_string(&value, path);
            if (operation == 1) { FVM_Value input; fvm_value_number(&input, 73); assert(fvm_exec_variable_set(c, scope, c->global, c->global, &value, &input) == expected); fvm_value_number(&out, 73); }
            else if (operation == 2) assert(fvm_exec_call_function(c, scope, c->global, c->global, c->global, 0, &value, 0, 0, &out) == expected);
            else { int handled = 0, present = 0; assert(fvm_exec_variable_path(c, scope, c->global, c->global, &value, &out, &handled, &present) == expected); }
            if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
        } else {
            /* A write's getter removes its own descriptor: return a constant instead of re-reading. */
            Program body, p; if (operation == 1) { body.text(path); body.number(73); body.op(0x1d); body.number(73); }
            else if (operation == 2) { body.number(0); body.text(path); body.op(0x3d); } else body.variable(path);
            body.op(0x3e); p.variable("owner"); p.op(0x94); root_case_word(p, 2); root_case_word(p, body.bytes.size()); root_case_append(p, body); p.number(99); p.finish();
            out = boxing_run(c, p, route - 1, expected);
        }
        assert(root_case_calls == (keyword ? 0U : 1U) && !c->exception_pending);
        if (expected == FVM_OK) assert(out.type == FVM_VALUE_NUMBER && out.number == (operation == 1 ? 73 : keyword ? 17 : 91));
        assert(fvm_gc_pin(c, root_case_shadow) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_object_get(c, root_case_shadow, "marker", &value) == FVM_OK && value.number == (operation == 1 && !keyword && expected == FVM_OK ? 73 : 91));
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_this_cell && c->lexical_scopes.empty()); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
