/* Direct Function invocation snapshots aliased/mutable arguments and retains all active inputs. */
#define main call_case_fixture_main
#include "object_call_case.cpp"
#undef main
static FVM_Value invoke_input[2];
static FVM_Object *invoke_argument, *invoke_receiver;
static unsigned invoke_mutate, invoke_fault, invoke_calls;
static FVM_Result invoke_result(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out)
{
    ++invoke_calls; assert(receiver == invoke_receiver);
    if (invoke_mutate) { fvm_value_undefined(invoke_input); fvm_value_undefined(invoke_input + 1); }
    assert(n == 2 && args[0].type == FVM_VALUE_STRING && fvm_value_string_size(args) == 3 && !std::memcmp(args[0].string, "a\0b", 3));
    assert(args[1].type == FVM_VALUE_OBJECT && args[1].object == invoke_argument);
    if (!c->gc_native_depth) assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value marker; assert(fvm_object_get(c, invoke_argument, "marker", &marker) == FVM_OK && marker.number == 55);
    *out = invoke_fault == 2 ? args[1] : args[0]; return invoke_fault == 1 ? FVM_ERROR : invoke_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
static FVM_Result invoke_native(FVM_Context *c, FVM_Object *receiver, unsigned n, const FVM_Value *args, FVM_Value *out, void *)
{ return invoke_result(c, receiver, n, args, out); }
static FVM_Result invoke_body(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{ return invoke_result(c, frame->receiver, frame->argc, frame->argv, out); }
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned route = 0; route < 3; ++route) for (unsigned receiver_kind = 0; receiver_kind < 2; ++receiver_kind) for (unsigned alias = 0; alias < 3; ++alias) for (invoke_mutate = 0; invoke_mutate < 2; ++invoke_mutate) for (invoke_fault = 0; invoke_fault < 3; ++invoke_fault) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *function = 0; const unsigned char end[] = {0}; FVM_Value value, out;
        if (route == 1) assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &function) == FVM_OK && fvm_function_set_body(c, function, invoke_body, 0, 0) == FVM_OK);
        else { assert(fvm_function_create_native(c, invoke_native, 0, 0, &function) == FVM_OK); function->invocable.collector_entry = route == 2; }
        invoke_receiver = c->global; if (receiver_kind) assert(fvm_object_create(c, &invoke_receiver) == FVM_OK);
        assert(fvm_object_create(c, &invoke_argument) == FVM_OK); value = call_object(invoke_argument); assert(fvm_object_set(c, invoke_argument, "self", &value) == FVM_OK);
        fvm_value_number(&value, 55); assert(fvm_object_set(c, invoke_argument, "marker", &value) == FVM_OK);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_value_to_string(c, &value, invoke_input) == FVM_OK); invoke_input[1] = call_object(invoke_argument);
        invoke_calls = 0; FVM_Value *output = alias ? invoke_input + alias - 1 : &out;
        FVM_Result expected = invoke_fault == 1 ? FVM_ERROR : invoke_fault == 2 ? FVM_EXCEPTION : FVM_OK;
        assert(fvm_function_invoke(c, function, receiver_kind ? invoke_receiver : 0, 2, invoke_input, output) == expected && invoke_calls == 1);
        if (expected == FVM_OK) { assert(output->type == FVM_VALUE_STRING && fvm_value_string_size(output) == 3 && !std::memcmp(output->string, "a\0b", 3)); assert(fvm_set_global(c, "reply", output) == FVM_OK); }
        fvm_value_undefined(invoke_input); fvm_value_undefined(invoke_input+1);
        if (expected == FVM_EXCEPTION) { assert(fvm_gc_collect(c, 0) == FVM_OK && fvm_exception_get(c, &value) == FVM_OK && value.object == invoke_argument && fvm_exception_clear(c) == FVM_OK); }
        assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && fvm_gc_collect(c, 0) == FVM_OK);
        for (size_t i = 0; i < c->objects.size(); ++i) assert(c->objects[i] != invoke_argument);
        if (expected == FVM_OK) assert(fvm_get_global(c, "reply", &out) == FVM_OK && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime); return 0;
}
