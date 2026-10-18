#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
static unsigned converted;
static bool fail_limit, fail_separator;
static FVM_Result limit(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ assert(!converted++); if (fail_limit) return FVM_ERROR; fvm_value_number(out, 0); return FVM_OK; }
static FVM_Result separator(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ assert(converted++ == 1); if (fail_separator) return FVM_ERROR; return fvm_value_string(out, ","); }
static FVM_Result numeric_separator(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ fvm_value_number(out, 81); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *str = 0; FVM_Value value, args[2], out;
        fvm_value_string(&value, "A,B,C"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK);
        assert(fvm_object_call(c, str, "split", 0, 0, &out) == FVM_OK); number(c, out.object, "length", version == 5 ? 3 : 1);
        fvm_value_string(args, ","); fvm_value_number(args+1, -1); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", version ? 0 : 3);
        fvm_value_string(args, ""); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", version == 5 ? 1 : version ? 0 : 5);
        fvm_value_undefined(args); fvm_value_number(args+1, 0); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", version && version != 5 ? 1 : 0);
        fvm_value_string(args, ","); fvm_value_number(args+1, 2147483648.0); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", version ? 0 : 3);
        fvm_value_number(args+1, 4294967297.0); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", 1); assert(fvm_table_get_index(c, out.object, 0, &value) == FVM_OK && !std::strcmp(value.string, "A"));
        fvm_value_number(args+1, HUGE_VAL); assert(fvm_object_call(c, str, "split", 2, args, &out) == FVM_OK); number(c, out.object, "length", 0);
        fvm_value_string(&value, "1,true,1"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK); fvm_value_boolean(args, 1);
        assert(fvm_object_call(c, str, "split", 1, args, &out) == FVM_OK); number(c, out.object, "length", version == 4 ? 3 : 2);
        FVM_Object *delimiter = 0, *limit_obj = 0, *fn = 0; assert(fvm_construct(c, "Object", 0, 0, &delimiter) == FVM_OK);
        assert(fvm_function_create_native(c, numeric_separator, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, delimiter, "toString", &value) == FVM_OK);
        fvm_value_string(&value, "a[type Object]b"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK); args[0] = object(delimiter);
        assert(fvm_object_call(c, str, "split", 1, args, &out) == FVM_OK); number(c, out.object, "length", version ? 2 : 1);
        assert(fvm_construct(c, "Object", 0, 0, &limit_obj) == FVM_OK); assert(fvm_function_create_native(c, limit, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, limit_obj, "valueOf", &value) == FVM_OK);
        assert(fvm_function_create_native(c, separator, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, delimiter, "toString", &value) == FVM_OK); args[0] = object(delimiter); args[1] = object(limit_obj);
        for (unsigned mode = 0; mode < 3; ++mode) {
            converted = 0; fail_limit = mode == 1; fail_separator = mode == 2;
            assert(fvm_object_call(c, str, "split", 2, args, &out) == (mode ? FVM_ERROR : FVM_OK));
            assert(converted == (mode == 1 ? 1U : 2U)); if (!mode) number(c, out.object, "length", 0);
            assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
