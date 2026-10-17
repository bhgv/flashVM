/* Coercible recreated length can remove its own binding and collect during Array methods. */
#include "../src/standard/base_state.hpp"
#define FVM_ARRAY_DELETED_LENGTH_FIXTURE_ONLY
#include "array_deleted_length.cpp"
static unsigned deleted_array_conversions, deleted_array_fault;
static FVM_Result deleted_array_number(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++deleted_array_conversions; assert(frame->capture);
    int removed = 0; assert(fvm_object_delete(c, frame->capture, "length", &removed) == FVM_OK && removed);
    assert(fvm_object_delete(c, c->global, "arrayDeletedTarget", &removed) == FVM_OK && removed);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    bool alive = false; for (size_t i = 0; i < c->objects.size(); ++i) alive |= c->objects[i] == frame->capture; assert(alive);
    fvm_value_number(out, 1);
    return deleted_array_fault == 1 ? FVM_ERROR : deleted_array_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
/* Raw fixture storage defers the destructive callback until the operation under test. */
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"join", "slice", "push", "unshift", "sort"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 5; ++method)
    for (deleted_array_fault = 0; deleted_array_fault < 3; ++deleted_array_fault)
    for (unsigned route = 0; route < 5; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *array = deleted_length_array(c), *number = 0, *fn = deleted_length_function(c);
        assert(fvm_construct(c, "Object", 0, 0, &number) == FVM_OK && fvm_function_set_body(c, fn, deleted_array_number, 0, array) == FVM_OK);
        FVM_Value value = class_value(fn), out; assert(fvm_object_set(c, number, "valueOf", &value) == FVM_OK); value = class_value(number); assert(fvm_base_store_data(c, array, "length", value, false) == FVM_OK);
        value = class_value(array); assert(fvm_set_global(c, "arrayDeletedTarget", &value) == FVM_OK);
        const FVM_Result expected = !deleted_array_fault ? FVM_OK : deleted_array_fault == 1 ? FVM_ERROR : FVM_EXCEPTION;
        const unsigned argc = method == 2 || method == 3 ? 1 : 0; deleted_array_conversions = 0;
        if (route == 4) {
            assert(fvm_gc_pin(c, array) == FVM_OK);
            FVM_Object *owner = array; FVM_NativeEntry entry; bool found = false;
            for (; owner && !found; owner = owner->prototype) { fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method_entry = owner->methods.find(methods[method]); if (method_entry != owner->methods.end()) { entry = method_entry->second; found = true; } }
            assert(found); fvm_value_number(&value, 81); assert(entry.callback(c, array, argc, &value, &out, entry.userdata) == expected);
            if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
        }
        else if (!route) { fvm_value_number(&value, 81); assert(fvm_object_call(c, array, methods[method], argc, &value, &out) == expected); if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK); }
        else { Program p; if (argc) p.number(81); p.number(argc); p.variable("arrayDeletedTarget"); p.text(methods[method]); p.op(0x52); p.finish(); out = boxing_run(c, p, route - 1, expected); }
        assert(deleted_array_conversions == 1 && !c->exception_pending);
        if (!deleted_array_fault) {
            if (!method) assert(out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "11"));
            if (method == 1) { assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 1); }
            if (argc) assert(out.type == FVM_VALUE_NUMBER && out.number == 2);
        }
        assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK);
        int own = 0; assert(fvm_object_has_own(c, array, "length", &own) == FVM_OK && !!own == (!deleted_array_fault && method >= 2));
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->script_this_cell); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
