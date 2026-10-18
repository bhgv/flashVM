/* Function and Function2 metadata arrays bypass the public Array prototype getter. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "../src/standard/function_state.hpp"
#include "flashvm/property.h"
#include "flashvm/table.h"

static unsigned metadata_traps;
static FVM_Result metadata_trap(FVM_Context *, FVM_Object *, unsigned,
                                const FVM_Value *, FVM_Value *, void *)
{
    ++metadata_traps;
    return FVM_ERROR;
}

int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned function2 = 0; function2 < 2; ++function2)
    {
        if (version && version < 4)
            continue;
        FVM_Context *c = 0;
        assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version)
        {
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
            swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value klass;
        assert(fvm_get_global(c, "Array", &klass) == FVM_OK);
        int removed = 0;
        assert(fvm_object_set_property_flags(c, klass.object, "prototype", 0, FVM_PROPERTY_VALID_FLAGS) == FVM_OK &&
               fvm_object_delete(c, klass.object, "prototype", &removed) == FVM_OK && removed &&
               fvm_object_define_accessor_rooted(c, klass.object, "prototype", metadata_trap, 0, 0, 0) == FVM_OK);
        metadata_traps = 0;
        Program code;
        if (function2)
        {
            const unsigned char push_register[] = {0x96,2,0,4,7};
            for (unsigned i = 0; i < sizeof(push_register); ++i)
                code.op(push_register[i]);
        }
        else
            code.variable("x");
        code.finish();
        FVM_ScriptFunction definition;
        definition.body = code.bytes;
        definition.function2 = function2 != 0;
        definition.register_count = function2 ? 8 : 0;
        definition.parameters.push_back(fvm_detail::string("x"));
        definition.constants.push_back(fvm_detail::string("pool-marker"));
        if (function2)
            definition.parameter_registers.push_back(7);
        FVM_Object *function = 0;
        assert(fvm_function_make_script(c, definition, 0, &function) == FVM_OK &&
               fvm_gc_pin(c, function) == FVM_OK && !metadata_traps);
        FVM_Object *data = function->resource_result.object;
        const char *names[] = {"parameters", "constants", "parameter_registers"};
        for (unsigned i = 0; i < (function2 ? 3U : 2U); ++i)
        {
            FVM_Value array, value;
            assert(fvm_object_get(c, data, names[i], &array) == FVM_OK &&
                   array.type == FVM_VALUE_OBJECT && !array.object->prototype);
            assert(fvm_object_get(c, array.object, "length", &value) == FVM_OK && value.number == 1);
            assert(fvm_table_get_index(c, array.object, 0, &value) == FVM_OK);
            if (i == 2)
                assert(value.type == FVM_VALUE_NUMBER && value.number == 7);
            else
                assert(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, i ? "pool-marker" : "x"));
        }
        FVM_Value input, out;
        fvm_value_number(&input, 42);
        FVM_JitCode *native = 0;
        if (fvm_jit_available())
            assert(fvm_jit_compile(code.bytes.begin(), code.bytes.size(), &native) == FVM_OK);
        for (unsigned mode = 0; mode < (native ? 3U : 1U); ++mode)
        {
            assert(fvm_execution_set_mode(c, mode == 2 ? FVM_EXECUTION_JIT : FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
                   fvm_function_set_body(c, function, mode == 1 ? fvm_jit_body : 0, mode == 1 ? native : 0, 0) == FVM_OK);
            unsigned long before = c->execution_mode.stats.native_entries;
            assert(fvm_function_invoke(c, function, 0, 1, &input, &out) == FVM_OK &&
                   out.type == FVM_VALUE_NUMBER && out.number == 42 && !metadata_traps &&
                   c->execution_mode.stats.native_entries == before + (mode != 0));
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        }
        assert(fvm_execution_set_mode(c, FVM_EXECUTION_MANUAL, 0) == FVM_OK &&
               fvm_function_set_body(c, function, 0, 0, 0) == FVM_OK);
        if (native)
            assert(fvm_jit_destroy(native) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);
    return 0;
}
