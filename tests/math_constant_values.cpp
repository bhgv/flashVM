/* Constants use canonical binary64 values in C, AVM1/JIT and graph transfer. */
#include "math_constant_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned before = 0; before < 2; ++before) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (before) { assert(fvm_standard_install(c) == FVM_OK); FVM_Value host, value; assert(fvm_get_global(c, "Math", &host) == FVM_OK && fvm_object_get(c, host.object, "LOG10E", &value) == FVM_OK && value.number == 1 / std::log(10.0)); }
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        if (!before) assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value math, out; assert(fvm_get_global(c, "Math", &math) == FVM_OK && fvm_set_global(c, "mathNamespace", &math) == FVM_OK);
        math_constant_namespace(c, math.object);
        for (unsigned mode = 0; mode < 3; ++mode) {
            if (mode && !fvm_jit_available()) continue;
            class_mode = mode;
            for (unsigned i = 0; i < 8; ++i) { Program code; code.variable("mathNamespace"); code.text(math_fields[i]); code.op(0x4e); code.finish(); out = class_run(c, code); assert(out.type == FVM_VALUE_NUMBER); math_constant_check(i, out.number); }
        }
        FVM_Context *other = 0; unsigned char *data = 0; size_t bytes = 0;
        assert(fvm_context_create(rt, &other) == FVM_OK && fvm_value_serialize(c, &math, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
        assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK); math_constant_namespace(other, out.object); fvm_context_destroy(other);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
