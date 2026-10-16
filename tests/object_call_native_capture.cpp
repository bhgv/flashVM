/* Trusted collecting native calls retain a selected capture after registration replacement. */
#define main call_case_fixture_main
#include "object_call_case.cpp"
#undef main
static FVM_Object *capture_owner, *capture_object, *capture_argument;
static unsigned capture_source, capture_status, capture_calls;
static FVM_Value capture_input[2];
static FVM_Result capture_replacement(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ fvm_value_number(out, 29); return FVM_OK; }
static bool capture_alive(FVM_Context *c, FVM_Object *object)
{ for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == object) return true; return false; }
static FVM_Result capture_callback(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *userdata)
{
    ++capture_calls; assert(!c->gc_native_depth && userdata == capture_object && n == 2 && receiver == call_receiver);
    if (capture_source == 0) { int removed = 0; assert(fvm_object_delete(c, capture_owner, "run", &removed) == FVM_OK && removed); }
    else if (capture_source == 1) assert(fvm_object_define_method_rooted(c, capture_owner, "run", capture_replacement, 0, 0) == FVM_OK);
    else assert(fvm_register_function_rooted(c, "run", capture_replacement, 0, 0) == FVM_OK);
    fvm_value_undefined(capture_input); fvm_value_undefined(capture_input + 1);
    assert(fvm_gc_collect(c, 0) == FVM_OK && capture_alive(c, capture_object) && capture_alive(c, capture_argument));
    assert(fvm_value_string_size(args) == 3 && !std::memcmp(args[0].string, "a\0b", 3) && args[1].object == capture_argument);
    FVM_Value edge; assert(fvm_object_get(c, capture_object, "self", &edge) == FVM_OK && edge.object == capture_object);
    *out = call_object(capture_object); return capture_status == 1 ? fvm_exception_raise(c, out) : capture_status == 2 ? FVM_ERROR : FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (capture_source = 0; capture_source < 3; ++capture_source) for (capture_status = 0; capture_status < 3; ++capture_status) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &call_receiver) == FVM_OK && fvm_gc_pin(c, call_receiver) == FVM_OK);
        capture_owner = call_receiver;
        if (capture_source == 1) { assert(fvm_construct(c, "Object", 0, 0, &capture_owner) == FVM_OK && fvm_object_set_prototype(c, call_receiver, capture_owner) == FVM_OK); }
        assert(fvm_object_create(c, &capture_object) == FVM_OK); FVM_Value value = call_object(capture_object); assert(fvm_object_set(c, capture_object, "self", &value) == FVM_OK);
        if (capture_source == 2) { assert(fvm_register_function_rooted(c, "run", capture_callback, capture_object, capture_object) == FVM_OK); c->functions.find(fvm_detail::string("run"))->second.collector_entry = true; }
        else { assert(fvm_object_define_method_rooted(c, capture_owner, "run", capture_callback, capture_object, capture_object) == FVM_OK); capture_owner->methods.find(fvm_detail::string("run"))->second.collector_entry = true; }
        /* collector_entry is internal trusted-bridge state, not a public native GC opt-out. */
        assert(fvm_object_create(c, &capture_argument) == FVM_OK); value = call_object(capture_argument); assert(fvm_object_set(c, capture_argument, "self", &value) == FVM_OK);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_value_to_string(c, &value, capture_input) == FVM_OK); capture_input[1] = call_object(capture_argument);
        capture_calls = 0; FVM_Value out; FVM_Result status = fvm_object_call(c, call_receiver, "run", 2, capture_input, &out);
        assert(capture_calls == 1 && status == (capture_status == 1 ? FVM_EXCEPTION : capture_status == 2 ? FVM_ERROR : FVM_OK));
        if (!capture_status) {
            assert(out.type == FVM_VALUE_OBJECT && out.object == capture_object && fvm_gc_pin(c, out.object) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && capture_alive(c, capture_object));
            assert(fvm_gc_unpin(c, out.object) == FVM_OK);
        } else if (capture_status == 1) {
            assert(fvm_exception_get(c, &out) == FVM_OK && out.object == capture_object && fvm_gc_collect(c, 0) == FVM_OK && capture_alive(c, capture_object));
            assert(fvm_exception_clear(c) == FVM_OK);
        }
        fvm_value_undefined(&out); assert(fvm_gc_collect(c, 0) == FVM_OK && !capture_alive(c, capture_object) && !capture_alive(c, capture_argument));
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
