/* Borrowed Function adapters do not turn loaded super facades into ordinary functions. */
#define main super_adapter_fixture_main
#include "avm1_super.cpp"
#undef main
#include "../src/core/runtime.hpp"
static unsigned parent_calls, source_reads, source_fault;
static FVM_Result source_body(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++source_reads;
    assert(!c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK);
    if (source_fault == 1) return FVM_ERROR;
    if (source_fault == 2) { FVM_Value error; fvm_value_string_bytes(&error, "super\0failed", 12); return fvm_exception_raise(c, &error); }
    fvm_value_number(out, 77); return FVM_OK;
}
static FVM_Result parent_body(FVM_Context *, const FVM_ScriptFrame *frame, FVM_Value *out, void *)
{ ++parent_calls; fvm_value_number(out, frame->argc); return FVM_OK; }
static FVM_Value object_value(FVM_Object *object)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = object; return value; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 5) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        Bytes empty; empty.push_back(0);
        FVM_Value base = define(c, "Base", 0, 0, empty), sub = define(c, "Sub", 0, 0, empty);
        assert(fvm_function_set_body(c, base.object, parent_body, 0, 0) == FVM_OK); extend(c, "Sub", "Base");
        Bytes escape; variable(escape, "super"); result(escape);
        FVM_Value method = define(c, "escape", 0, 0, escape);
        assert(fvm_object_set(c, prototype(c, sub), "escape", &method) == FVM_OK);
        FVM_Object *instance = 0;
        assert(fvm_function_construct(c, sub.object, 0, 0, &instance) == FVM_OK && fvm_gc_pin(c, instance) == FVM_OK);
        FVM_Value facade, out, call_method, apply_method, args[3];
        assert(fvm_object_call(c, instance, "escape", 0, 0, &facade) == FVM_OK && facade.type == FVM_VALUE_OBJECT);
        assert(facade.object->resource_kind == FVM_RESOURCE_SUPER && fvm_gc_pin(c, facade.object) == FVM_OK);
        assert(fvm_object_get(c, base.object, "call", &call_method) == FVM_OK && fvm_gc_pin(c, call_method.object) == FVM_OK);
        assert(fvm_object_get(c, base.object, "apply", &apply_method) == FVM_OK && fvm_gc_pin(c, apply_method.object) == FVM_OK);
        FVM_Object *source = 0; assert(fvm_construct(c, "Object", 0, 0, &source) == FVM_OK && fvm_gc_pin(c, source) == FVM_OK);
        FVM_Value value; fvm_value_number(&value, 1); assert(fvm_object_set(c, source, "length", &value) == FVM_OK);
        const unsigned char end[] = {0}; FVM_Object *source_getter = 0;
        assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &source_getter) == FVM_OK && fvm_function_set_body(c, source_getter, source_body, 0, 0) == FVM_OK);
        FVM_Value descriptor[3]; fvm_value_string(descriptor, "0"); descriptor[1] = object_value(source_getter); fvm_value_undefined(descriptor + 2); descriptor[2].type = FVM_VALUE_NULL;
        assert(fvm_object_call(c, source, "addProperty", 3, descriptor, &out) == FVM_OK && out.boolean);
        args[0] = facade; fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL; args[2] = object_value(source);
        for (source_fault = 1; source_fault <= 2; ++source_fault) {
            parent_calls = source_reads = 0;
            assert(fvm_object_call(c, apply_method.object, "call", 3, args, &out) == (source_fault == 1 ? FVM_ERROR : FVM_EXCEPTION));
            assert(source_reads == 1 && !parent_calls && !c->gc_frames && !c->avm1_execution_depth);
            if (source_fault == 2) { assert(fvm_exception_get(c, &out) == FVM_OK && fvm_value_string_size(&out) == 12 && !std::memcmp(out.string, "super\0failed", 12)); assert(fvm_exception_clear(c) == FVM_OK); }
        }
        source_fault = source_reads = 0;
        parent_calls = 0;
        assert(fvm_object_call(c, apply_method.object, "call", 3, args, &out) == FVM_OK);
        assert(source_reads == 1);
        if (version) assert(out.type == FVM_VALUE_UNDEFINED && !parent_calls);
        else assert(out.type == FVM_VALUE_NUMBER && out.number == 1 && parent_calls == 1);
        fvm_value_number(args + 2, 77); parent_calls = 0;
        assert(fvm_object_call(c, call_method.object, "call", 3, args, &out) == FVM_OK);
        if (version) assert(out.type == FVM_VALUE_UNDEFINED && !parent_calls);
        else assert(out.type == FVM_VALUE_NUMBER && out.number == 1 && parent_calls == 1);
        parent_calls = 0;
        assert(fvm_function_invoke(c, facade.object, instance, 1, args + 2, &out) == FVM_OK && out.number == 1 && parent_calls == 1);
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
