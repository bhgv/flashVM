/* A loaded super facade uses Object fallback when parent toString returns no string. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#include "flashvm/execution.h"
#include "flashvm/exception.h"
#include <cstring>
static unsigned string_mode, string_calls;
static FVM_Result super_string_result(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++string_calls;
    switch (string_mode) {
    case 0: fvm_value_number(out, 17); return FVM_OK;
    case 1: fvm_value_undefined(out); return FVM_OK;
    case 2: fvm_value_undefined(out); out->type = FVM_VALUE_NULL; return FVM_OK;
    case 3: return fvm_value_string_bytes(out, "a\0b", 3);
    default: fvm_value_number(out, 71); return fvm_exception_raise(c, out);
    }
}
#ifdef FVM_SUPER_STRING_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
static FVM_Result super_string_convert(FVM_Context *c, FVM_Object *facade, FVM_Value *out)
{
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = facade;
#ifdef FVM_SUPER_STRING_JIT
    assert(fvm_set_global(c, "stringTarget", &value) == FVM_OK);
    Program code; code.variable("stringTarget"); code.op(0x4b); code.finish();
    FVM_Result status = string_mode == 4 ? FVM_EXCEPTION : FVM_OK;
    *out = class_run(c, code, status); return status;
#else
    *out = value; return fvm_exec_to_string(c, out, out);
#endif
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_SUPER_STRING_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 5; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0, *method = 0; super_accessor_setup(c, &instance, &target, &facade);
        FVM_Value base, value; assert(fvm_get_global(c, "Base", &base) == FVM_OK);
        const unsigned char end[] = {0}; assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &method) == FVM_OK && fvm_function_set_body(c, method, super_string_result, 0, 0) == FVM_OK);
        fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = method;
        assert(fvm_object_set(c, prototype(c, base), "toString", &value) == FVM_OK);
        for (string_mode = 0; string_mode < 5; ++string_mode) {
            string_calls = 0; FVM_Result status = super_string_convert(c, facade, &value);
            assert(string_calls == 1 && status == (string_mode == 4 ? FVM_EXCEPTION : FVM_OK));
            if (string_mode == 4) {
#ifndef FVM_SUPER_STRING_JIT
                assert(fvm_exception_get(c, &value) == FVM_OK && value.number == 71 && fvm_exception_clear(c) == FVM_OK);
#endif
            } else if (string_mode == 3) assert(value.type == FVM_VALUE_STRING && fvm_value_string_size(&value) == 3 && !std::memcmp(value.string, "a\0b", 3));
            else assert(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, "[type Object]"));
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->string_depth);
        }
        string_mode = 0; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = method;
        assert(fvm_object_set(c, base.object, "toString", &value) == FVM_OK);
        assert(super_string_convert(c, base.object, &value) == FVM_OK && !std::strcmp(value.string, "[type Function]"));
        assert(super_string_convert(c, instance, &value) == FVM_OK && !std::strcmp(value.string, "[type Object]"));
        assert(fvm_object_get(c, instance, "marker", &value) == FVM_OK && value.number == 9);
        assert(fvm_function_invoke(c, facade, instance, 0, 0, &value) == FVM_OK);
        assert(fvm_object_get(c, instance, "marker", &value) == FVM_OK && value.number == 123);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
