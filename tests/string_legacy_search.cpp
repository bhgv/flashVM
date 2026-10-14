#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
static unsigned calls;
static bool fail;
static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; if (fail) return FVM_ERROR; fvm_value_number(out, 81); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, args[2], out; FVM_Object *str = 0, *label = 0, *obj = 0, *fn = 0;
        fvm_value_string(&value, "ABAB"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK);
        fvm_value_string(&value, "[type Object]"); assert(fvm_construct(c, "String", 1, &value, &label) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &obj) == FVM_OK); assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, obj, "toString", &value) == FVM_OK);
        const char *methods[] = {"indexOf","lastIndexOf"};
        for (unsigned backwards = 0; backwards < 2; ++backwards) {
            assert(fvm_object_call(c, str, methods[backwards], 0, 0, &out) == FVM_OK);
            assert(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_NUMBER && out.number == -1);
            fvm_value_undefined(args); assert(fvm_object_call(c, str, methods[backwards], 1, args, &out) == FVM_OK && out.number == (version && version < 7 ? backwards ? 4 : 0 : -1));
            args[0] = object(obj); calls = 0; fail = false; assert(fvm_object_call(c, label, methods[backwards], 1, args, &out) == FVM_OK && out.number == (version ? 0 : -1) && calls == 1);
            calls = 0; assert(borrow(c, str, methods[backwards], obj, 0, 0, &out) == FVM_OK && calls == 1);
            assert(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_NUMBER && out.number == -1);
            fail = true; calls = 0; assert(borrow(c, str, methods[backwards], obj, 0, 0, &out) == FVM_ERROR && calls == 1); fail = false;
        }
        fvm_value_string(args, "A"); fvm_value_number(args+1, -1); assert(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? -1 : 0));
        fvm_value_number(args+1, HUGE_VAL-HUGE_VAL); assert(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? 0 : 2));
        fvm_value_number(args+1, HUGE_VAL); assert(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == (version ? 0 : 2));
        fvm_value_undefined(args+1); assert(fvm_object_call(c, str, "lastIndexOf", 2, args, &out) == FVM_OK && out.number == 2);
        fvm_value_number(args+1, 4294967298.0); assert(fvm_object_call(c, str, "indexOf", 2, args, &out) == FVM_OK && out.number == (version ? 2 : -1));
        fvm_value_string(args, ""); fvm_value_number(args+1, 99); assert(fvm_object_call(c, str, "indexOf", 2, args, &out) == FVM_OK && out.number == (version ? -1 : 4));
        assert(c->gc_frames == 0 && c->string_depth == 0 && c->primitive_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
