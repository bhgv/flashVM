#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static FVM_Object *receiver;
static unsigned mode, field_calls, option_calls;
static FVM_Result field(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    assert(!field_calls++ && !option_calls); FVM_Value value;
    fvm_value_number(&value, 1); assert(fvm_table_set_index(c, receiver, 0, &value) == FVM_OK);
    fvm_value_number(&value, 3); assert(fvm_object_set(c, receiver, "length", &value) == FVM_OK);
    fvm_value_number(&value, 0); assert(fvm_table_set_index(c, receiver, 2, &value) == FVM_OK);
    if (mode == 1) return FVM_ERROR;
    return fvm_value_string(out, "unused");
}
static FVM_Result option(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    assert(field_calls == 1 && !option_calls++); FVM_Value value;
    fvm_value_number(&value, -1); assert(fvm_table_set_index(c, receiver, 2, &value) == FVM_OK);
    if (mode == 2) return FVM_ERROR;
    fvm_value_number(out, 16); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, args[2], out; FVM_Object *methods = 0;
        assert(fvm_construct(c, "Array", 0, 0, &methods) == FVM_OK); assert(fvm_gc_pin(c, methods) == FVM_OK);
        for (unsigned i = 0; i < 2; ++i) {
            FVM_Object *coercer = 0, *fn = 0, *array = 0; assert(fvm_construct(c, "Object", 0, 0, &coercer) == FVM_OK);
            assert(fvm_function_create_native(c, i ? option : field, 0, 0, &fn) == FVM_OK); value = object(fn);
            assert(fvm_object_set(c, coercer, i ? "valueOf" : "toString", &value) == FVM_OK); value = object(coercer);
            assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK); assert(fvm_gc_pin(c, array) == FVM_OK); args[i] = object(array);
        }
        for (unsigned native = 0; native < 2; ++native) for (mode = 0; mode < 3; ++mode) {
            assert(fvm_construct(c, native ? "Array" : "Object", 0, 0, &receiver) == FVM_OK); assert(fvm_gc_pin(c, receiver) == FVM_OK);
            fvm_value_number(&value, 2); assert(fvm_object_set(c, receiver, "length", &value) == FVM_OK);
            fvm_value_number(&value, 10); assert(fvm_table_set_index(c, receiver, 0, &value) == FVM_OK); fvm_value_number(&value, 2); assert(fvm_table_set_index(c, receiver, 1, &value) == FVM_OK);
            field_calls = option_calls = 0; assert(borrow(c, methods, "sortOn", receiver, 2, args, &out) == (mode ? FVM_ERROR : FVM_OK));
            assert(field_calls == 1 && option_calls == (mode == 1 ? 0U : 1U)); number(c, receiver, "length", 3);
            assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
            number(c, receiver, "0", mode ? 1 : -1); number(c, receiver, "1", mode ? 2 : 1); number(c, receiver, "2", !mode ? 2 : mode == 1 ? 0 : -1);
            assert(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->calling_setter_owner == 0);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
