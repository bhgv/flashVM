/* Indexed growth roots detached watched graphs and converts live length exactly once. */
#define FVM_INDEX_LIVE_LENGTH_FIXTURE_ONLY
#include "array_index_live_length.cpp"
static FVM_Object *index_live_array, *index_live_input, *index_live_meta;
static unsigned index_live_calls, index_live_fault, index_live_setters;
static bool index_live_alive(FVM_Context *c, FVM_Object *object)
{
    for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == object) return true;
    return false;
}
static FVM_Result index_live_number(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++index_live_calls; int removed = 0;
    assert(fvm_object_delete(c, index_live_array, "length", &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "indexLiveTarget", &removed) == FVM_OK);
    assert(fvm_object_delete(c, c->global, "indexLiveValue", &removed) == FVM_OK);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    assert(index_live_alive(c, index_live_array) && index_live_alive(c, index_live_input) && index_live_alive(c, index_live_meta));
    fvm_value_number(out, 1);
    return index_live_fault == 1 ? FVM_ERROR : index_live_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
static FVM_Result index_live_setter(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++index_live_setters; FVM_Value value; assert(fvm_object_get(c, index_live_array, "length", &value) == FVM_OK && value.number == 4);
    fvm_value_number(&value, 0); assert(fvm_object_set(c, index_live_array, "length", &value) == FVM_OK);
    int removed = 0; assert(fvm_object_delete(c, index_live_array, "3", &removed) == FVM_OK && removed);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    fvm_value_undefined(out); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned route = 0; route < 5; ++route)
    for (index_live_fault = 0; index_live_fault < 3; ++index_live_fault)
    for (unsigned setter = 0; setter < 2; ++setter) {
        if ((route == 2 || route == 3) && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        index_live_array = deleted_length_array(c); assert(fvm_construct(c, "Object", 0, 0, &index_live_input) == FVM_OK && fvm_object_create(c, &index_live_meta) == FVM_OK);
        FVM_Object *number = 0, *function = version_function(c, 1); assert(fvm_construct(c, "Object", 0, 0, &number) == FVM_OK && fvm_function_set_body(c, function, index_live_number, 0, 0) == FVM_OK);
        FVM_Value value = class_value(function), input = class_value(index_live_input), out;
        value = class_value(index_live_meta); assert(fvm_object_set(c, index_live_input, "edge", &value) == FVM_OK); value = class_value(function); assert(fvm_object_set(c, number, "valueOf", &value) == FVM_OK); value = class_value(number); assert(fvm_object_set(c, index_live_array, "length", &value) == FVM_OK);
        value = class_value(index_live_array); assert(fvm_set_global(c, "indexLiveTarget", &value) == FVM_OK && fvm_set_global(c, "indexLiveValue", &input) == FVM_OK);
        if (setter) { FVM_Object *fn = version_function(c, 0); assert(fvm_function_set_body(c, fn, index_live_setter, 0, 0) == FVM_OK); FVM_Value args[3]; fvm_value_string(args, "3"); args[1] = class_value(version_function(c, 7)); args[2] = class_value(fn); assert(fvm_object_call(c, index_live_array, "addProperty", 3, args, &out) == FVM_OK && out.boolean); }
        index_live_calls = index_live_setters = 0; FVM_Result expected = !index_live_fault ? FVM_OK : index_live_fault == 1 ? FVM_ERROR : FVM_EXCEPTION;
        assert(index_live_write(c, index_live_array, "3", input, route, expected) == expected); if (c->exception_pending) assert(fvm_exception_clear(c) == FVM_OK);
        assert(index_live_calls == 1 && index_live_setters == (!index_live_fault && setter ? 1U : 0U));
        assert(fvm_gc_pin(c, index_live_array) == FVM_OK);
        if (!index_live_fault) { assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_object_get(c, index_live_array, "3", &out) == FVM_OK && out.type == FVM_VALUE_OBJECT && out.object == index_live_input); assert(fvm_object_get(c, index_live_array, "length", &out) == FVM_OK && out.type == FVM_VALUE_NUMBER && out.number == (setter ? 0 : 4)); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->script_this_cell && !c->calling_setter_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
