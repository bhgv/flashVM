/* Loaded Math methods/constants are readonly/nonenumerable/nondeletable; host defaults remain. */
#include "math_attribute_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (unsigned before = 0; before < 2; ++before)
    for (unsigned field = 0; field < sizeof(math_fields) / sizeof(math_fields[0]); ++field)
    for (unsigned route = 0; route < 4; ++route) {
        if ((version && version < 4) || (!version && before) || (route >= 2 && !fvm_jit_available())) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        if (before) assert(fvm_standard_install(c) == FVM_OK);
        if (version) assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        if (!before) assert(fvm_standard_install(c) == FVM_OK);
        FVM_Value math, original, out, value; assert(fvm_get_global(c, "Math", &math) == FVM_OK && fvm_object_get(c, math.object, math_fields[field], &original) == FVM_OK);
        assert(fvm_set_global(c, "mathNamespace", &math) == FVM_OK);
        unsigned expected = version ? 7U : field < 8 ? 0U : 1U;
        math_flag(c, math.object, math_fields[field], expected); math_enumerable(c, math.object, math_fields[field], !(expected & 1U));
        fvm_value_number(&value, 81);
        if (!route) assert(fvm_object_set(c, math.object, math_fields[field], &value) == FVM_OK && fvm_object_get(c, math.object, math_fields[field], &out) == FVM_OK);
        else { class_mode = route - 1; Program code; code.variable("mathNamespace"); code.text(math_fields[field]); code.number(81); code.op(0x4f); code.variable("mathNamespace"); code.text(math_fields[field]); code.op(0x4e); code.finish(); out = class_run(c, code); }
        if (version) math_same(original, out); else assert(out.type == FVM_VALUE_NUMBER && out.number == 81);
        int removed = 0; assert(fvm_object_delete(c, math.object, math_fields[field], &removed) == FVM_OK && !!removed == !version);
        if (version && !route) {
            FVM_Context *other = 0; assert(fvm_context_create(rt, &other) == FVM_OK && fvm_load_memory(other, swf, sizeof(swf)) == FVM_OK);
            unsigned char *data = 0; size_t bytes = 0;
            assert(fvm_value_serialize(c, &math, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &out) == FVM_OK);
            fvm_serialized_free(data); FVM_Object *copy = out.object;
            assert(fvm_gc_pin(other, copy) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK);
            math_flag(other, copy, math_fields[field], 7); math_enumerable(other, copy, math_fields[field], false);
            assert(fvm_object_set(other, copy, math_fields[field], &value) == FVM_OK && fvm_object_get(other, copy, math_fields[field], &out) == FVM_OK && out.type == original.type);
            if (out.type == FVM_VALUE_NUMBER) assert(out.number == original.number);
            assert(fvm_object_delete(other, copy, math_fields[field], &removed) == FVM_OK && !removed);
            fvm_context_destroy(other);
        }
        assert(!c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
