/* Raw Array operations retain hidden data and clear requirements only on overwritten slots. */
#include "property_version_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    const char *methods[] = {"sort", "reverse", "shift", "unshift", "push", "pop", "splice"};
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned method = 0; method < 7; ++method) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK); version_load(c, version); assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value values[3], args[3], out; fvm_value_number(values, 3); fvm_value_number(values+1, 1); fvm_value_number(values+2, 2); FVM_Object *array = 0; assert(fvm_construct(c, "Array", 3, values, &array) == FVM_OK);
        version_flags(c, array, "0,length", 0x1000, 0); assert(fvm_object_call(c, array, "join", 0, 0, &out) == FVM_OK && out.type == FVM_VALUE_STRING && !std::strcmp(out.string, "3,1,2"));
        fvm_value_number(args, method == 6 ? 0 : 42); fvm_value_number(args+1, 1); fvm_value_number(args+2, 42);
        assert(fvm_object_call(c, array, methods[method], method == 6 ? 3 : method == 3 || method == 4 ? 1 : 0, args, &out) == FVM_OK);
        unsigned flags = 0; assert(fvm_object_property_flags(c, array, "0", &flags) == FVM_OK && flags == (method == 4 || method == 5 ? 0x1000U : 0U));
        assert(fvm_object_property_flags(c, array, "length", &flags) == FVM_OK && flags == (method < 2 ? 0x1003U : 3U));
        assert(fvm_gc_pin(c, array) == FVM_OK && fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
