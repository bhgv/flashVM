#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
static unsigned primitive_calls, text_calls;
static FVM_Result primitive(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++primitive_calls; *out = object(self); return FVM_OK; }
static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value * , FVM_Value *out, void *)
{ ++text_calls; return fvm_value_string(out, "23"); }
static FVM_Result failure(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK); FVM_Value arg, out, value; FVM_Object *plain = 0, *fn = 0;
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
        assert(fvm_function_create_native(c, primitive, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK);
        assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "toString", &value) == FVM_OK);
        const char *names[] = {"isNaN", "isFinite"};
        for (unsigned i = 0; i < 2; ++i) {
            assert(fvm_call(c, names[i], 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !!out.boolean == (i == 0));
            fvm_value_undefined(&arg); assert(fvm_call(c, names[i], 1, &arg, &out) == FVM_OK && !!out.boolean == ((version && version < 7) ? i != 0 : i == 0));
            arg = object(plain); primitive_calls = text_calls = 0; assert(fvm_call(c, names[i], 1, &arg, &out) == FVM_OK && !!out.boolean == (version >= 5 ? i == 0 : i != 0)); assert(primitive_calls == 1 && text_calls == (version ? 0U : 1U));
        }
        assert(fvm_function_create_native(c, failure, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, plain, "valueOf", &value) == FVM_OK); arg = object(plain);
        assert(fvm_call(c, "isNaN", 1, &arg, &out) == FVM_ERROR); assert(fvm_call(c, "isFinite", 1, &arg, &out) == FVM_ERROR);
        assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
