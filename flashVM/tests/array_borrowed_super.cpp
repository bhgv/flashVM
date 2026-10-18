/* Borrowed loaded Array methods must not publish raw indices or length on super. */
#define FVM_SUPER_ACCESSOR_FIXTURE_ONLY
#include "object_super_accessors.cpp"
static FVM_Value borrowed_super_value(FVM_Object *o)
{ FVM_Value value; fvm_value_undefined(&value); value.type = FVM_VALUE_OBJECT; value.object = o; return value; }
#ifdef FVM_ARRAY_SUPER_JIT
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#endif
static void borrowed_super_call(FVM_Context *c, FVM_Object *array, FVM_Object *facade, const char *method, unsigned n, FVM_Value *args, FVM_Value *out)
{
    FVM_Value function, passed[4]; assert(n < 4);
#ifdef FVM_ARRAY_SUPER_JIT
    assert(fvm_object_get(c, array, method, &function) == FVM_OK && fvm_set_global(c, "superArrayMethod", &function) == FVM_OK);
    FVM_Value value = borrowed_super_value(facade); assert(fvm_set_global(c, "superArrayReceiver", &value) == FVM_OK);
    Program call; for (unsigned i = n; i; --i) { assert(args[i - 1].type == FVM_VALUE_NUMBER); call.number(args[i - 1].number); }
    call.variable("superArrayReceiver"); call.number(n + 1); call.variable("superArrayMethod"); call.text("call"); call.op(0x52); call.finish();
    *out = class_run(c, call); return;
#endif
    assert(fvm_object_get(c, array, method, &function) == FVM_OK); passed[0] = borrowed_super_value(facade);
    for (unsigned i = 0; i < n; ++i) passed[i + 1] = args[i];
    assert(fvm_object_call(c, function.object, "call", n + 1, passed, out) == FVM_OK);
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
#ifdef FVM_ARRAY_SUPER_JIT
    if (!fvm_jit_available()) { fvm_runtime_destroy(rt); return 0; }
    for (class_mode = 0; class_mode < 3; ++class_mode)
#endif
    for (unsigned version = 5; version <= 8; ++version) {
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version;
        assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK && fvm_standard_install(c) == FVM_OK);
        FVM_Object *instance = 0, *target = 0, *facade = 0, *array = 0; super_accessor_setup(c, &instance, &target, &facade);
        assert(fvm_construct(c, "Array", 0, 0, &array) == FVM_OK && fvm_gc_pin(c, array) == FVM_OK);
        size_t properties = facade->properties.size(), indices = facade->indexed.size(), accessors = facade->accessors.size();
        FVM_Value args[3], out; fvm_value_number(args, 10); fvm_value_number(args + 1, 20);
        const char *methods[] = {"push", "unshift", "splice", "slice", "pop", "shift", "reverse"};
        for (unsigned i = 0; i < sizeof(methods) / sizeof(methods[0]); ++i) {
            unsigned n = i < 2 ? 2 : i == 2 ? 3 : 0;
            if (i == 2) { fvm_value_number(args, 0); fvm_value_number(args + 1, 0); fvm_value_number(args + 2, 30); }
            borrowed_super_call(c, array, facade, methods[i], n, args, &out);
            if (i < 2) assert(out.type == FVM_VALUE_NUMBER && out.number == 2);
            if (i == 2 || i == 3) { FVM_Value length; assert(out.type == FVM_VALUE_OBJECT && fvm_object_get(c, out.object, "length", &length) == FVM_OK && length.number == 0); }
            if (i == 4 || i == 5) assert(out.type == FVM_VALUE_UNDEFINED);
            assert(facade->properties.size() == properties && facade->indexed.size() == indices && facade->accessors.size() == accessors);
            assert(fvm_gc_collect(c, 0) == FVM_OK && !c->gc_frames && !c->gc_native_depth);
            assert(fvm_object_get(c, instance, "marker", &out) == FVM_OK && out.number == 9);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
