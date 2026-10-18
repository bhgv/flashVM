/* Borrowed Function adapters prepare arguments before rejecting non-callable targets. */
#define FVM_APPLY_WORK_FIXTURE_ONLY
#include "function_apply_work.cpp"
static unsigned borrowed_reads, borrowed_fault;
static FVM_Result borrowed_getter(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++borrowed_reads;
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (borrowed_fault == 1) return FVM_ERROR;
    if (borrowed_fault == 2) { FVM_Value error; fvm_value_string_bytes(&error, "borrow\0failed", 13); return fvm_exception_raise(c, &error); }
    fvm_value_number(out, 42); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; FVM_Object *fn = 0, *plain = 0, *source = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        assert(fvm_function_create_native(c, work_target, 0, 0, &fn) == FVM_OK && fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK && fvm_gc_pin(c, plain) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
        FVM_Value apply_method, call_method, value, args[3], out;
        assert(fvm_object_get(c, fn, "apply", &apply_method) == FVM_OK && fvm_gc_pin(c, apply_method.object) == FVM_OK);
        assert(fvm_object_get(c, fn, "call", &call_method) == FVM_OK && fvm_gc_pin(c, call_method.object) == FVM_OK);
        fvm_value_number(&value, 1); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        apply_getter(c, source, "0", borrowed_getter);
        args[0] = apply_object(plain); fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL; args[2] = apply_object(source);
        for (unsigned phase = 0; phase < 4; ++phase) {
            borrowed_fault = phase == 3 ? 0 : phase; borrowed_reads = 0; work_calls = 0;
            FVM_Result status = fvm_object_call(c, apply_method.object, "call", 3, args, &out);
            assert(borrowed_reads == 1 && !work_calls);
            if (borrowed_fault) {
                assert(status == (borrowed_fault == 1 ? FVM_ERROR : FVM_EXCEPTION));
                if (borrowed_fault == 2) { assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 13 && !std::memcmp(out.string, "borrow\0failed", 13)); assert(fvm_exception_clear(c) == FVM_OK); }
            } else if (version) assert(status == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
            else assert(status == FVM_UNSUPPORTED);
            assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        }
        fvm_value_number(args + 2, 99);
        FVM_Result status = fvm_object_call(c, call_method.object, "call", 3, args, &out);
        if (version) assert(status == FVM_OK && out.type == FVM_VALUE_UNDEFINED); else assert(status == FVM_UNSUPPORTED);
        args[0] = apply_object(fn); work_calls = 0;
        assert(fvm_object_call(c, call_method.object, "call", 3, args, &out) == FVM_OK && out.number == 1 && work_calls == 1);
        args[0] = apply_object(plain); args[2] = apply_object(source);
        if (version) {
            fvm_value_number(&value, 2); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
            rt->config.instruction_limit = 1; borrowed_reads = 0;
            assert(fvm_object_call(c, apply_method.object, "call", 3, args, &out) == FVM_ERROR && borrowed_reads == 1 && c->avm1_steps == 1);
            rt->config.instruction_limit = 0;
            assert(fvm_object_call(c, apply_method.object, "call", 3, args, &out) == FVM_OK && out.type == FVM_VALUE_UNDEFINED);
        }
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
