/* Loaded TypeOf classifies an escaped super facade as object without invoking its parent. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#include "flashvm/execution.h"
#include <cstring>
#ifdef FVM_SUPER_TYPEOF_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
static void super_typeof(FVM_Context *c, FVM_Object *facade, const char *expected)
{
    FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = facade;
#ifdef FVM_SUPER_TYPEOF_JIT
    assert(fvm_set_global(c, "typeTarget", &value) == FVM_OK);
    Program code; code.variable("typeTarget"); code.op(0x44); code.finish(); value = class_run(c, code);
#else
    assert(fvm_exec_type_of(c, &value, &value) == FVM_OK);
#endif
    assert(value.type == FVM_VALUE_STRING && !std::strcmp(value.string, expected));
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_SUPER_TYPEOF_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 5) continue;
#ifdef FVM_SUPER_TYPEOF_JIT
        if (!version) continue;
#endif
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0; super_accessor_setup(c, &instance, &target, &facade);
        super_typeof(c, facade, version ? "object" : "function");
        super_typeof(c, c->global->properties[fvm_detail::string("Object")].object, "function");
        super_typeof(c, instance, "object");
        FVM_Value marker; assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 9);
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        super_typeof(c, facade, version ? "object" : "function");
        assert(fvm_function_invoke(c, facade, instance, 0, 0, &marker) == FVM_OK);
        assert(fvm_object_get(c, instance, "marker", &marker) == FVM_OK && marker.number == 123);
        assert(!c->gc_frames && !c->gc_native_depth); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
