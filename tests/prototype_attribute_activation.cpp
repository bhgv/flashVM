/* Editing one prototype method leaves the other native defaults eligible for SWF flags. */
#include "prototype_attribute_fixture.hpp"
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 4; version <= 8; ++version)
    for (unsigned type = 0; type < 4; ++type)
    for (unsigned anchor = 0; anchor < 6; ++anchor)
    for (unsigned peer = 0; peer < 6; ++peer)
    for (unsigned route = 0; route < 4; ++route) {
        if (route >= 2 && !fvm_jit_available()) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *proto = prototype_target(c, type); prototype_edit(c, proto, prototype_anchors[type], anchor); prototype_edit(c, proto, prototype_peers[type], peer);
        static_reads = 0; static_load(c, version); assert(!static_reads);
        math_flag(c, proto, prototype_anchors[type], prototype_expected(anchor)); math_flag(c, proto, prototype_peers[type], prototype_expected(peer));
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        if (!route && anchor != 3 && peer != 3) {
            FVM_Context *other = 0; FVM_Value value = class_value(proto), out; unsigned char *data = 0; size_t bytes = 0;
            assert(fvm_context_create(rt, &other) == FVM_OK && fvm_value_serialize(c, &value, &data, &bytes) == FVM_OK && fvm_value_deserialize(other, data, bytes, &out) == FVM_OK); fvm_serialized_free(data);
            assert(fvm_gc_pin(other, out.object) == FVM_OK && fvm_gc_collect(other, 0) == FVM_OK); math_flag(other, out.object, prototype_anchors[type], prototype_expected(anchor)); math_flag(other, out.object, prototype_peers[type], prototype_expected(peer)); fvm_context_destroy(other);
        }
        if (!peer) {
            FVM_Value target = class_value(proto), out; assert(fvm_set_global(c, "prototypeTarget", &target) == FVM_OK);
            if (!route) { fvm_value_number(&out, 81); assert(fvm_object_set(c, proto, prototype_peers[type], &out) == FVM_OK && fvm_object_get(c, proto, prototype_peers[type], &out) == FVM_OK); }
            else { class_mode = route - 1; Program code; code.variable("prototypeTarget"); code.text(prototype_peers[type]); code.number(81); code.op(0x4f); code.variable("prototypeTarget"); code.text(prototype_peers[type]); code.op(0x4e); code.finish(); out = class_run(c, code); }
            assert(out.type == FVM_VALUE_NUMBER && out.number == 81); int removed = 1; assert(fvm_object_delete(c, proto, prototype_peers[type], &removed) == FVM_OK && !removed);
        }
        static_load(c, version); math_flag(c, proto, prototype_anchors[type], prototype_expected(anchor)); math_flag(c, proto, prototype_peers[type], prototype_expected(peer));
        assert(!static_reads && !c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
