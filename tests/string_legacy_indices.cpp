#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include <cmath>
static unsigned primitive_calls, text_calls;
static FVM_Result primitive(FVM_Context *, FVM_Object *self, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++primitive_calls; *out = object(self); return FVM_OK; }
static FVM_Result text(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++text_calls; fvm_value_number(out, 2); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, args[2], out, klass; FVM_Object *str = 0, *arg = 0, *fn = 0;
        fvm_value_string(&value, "ABCDEFGHIJKLMNOP"); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK);
        const double numbers[] = {4294967297.9, -4294967295.9, 2147483648.0, HUGE_VAL};
        const char *expected[] = {"B","B","","A"};
        for (unsigned i = 0; i < 4; ++i) {
            fvm_value_number(args, numbers[i]); assert(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK);
            assert(!std::strcmp(out.string, version ? expected[i] : ""));
        }
        fvm_value_string(args, "010"); assert(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK && !std::strcmp(out.string, version >= 6 ? "I" : "K"));
        assert(fvm_construct(c, "Object", 0, 0, &arg) == FVM_OK);
        assert(fvm_function_create_native(c, primitive, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "valueOf", &value) == FVM_OK);
        assert(fvm_function_create_native(c, text, 0, 0, &fn) == FVM_OK); value = object(fn); assert(fvm_object_set(c, arg, "toString", &value) == FVM_OK);
        args[0] = object(arg); primitive_calls = text_calls = 0; assert(fvm_object_call(c, str, "charAt", 1, args, &out) == FVM_OK);
        assert(!std::strcmp(out.string, version ? "A" : "C") && primitive_calls == 1 && text_calls == (version ? 0U : 1U));
        fvm_value_number(args, 99); assert(fvm_object_call(c, str, "charCodeAt", 1, args, &out) == FVM_OK && (version == 5 ? out.number == 0 : out.number != out.number));
        fvm_value_number(args, -1); assert(fvm_object_call(c, str, "charCodeAt", 1, args, &out) == FVM_OK && out.number != out.number);
        fvm_value_number(args, 0); fvm_value_number(args+1, -1); assert(fvm_object_call(c, str, "substr", 2, args, &out) == FVM_OK && !std::strcmp(out.string, version ? "ABCDEFGHIJKLMNO" : ""));
        fvm_value_number(args, 4294967297.0); fvm_value_number(args+1, 3); assert(fvm_object_call(c, str, "substring", 2, args, &out) == FVM_OK && !std::strcmp(out.string, version ? "BC" : "DEFGHIJKLMNOP"));
        const char *methods[] = {"slice","substr","substring"};
        for (unsigned i = 0; i < 3; ++i) {
            assert(fvm_object_call(c, str, methods[i], 0, 0, &out) == FVM_OK); assert(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_STRING);
            primitive_calls = text_calls = 0; assert(borrow(c, str, methods[i], arg, 0, 0, &out) == FVM_OK);
            assert(version ? out.type == FVM_VALUE_UNDEFINED && !text_calls : out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "2") && text_calls == 1);
            assert(!primitive_calls);
        }
        assert(fvm_get_global(c, "String", &klass) == FVM_OK); fvm_value_string(args, "010"); assert(fvm_object_call(c, klass.object, "fromCharCode", 1, args, &out) == FVM_OK);
        unsigned short unit = 0; size_t count = 0; assert(fvm_value_get_utf16(&out, &unit, 1, &count) == FVM_OK && count == 1 && unit == (version >= 6 ? 8 : 10));
        assert(c->gc_frames == 0 && c->avm1_execution_depth == 0 && c->primitive_depth == 0 && c->string_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
