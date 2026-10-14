#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned primitive_calls, text_calls;
static FVM_Result primitive(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++primitive_calls; *out = object(self); return FVM_OK; }
static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++text_calls; return fvm_value_string(out, "23"); }
static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ return FVM_ERROR; }
static void check(FVM_Context *c, const FVM_Value *arg, double expected, bool nan)
{
    FVM_Value out; FVM_Object *boxed = 0;
    assert(fvm_call(c, "Number", arg ? 1 : 0, arg, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER); assert(nan ? out.number != out.number : out.number == expected);
    assert(fvm_construct(c, "Number", arg ? 1 : 0, arg, &boxed) == FVM_OK);
    assert(fvm_object_call(c, boxed, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_NUMBER); assert(nan ? out.number != out.number : out.number == expected);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value arg, value, out; FVM_Object *plain = 0, *fn = 0, *boxed = 0;
        check(c, 0, 0, false); fvm_value_undefined(&arg); check(c, &arg, 0, !version || version >= 7);
        fvm_value_string(&arg, "010"); check(c, &arg, version >= 6 ? 8 : 10, false);
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
        assert(fvm_function_create_native(c, primitive, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK);
        assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "toString", &value) == FVM_OK);
        arg = object(plain); primitive_calls = text_calls = 0; check(c, &arg, version == 4 ? 0 : 23, version >= 5); assert(primitive_calls == 2 && text_calls == (version ? 0U : 2U));
        assert(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK);
        assert(fvm_call(c, "Number", 1, &arg, &out) == FVM_ERROR); assert(fvm_construct(c, "Number", 1, &arg, &boxed) == FVM_ERROR);
        fvm_value_string(&arg, "010"); assert(fvm_construct(c, "Number", 1, &arg, &boxed) == FVM_OK); assert(fvm_gc_pin(c, boxed) == FVM_OK); assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_object_call(c, boxed, "valueOf", 0, 0, &out) == FVM_OK && out.number == (version >= 6 ? 8 : 10));
        assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
