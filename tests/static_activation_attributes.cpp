/* Replaced prototypes cannot suppress untouched Number constants or Date.UTC protection. */
#include "static_activation_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned type = 0; type < 2; ++type)
    for (unsigned variant = 0; variant < 7; ++variant)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *klass = static_class(c, type); static_prototype(c, klass, variant); static_flags(c, klass, type, type ? 1U : 0U);
        static_reads = 0; static_load(c, version); assert(!static_reads); static_flags(c, klass, type, 7);
        const unsigned begin = type ? 5 : 0, end = type ? 6 : 5;
        FVM_Value target = class_value(klass), out; assert(fvm_set_global(c, "staticTarget", &target) == FVM_OK);
        for (unsigned field = begin; field < end; ++field) {
            FVM_Value original, value; assert(fvm_object_get(c, klass, static_fields[field], &original) == FVM_OK);
            if (!route) { fvm_value_number(&value, 81); assert(fvm_object_set(c, klass, static_fields[field], &value) == FVM_OK && fvm_object_get(c, klass, static_fields[field], &out) == FVM_OK); }
            else { class_mode = route - 1; Program code; code.variable("staticTarget"); code.text(static_fields[field]); code.number(81); code.op(0x4f); code.variable("staticTarget"); code.text(static_fields[field]); code.op(0x4e); code.finish(); out = class_run(c, code); }
            assert(out.type == original.type);
            if (out.type == FVM_VALUE_NUMBER) assert(out.number == original.number || (out.number != out.number && original.number != original.number)); else assert(out.object == original.object);
            int removed = 1; assert(fvm_object_delete(c, klass, static_fields[field], &removed) == FVM_OK && !removed);
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK); static_flags(c, klass, type, 7);
        if (!route && variant != 5) {
            FVM_Context *other = 0; unsigned char *data = 0; size_t bytes = 0;
            assert(fvm_context_create(rt, &other) == FVM_OK && fvm_value_serialize(c, &target, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
            assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK); static_flags(other, out.object, type, 7); fvm_context_destroy(other);
        }
        /* Explicitly clearing protection is a live edit, preserved on reload. */
        for (unsigned field = begin; field < end; ++field) assert(fvm_object_set_property_flags(c, klass, static_fields[field], 0, 7) == FVM_OK);
        static_load(c, version); static_flags(c, klass, type, 0); assert(!static_reads && !c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
