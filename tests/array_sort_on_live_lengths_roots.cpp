/* sortOn snapshots field length once, converts all names, then reads/converts options. */
#define FVM_SORT_LIVE_LENGTH_FIXTURE_ONLY
#include "array_sort_on_live_lengths.cpp"
static unsigned sort_live_event, sort_live_failure, sort_live_throw, sort_live_empty;
static unsigned sort_live_phases[] = {1,2,3,4};
static FVM_Result sort_live_conversion(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *data)
{
    const unsigned phase = *(unsigned *)data; assert(phase == ++sort_live_event && frame->capture);
    int removed = 0;
    if (phase == 1 || phase == 3) {
        assert(fvm_object_delete(c, frame->capture, "length", &removed) == FVM_OK && removed);
        assert(fvm_object_delete(c, c->global, phase == 1 ? "sortLiveFields" : "sortLiveOptions", &removed) == FVM_OK && removed);
    } else if (phase == 2) assert(fvm_object_delete(c, frame->capture, "0", &removed) == FVM_OK && removed);
    else assert(fvm_object_delete(c, c->global, "sortLiveReceiver", &removed) == FVM_OK && removed);
    assert(fvm_gc_collect(c, 0) == (c->gc_native_depth ? FVM_INVALID_ARGUMENT : FVM_OK));
    if (sort_live_failure == phase) { fvm_value_number(out, 77); return sort_live_throw ? fvm_exception_raise(c, out) : FVM_ERROR; }
    if (phase == 2) return fvm_value_string(out, "key");
    fvm_value_number(out, phase == 4 ? 18 : phase == 1 && sort_live_empty ? 0 : 1); return FVM_OK;
}
static FVM_Object *sort_live_convertible(FVM_Context *c, unsigned phase, FVM_Object *capture)
{
    FVM_Object *object = 0, *fn = version_function(c, 0); assert(fvm_construct(c, "Object", 0, 0, &object) == FVM_OK && fvm_function_set_body(c, fn, sort_live_conversion, &sort_live_phases[phase - 1], capture) == FVM_OK);
    FVM_Value value = class_value(fn); assert(fvm_object_set(c, object, phase == 2 ? "toString" : "valueOf", &value) == FVM_OK); return object;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (sort_live_empty = 0; sort_live_empty < 2; ++sort_live_empty)
    for (sort_live_failure = 0; sort_live_failure <= 4; ++sort_live_failure)
    for (sort_live_throw = 0; sort_live_throw < 2; ++sort_live_throw)
    for (unsigned route = 0; route < 5; ++route) {
        if ((route == 2 || route == 3) && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *receiver = sort_live_receiver(c), *fields = sort_live_list(c, false, 1), *options = sort_live_list(c, true, 1); FVM_Value value;
        /* Install elements before coercible lengths: indexed assignment now reads live length. */
        value = class_value(sort_live_convertible(c, 2, fields)); assert(fvm_table_set_index(c, fields, 0, &value) == FVM_OK);
        value = class_value(sort_live_convertible(c, 4, receiver)); assert(fvm_table_set_index(c, options, 0, &value) == FVM_OK);
        value = class_value(sort_live_convertible(c, 1, fields)); assert(fvm_object_set(c, fields, "length", &value) == FVM_OK);
        value = class_value(sort_live_convertible(c, 3, options)); assert(fvm_object_set(c, options, "length", &value) == FVM_OK);
        FVM_Object *original[3]; for (unsigned i = 0; i < 3; ++i) { assert(fvm_table_get_index(c, receiver, i, &value) == FVM_OK); original[i] = value.object; }
        const bool fails = sort_live_failure && (!sort_live_empty || sort_live_failure == 1);
        const FVM_Result expected = fails ? sort_live_throw ? FVM_EXCEPTION : FVM_ERROR : FVM_OK; sort_live_event = 0;
        if (route == 4) {
            assert(fvm_gc_pin(c, receiver) == FVM_OK && fvm_gc_pin(c, fields) == FVM_OK && fvm_gc_pin(c, options) == FVM_OK);
            value = class_value(receiver); assert(fvm_set_global(c, "sortLiveReceiver", &value) == FVM_OK); value = class_value(fields); assert(fvm_set_global(c, "sortLiveFields", &value) == FVM_OK); value = class_value(options); assert(fvm_set_global(c, "sortLiveOptions", &value) == FVM_OK);
            FVM_Object *owner = receiver; FVM_NativeEntry entry; bool found = false;
            for (; owner && !found; owner = owner->prototype) { fvm_detail::map<fvm_detail::string, FVM_NativeEntry>::iterator method = owner->methods.find("sortOn"); if (method != owner->methods.end()) { entry = method->second; found = true; } }
            assert(found); FVM_Value args[2] = {class_value(fields),class_value(options)}; assert(entry.callback(c, receiver, 2, args, &value, entry.userdata) == expected); if (expected == FVM_EXCEPTION) assert(fvm_exception_clear(c) == FVM_OK);
        } else value = sort_live_call(c, receiver, fields, options, route, expected);
        assert(sort_live_event == (fails ? sort_live_failure : sort_live_empty ? 1 : 4));
        if (!fails) { assert(value.type == FVM_VALUE_OBJECT && value.object == receiver); const unsigned descending[] = {2,0,1}; for (unsigned i = 0; i < 3; ++i) assert(fvm_table_get_index(c, receiver, i, &value) == FVM_OK && value.object == original[sort_live_empty ? i : descending[i]]); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->exception_pending && !c->script_this_cell); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
