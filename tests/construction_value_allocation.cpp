/* Native construction snapshots counted string results and unwinds every allocation failure. */
#define main native_instance_fixture_main
#include "native_instance_allocation.cpp"
#undef main
#include "../src/standard/construction.hpp"
#include <cstring>
static char constructor_bytes[] = {'a', 0, 'b'};
static FVM_Result constructor_string(FVM_Context *, FVM_Object *, unsigned count,
                                      const FVM_Value *args, FVM_Value *out, void *)
{
    assert(count == 1 && args[0].type == FVM_VALUE_NUMBER && args[0].number == 37);
    return fvm_value_string_bytes(out, constructor_bytes, sizeof(constructor_bytes));
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    {
        bool failed = false, success = false;
        for (long boundary = 0; boundary < 200 && !success; ++boundary)
        {
            FVM_Context *c = 0;
            assert(fvm_context_create(rt, &c) == FVM_OK);
            unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
            FVM_Object *function = 0;
            assert(fvm_function_create_native(c, constructor_string, 0, 0, &function) == FVM_OK && fvm_gc_pin(c, function) == FVM_OK);
            function->constructable = function->invocable;
            FVM_Value input, out; fvm_value_number(&input, 37); fvm_value_number(&out, 91);
            constructor_bytes[0] = 'a';
            allowance = boundary;
            FVM_Result status = fvm_function_construct_value(c, function, 1, &input, &out);
            allowance = -1;
            assert(!c->gc_frames && !c->gc_native_depth && !c->calling_function && !c->calling_constructor);
            if (status == FVM_OUT_OF_MEMORY)
            {
                failed = true; assert(out.type == FVM_VALUE_UNDEFINED && fvm_gc_collect(c, 0) == FVM_OK);
                assert(fvm_function_construct_value(c, function, 1, &input, &out) == FVM_OK);
            }
            else { assert(status == FVM_OK); success = true; }
            constructor_bytes[0] = 'z';
            assert(out.type == FVM_VALUE_STRING && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
            fvm_context_destroy(c);
        }
        assert(failed && success);
    }
    fvm_runtime_destroy(rt);
}
