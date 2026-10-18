#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
#include <cmath>
static FVM_Result unexpected(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ assert(false); return FVM_ERROR; }
static void check(FVM_Context *c, const FVM_Value &arg, bool truth)
{
    FVM_Value out; FVM_Object *boxed = 0;
    assert(fvm_call(c, "Boolean", 1, &arg, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !!out.boolean == truth);
    assert(fvm_construct(c, "Boolean", 1, &arg, &boxed) == FVM_OK);
    assert(fvm_object_call(c, boxed, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !!out.boolean == truth);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value arg, out; FVM_Object *boxed = 0, *plain = 0, *fn = 0;
        assert(fvm_call(c, "Boolean", 0, 0, &out) == FVM_OK); assert(version ? out.type == FVM_VALUE_UNDEFINED : out.type == FVM_VALUE_BOOLEAN && !out.boolean);
        assert(fvm_construct(c, "Boolean", 0, 0, &boxed) == FVM_OK); assert(fvm_object_call(c, boxed, "valueOf", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_BOOLEAN && !out.boolean);
        const char *strings[] = {"", "0", "false", "word", "1", "-2", "010", "NaN"};
        for (unsigned i = 0; i < 8; ++i) { fvm_value_string(&arg, strings[i]); check(c, arg, version && version < 7 ? i >= 4 && i <= 6 : i != 0); }
        fvm_value_undefined(&arg); check(c, arg, false); fvm_value_undefined(&arg); arg.type = FVM_VALUE_NULL; check(c, arg, false);
        fvm_value_number(&arg, 0); check(c, arg, false); fvm_value_number(&arg, HUGE_VAL-HUGE_VAL); check(c, arg, false); fvm_value_number(&arg, HUGE_VAL); check(c, arg, true);
        assert(fvm_construct(c, "Object", 0, 0, &plain) == FVM_OK);
        assert(fvm_function_create_native(c, unexpected, 0, 0, &fn) == FVM_OK); arg = object(fn); assert(fvm_object_set(c, plain, "valueOf", &arg) == FVM_OK); assert(fvm_object_set(c, plain, "toString", &arg) == FVM_OK);
        arg = object(plain); check(c, arg, true); arg = object(boxed); check(c, arg, true);
        assert(fvm_gc_pin(c, boxed) == FVM_OK); assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); assert(fvm_object_call(c, boxed, "valueOf", 0, 0, &out) == FVM_OK && !out.boolean);
        assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
