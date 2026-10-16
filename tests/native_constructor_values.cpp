/* Loaded NewObject/NewMethod preserve native values without weakening public Object construction. */
#define main native_constructor_reference_main
#include "function_construct_native_result.cpp"
#undef main
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/execution.h"

int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    {
        FVM_Context *c = 0, *other = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        assert(fvm_object_create(c, &factory_local) == FVM_OK && fvm_object_create(other, &factory_foreign) == FVM_OK);
        FVM_Object *function = 0, *holder = 0;
        assert(fvm_function_create_native(c, factory_result, factory_local, factory_local, &function) == FVM_OK);
        function->constructable = function->invocable;
        FVM_Value value = class_value(function);
        assert(fvm_set_global(c, "NativeCtor", &value) == FVM_OK && fvm_object_create(c, &holder) == FVM_OK && fvm_object_set(c, holder, "fn", &value) == FVM_OK);
        value = class_value(holder); assert(fvm_set_global(c, "holder", &value) == FVM_OK);
        for (factory_result_mode = 0; factory_result_mode < 7; ++factory_result_mode)
        for (unsigned route = 0; route < 3; ++route)
        for (unsigned mode = 0; mode < 3; ++mode)
        {
            if (!fvm_jit_available()) continue;
            class_mode = mode;
            Program code; code.number(0);
            if (!route) { code.text("NativeCtor"); code.op(0x40); }
            else { code.variable(route == 1 ? "holder" : "NativeCtor"); code.text(route == 1 ? "fn" : ""); code.op(0x53); }
            code.finish();
            FVM_Result expected = factory_result_mode == 2 || factory_result_mode == 3 ? FVM_INVALID_ARGUMENT : factory_result_mode == 5 ? FVM_ERROR : factory_result_mode == 6 ? FVM_EXCEPTION : FVM_OK;
            FVM_Value out = class_run(c, code, expected);
            if (!factory_result_mode) assert(out.type == FVM_VALUE_UNDEFINED);
            if (factory_result_mode == 1) assert(out.type == FVM_VALUE_NUMBER && out.number == 7);
            if (factory_result_mode == 4) assert(out.type == FVM_VALUE_OBJECT && out.object == factory_local);
            assert(fvm_gc_collect(c, 0) == FVM_OK);
        }
        fvm_context_destroy(c); fvm_context_destroy(other);
    }
    fvm_runtime_destroy(rt);
}
