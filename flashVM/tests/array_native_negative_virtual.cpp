/* Negative native operations bypass getter/setter/watch callbacks and honor raw descriptor flags. */
#include "property_version_fixture.hpp"
#include "../src/standard/base_state.hpp"
static unsigned callback_count;
static FVM_Result signed_trap(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++callback_count; fvm_value_number(out, 55); return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"push", "unshift", "pop", "shift", "splice"};
    const char *names[] = {"-3", "-4", "0"};
    const double original[] = {71, 72, 81};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 5; ++method)
    for (unsigned readonly = 0; readonly < 2; ++readonly) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value value, args[3], out; fvm_value_number(&value, -3); FVM_Object *array = 0, *trap = 0;
        assert(fvm_construct(c, "Array", 1, &value, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        assert(fvm_function_create_native(c, signed_trap, 0, 0, &trap) == FVM_OK && fvm_gc_pin(c, trap) == FVM_OK);
        for (unsigned i = 0; i < 3; ++i) {
            fvm_value_number(&value, original[i]);
            if (i == 2) assert(fvm_array_store_own(c, array, 0, value) == FVM_OK);
            else assert(fvm_base_store_data(c, array, names[i], value, false) == FVM_OK);
            fvm_value_string(args, names[i]); args[1] = args[2] = class_value(trap);
            assert(fvm_object_call(c, array, "addProperty", 3, args, &out) == FVM_OK && out.boolean);
            assert(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean);
            version_flags(c, array, names[i], readonly ? 0x1006 : 0x1002, 65535);
        }
        fvm_value_string(args, "length"); args[1] = class_value(trap);
        assert(fvm_object_call(c, array, "watch", 2, args, &out) == FVM_OK && out.boolean);
        assert(fvm_gc_collect(c, 0) == FVM_OK); callback_count = 0;
        fvm_value_number(args, 10); unsigned argc = method < 2 ? 1 : 0;
        if (method == 4) { fvm_value_number(args, 0); fvm_value_number(args + 1, 0); fvm_value_number(args + 2, 10); argc = 3; }
        assert(fvm_object_call(c, array, methods[method], argc, args, &out) == FVM_OK && !callback_count);
        if (method == 4) assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &value) == FVM_OK && value.number == 0);
        else assert(out.type == FVM_VALUE_NUMBER && out.number == (method < 2 ? -2 : method == 2 ? 72 : 81));
        for (unsigned i = 0; i < 3; ++i) {
            assert(fvm_base_accessor_data(array->accessors.find(names[i])->second, value));
            const bool written = ((!method || method == 4) && !i) || (method == 1 && i == 2);
            assert(value.type == FVM_VALUE_NUMBER && value.number == (written && !readonly ? 10 : original[i]));
            unsigned flags = 0; assert(fvm_object_property_flags(c, array, names[i], &flags) == FVM_OK);
            assert(flags == (readonly ? 0x1006 : written ? 2 : 0x1002));
        }
        assert(fvm_object_get(c, array, "length", &out) == FVM_OK && out.number == (method < 2 || method == 4 ? -2 : -4));
        assert(!callback_count && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
