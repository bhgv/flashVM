#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static unsigned calls, mode;
static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    ++calls;
    if (mode == 2) return FVM_ERROR;
    if (mode == 1) return fvm_value_string_bytes(out, "a\0b", 3);
    fvm_value_number(out, 81); return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value args[3], out, value; FVM_Object *obj = 0, *fn = 0, *box = 0, *str = 0, *holder = 0;
        assert(fvm_construct(c, "Object", 0, 0, &obj) == FVM_OK); assert(fvm_gc_pin(c, obj) == FVM_OK);
        assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, obj, "toString", &value) == FVM_OK);
        const char *expected[] = {version && version < 7 ? "" : "undefined", version == 4 ? "1" : "true", version ? "[type Object]" : "81"};
        for (unsigned i = 0; i < 3; ++i) {
            if (!i) fvm_value_undefined(args); else if (i == 1) fvm_value_boolean(args, 1); else args[0] = object(obj);
            calls = mode = 0; assert(fvm_call(c, "String", 1, args, &out) == FVM_OK && !std::strcmp(out.string, expected[i])); assert(calls == (i == 2 ? 1U : 0U));
            calls = 0; assert(fvm_construct(c, "String", 1, args, &box) == FVM_OK && calls == (i == 2 ? 1U : 0U));
            assert(fvm_object_call(c, box, "valueOf", 0, 0, &out) == FVM_OK && !std::strcmp(out.string, expected[i]));
        }
        assert(fvm_call(c, "String", 0, 0, &out) == FVM_OK && !std::strcmp(out.string, ""));
        assert(fvm_construct(c, "String", 0, 0, &box) == FVM_OK); assert(fvm_object_call(c, box, "valueOf", 0, 0, &out) == FVM_OK && !std::strcmp(out.string, ""));
        fvm_value_string(&value, "prefix:"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK); assert(fvm_gc_pin(c, str) == FVM_OK);
        fvm_value_undefined(args); fvm_value_boolean(args+1, 1); args[2] = object(obj); calls = 0;
        const char *joined = !version ? "prefix:undefinedtrue81" : version == 4 ? "prefix:1[type Object]" : version < 7 ? "prefix:true[type Object]" : "prefix:undefinedtrue[type Object]";
        assert(fvm_object_call(c, str, "concat", 3, args, &out) == FVM_OK && !std::strcmp(out.string, joined) && calls == 1);
        mode = 1; args[0] = object(obj); calls = 0; assert(fvm_object_call(c, str, "concat", 1, args, &out) == FVM_OK && calls == 1 && fvm_value_string_size(&out) == 10 && !std::memcmp(out.string, "prefix:a\0b", 10));
        assert(fvm_object_create(c, &holder) == FVM_OK); assert(fvm_gc_pin(c, holder) == FVM_OK); assert(fvm_object_set(c, holder, "joined", &out) == FVM_OK);
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(fvm_object_get(c, holder, "joined", &out) == FVM_OK && fvm_value_string_size(&out) == 10 && !std::memcmp(out.string, "prefix:a\0b", 10));
        mode = 2; assert(fvm_call(c, "String", 1, args, &out) == FVM_ERROR); assert(fvm_construct(c, "String", 1, args, &box) == FVM_ERROR); assert(fvm_object_call(c, str, "concat", 1, args, &out) == FVM_ERROR);
        assert(c->gc_frames == 0 && c->string_depth == 0 && c->primitive_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
