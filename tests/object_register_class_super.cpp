/* Loaded registerClass rejects super before coercing its name or replacing a constructor. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
#include "flashvm/exception.h"
static unsigned registration_names, registration_fault;
static FVM_Result registration_name(FVM_Context *c, const FVM_ScriptFrame *, FVM_Value *out, void *)
{
    ++registration_names;
    if (registration_fault) { fvm_value_number(out, 71); return fvm_exception_raise(c, out); }
    return fvm_value_string_bytes(out, "Class\0Name", 10);
}
static FVM_Value registration_object(FVM_Object *o)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = o; return value; }
#ifdef FVM_REGISTER_SUPER_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
static FVM_Result registration_call(FVM_Context *c, FVM_Object *klass, FVM_Value *args, FVM_Value *out)
{
#ifdef FVM_REGISTER_SUPER_JIT
    FVM_Value value = registration_object(klass); assert(fvm_set_global(c, "registrationObject", &value) == FVM_OK);
    assert(fvm_set_global(c, "registrationName", args) == FVM_OK && fvm_set_global(c, "registrationConstructor", args + 1) == FVM_OK);
    Program call; call.variable("registrationConstructor"); call.variable("registrationName"); call.number(2); call.variable("registrationObject"); call.text("registerClass"); call.op(0x52); call.finish();
    *out = class_run(c, call); return FVM_OK;
#else
    return fvm_object_call(c, klass, "registerClass", 2, args, out);
#endif
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_REGISTER_SUPER_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 5) continue;
#ifdef FVM_REGISTER_SUPER_JIT
        if (!version) continue;
#endif
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0, *key = 0, *name_fn = 0, *found = 0; super_accessor_setup(c, &instance, &target, &facade);
        FVM_Value klass, args[2], out; assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
        fvm_value_string_bytes(args, "Class\0Name", 10); args[1] = klass;
        assert(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean);
        if (version) {
            const unsigned char end[] = {0};
            assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK && fvm_gc_pin(c, key) == FVM_OK);
            assert(fvm_function_create_avm1(c, end, sizeof(end), 0, 0, 0, &name_fn) == FVM_OK && fvm_function_set_body(c, name_fn, registration_name, 0, 0) == FVM_OK);
            FVM_Value value = registration_object(name_fn); assert(fvm_object_set(c, key, "toString", &value) == FVM_OK);
            args[0] = registration_object(key);
        }
        registration_names = registration_fault = 0; args[1] = registration_object(facade);
        assert(registration_call(c, klass.object, args, &out) == FVM_OK && !!out.boolean == !version && registration_names == 0);
        assert(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_OK && found == (version ? klass.object : facade));
        if (version) {
            registration_fault = 1;
            assert(registration_call(c, klass.object, args, &out) == FVM_OK && !out.boolean && !registration_names && !c->exception_pending);
            args[1] = klass;
            assert(fvm_object_call(c, klass.object, "registerClass", 2, args, &out) == FVM_EXCEPTION && registration_names == 1);
            assert(fvm_exception_get(c, &out) == FVM_OK && out.number == 71 && fvm_exception_clear(c) == FVM_OK);
            assert(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_OK && found == klass.object);
            registration_fault = 0;
            assert(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean && registration_names == 2);
        }
        fvm_value_undefined(args + 1); args[1].type = FVM_VALUE_NULL;
        assert(registration_call(c, klass.object, args, &out) == FVM_OK && out.boolean);
        assert(fvm_registered_class_bytes(c, "Class\0Name", 10, &found) == FVM_NOT_FOUND);
        assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->calling_setter_owner);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
