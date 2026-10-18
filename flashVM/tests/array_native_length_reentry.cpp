/* Original native length conversion observes descriptor deletion/replacement before publication. */
#define FVM_REASSIGNMENT_FIXTURE_ONLY
#include "array_length_reassignment.cpp"
static FVM_Object *native_reentry_array, *native_reentry_input;
static unsigned native_reentry_mode, native_reentry_calls, native_reentry_fault;
static FVM_Result native_reentry_convert(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++native_reentry_calls;
    if (native_reentry_mode == 1) {
        FVM_Value args[3], result; fvm_value_string(args, "length"); args[1] = class_value(version_function(c, 99)); args[2] = class_value(version_function(c, 88));
        assert(fvm_object_call(c, native_reentry_array, "addProperty", 3, args, &result) == FVM_OK && result.boolean);
    } else {
        assert(fvm_object_set_property_flags(c, native_reentry_array, "length", 0, FVM_PROPERTY_DONT_DELETE) == FVM_OK);
        int removed = 0; assert(fvm_object_delete(c, native_reentry_array, "length", &removed) == FVM_OK && removed);
    }
    int removed = 0; assert(fvm_object_delete(c, c->global, "reassignmentInput", &removed) == FVM_OK && removed);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    fvm_value_number(out, 1); return native_reentry_fault == 1 ? FVM_ERROR : native_reentry_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned route = 0; route < 4; ++route)
    for (native_reentry_mode = 0; native_reentry_mode < 2; ++native_reentry_mode)
    for (native_reentry_fault = 0; native_reentry_fault < 3; ++native_reentry_fault) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value initial[3]; for (unsigned j = 0; j < 3; ++j) fvm_value_number(initial + j, 10 + j);
        assert(fvm_construct(c, "Array", 3, initial, &native_reentry_array) == FVM_OK && fvm_gc_pin(c, native_reentry_array) == FVM_OK && fvm_construct(c, "Object", 0, 0, &native_reentry_input) == FVM_OK);
        FVM_Object *fn = version_function(c, 1); assert(fvm_function_set_body(c, fn, native_reentry_convert, 0, 0) == FVM_OK);
        FVM_Value input = class_value(native_reentry_input), value = class_value(fn), out; assert(fvm_object_set(c, native_reentry_input, "valueOf", &value) == FVM_OK);
        native_reentry_calls = 0; FVM_Result expected = !native_reentry_fault ? FVM_OK : native_reentry_fault == 1 ? FVM_ERROR : FVM_EXCEPTION;
        assert(reassignment_write(c, native_reentry_array, input, route, expected) == expected && native_reentry_calls == 1);
        if (c->exception_pending) assert(fvm_exception_clear(c) == FVM_OK);
        if (native_reentry_mode) assert(fvm_base_accessor_data(native_reentry_array->accessors.find("length")->second, out));
        else assert(fvm_object_get(c, native_reentry_array, "length", &out) == FVM_OK);
        if (!native_reentry_fault) assert(out.type == FVM_VALUE_OBJECT && out.object == native_reentry_input);
        else if (native_reentry_mode) assert(out.type == FVM_VALUE_NUMBER && out.number == 3);
        else assert(out.type == FVM_VALUE_UNDEFINED);
        int own = 0; assert(fvm_object_has_own(c, native_reentry_array, "1", &own) == FVM_OK && !!own == (!native_reentry_mode || !!native_reentry_fault));
        assert(native_reentry_array->resource_result.type == FVM_VALUE_NUMBER && native_reentry_array->resource_result.number == 0);
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->script_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
