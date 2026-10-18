/* Native member read/write does not invoke super descriptors as ordinary Functions. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 5; version <= 8; ++version) for (class_mode = 0; class_mode < 3; ++class_mode) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0; FVM_Value out, marker, value;
        super_accessor_setup(c, &instance, &target, &facade);
        value = class_value(target); assert(fvm_set_global(c, "caseTarget", &value) == FVM_OK);
        Program read; read.variable("caseTarget"); read.text("slot"); read.op(0x4e); read.finish();
        out = class_run(c, read); assert(out.type == FVM_VALUE_NUMBER && out.number == 9);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 9);
        Program write; write.variable("caseTarget"); write.text("slot"); write.number(81); write.op(0x4f); write.number(99); write.finish();
        out = class_run(c, write); assert(out.number == 99);
        assert(fvm_object_get(c, target, "slot", &out) == FVM_OK && out.number == 81);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 9);
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
