/* Typed length coercion preserves collecting input roots, callback mutations and errors. */
#define FVM_TYPED_LENGTH_FIXTURE_ONLY
#include "array_typed_length.cpp"
static FVM_Object *typed_root_array, *typed_root_input, *typed_root_meta;
static unsigned typed_root_calls, typed_root_fault;
static bool typed_root_alive(FVM_Context *c, FVM_Object *o)
{ for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == o) return true; return false; }
static FVM_Result typed_root_convert(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++typed_root_calls;
    int removed = 0; assert(fvm_object_delete(c, c->global, "typedRootInput", &removed) == FVM_OK && removed);
    FVM_Value length; fvm_value_number(&length, 2); assert(fvm_object_set(c, typed_root_array, "length", &length) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    assert(typed_root_alive(c, typed_root_array) && typed_root_alive(c, typed_root_input) && typed_root_alive(c, typed_root_meta));
    fvm_value_number(out, 1);
    return typed_root_fault == 1 ? FVM_ERROR : typed_root_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned direct = 0; direct < 2; ++direct)
    for (typed_root_fault = 0; typed_root_fault < 3; ++typed_root_fault) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Value initial[3]; for (unsigned j = 0; j < 3; ++j) fvm_value_number(initial + j, 10 + j);
        assert(fvm_construct(c, "Array", 3, initial, &typed_root_array) == FVM_OK && fvm_gc_pin(c, typed_root_array) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &typed_root_input) == FVM_OK && fvm_object_create(c, &typed_root_meta) == FVM_OK);
        FVM_Value input = class_value(typed_root_input), edge = class_value(typed_root_meta), out;
        assert(fvm_object_set(c, typed_root_input, "edge", &edge) == FVM_OK && fvm_object_set(c, typed_root_meta, "cycle", &input) == FVM_OK && fvm_set_global(c, "typedRootInput", &input) == FVM_OK);
        FVM_Object *fn = version_function(c, 1); assert(fvm_function_set_body(c, fn, typed_root_convert, 0, 0) == FVM_OK);
        edge = class_value(fn); assert(fvm_object_set(c, typed_root_input, "valueOf", &edge) == FVM_OK);
        typed_root_calls = 0;
        const FVM_Result expected = !typed_root_fault ? FVM_OK : typed_root_fault == 1 ? FVM_ERROR : FVM_EXCEPTION;
        FVM_Result result;
        if (direct) { FVM_AccessorEntry entry = typed_root_array->accessors.find("length")->second; result = entry.setter(c, typed_root_array, 1, &input, &out, entry.userdata); }
        else result = fvm_object_set(c, typed_root_array, "length", &input);
        assert(result == expected && typed_root_calls == 1);
        if (c->exception_pending) assert(fvm_exception_clear(c) == FVM_OK);
        assert(fvm_object_get(c, typed_root_array, "length", &out) == FVM_OK);
        if (typed_root_fault) assert(out.type == FVM_VALUE_NUMBER && out.number == 2);
        else assert(out.type == FVM_VALUE_OBJECT && out.object == typed_root_input);
        int own = 0; assert(fvm_object_has_own(c, typed_root_array, "1", &own) == FVM_OK && !!own == !!typed_root_fault);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(typed_root_alive(c, typed_root_input) == !typed_root_fault && typed_root_alive(c, typed_root_meta) == !typed_root_fault);
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
