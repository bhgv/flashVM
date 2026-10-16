/* C call arguments survive collecting getter mutation and permit aliased output. */
#define main call_case_fixture_main
#include "object_call_case.cpp"
#undef main
static FVM_Value pending_args[2];
static FVM_Object *pending_object, *snapshot_function;
static unsigned snapshot_mutate, snapshot_fault, snapshot_calls;
static FVM_Result snapshot_get(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    if (snapshot_mutate) { fvm_value_undefined(pending_args); fvm_value_undefined(pending_args + 1); }
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    bool live = false; for (size_t i = 0; i < c->objects.size(); ++i) if (c->objects[i] == pending_object) live = true; assert(live);
    if (snapshot_fault == 1) { fvm_value_number(out, 71); return fvm_exception_raise(c, out); }
    *out = call_object(snapshot_function); return FVM_OK;
}
static FVM_Result snapshot_method(FVM_Context *c, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{
    ++snapshot_calls; assert(frame->argc == 2 && fvm_value_string_size(frame->argv) == 3 && !std::memcmp(frame->argv[0].string, "a\0b", 3));
    assert(frame->argv[1].type == FVM_VALUE_OBJECT && frame->argv[1].object == pending_object);
    assert(fvm_gc_collect(c, 0) == FVM_OK);
    FVM_Value marker; assert(fvm_object_get(c, pending_object, "marker", &marker) == FVM_OK && marker.number == 55);
    fvm_value_number(out, snapshot_fault == 2 ? 72 : 17); return snapshot_fault == 2 ? fvm_exception_raise(c, out) : FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (snapshot_mutate = 0; snapshot_mutate < 2; ++snapshot_mutate) for (unsigned alias = 0; alias < 2; ++alias) for (snapshot_fault = 0; snapshot_fault < 3; ++snapshot_fault) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK);
        FVM_Object *receiver = 0, *get = 0; const unsigned char end[] = {0}; FVM_Value args[3], value, out;
        assert(fvm_construct(c, "Object", 0, 0, &receiver) == FVM_OK && fvm_gc_pin(c, receiver) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &snapshot_function) == FVM_OK && fvm_function_set_body(c, snapshot_function, snapshot_method, 0, 0) == FVM_OK && fvm_gc_pin(c, snapshot_function) == FVM_OK);
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &get) == FVM_OK && fvm_function_set_body(c, get, snapshot_get, 0, 0) == FVM_OK);
        fvm_value_string(args, "run"); args[1] = call_object(get); fvm_value_undefined(args + 2); args[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, receiver, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
        assert(fvm_object_create(c, &pending_object) == FVM_OK); value = call_object(pending_object); assert(fvm_object_set(c, pending_object, "self", &value) == FVM_OK);
        fvm_value_number(&value, 55); assert(fvm_object_set(c, pending_object, "marker", &value) == FVM_OK);
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_value_to_string(c, &value, pending_args) == FVM_OK); pending_args[1] = call_object(pending_object);
        snapshot_calls = 0; FVM_Value *output = alias ? pending_args : &out;
        FVM_Result status = fvm_object_call(c, receiver, "run", 2, pending_args, output);
        assert(status == (snapshot_fault ? FVM_EXCEPTION : FVM_OK) && snapshot_calls == (snapshot_fault == 1 ? 0U : 1U));
        if (!snapshot_fault) assert(output->type == FVM_VALUE_NUMBER && output->number == 17);
        else assert(fvm_exception_get(c, &value) == FVM_OK && value.number == (snapshot_fault == 1 ? 71 : 72) && fvm_exception_clear(c) == FVM_OK);
        fvm_value_undefined(pending_args); fvm_value_undefined(pending_args + 1);
        assert(!c->gc_frames && !c->gc_native_depth && !c->calling_owner && !c->calling_function && fvm_gc_collect(c, 0) == FVM_OK);
        for (size_t i = 0; i < c->objects.size(); ++i) assert(c->objects[i] != pending_object);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
