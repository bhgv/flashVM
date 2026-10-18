/* Property-name queries coerce once, apply SWF undefined rules and return false without arguments. */
#include "flashvm/standard.h"
#include "flashvm/function.h"
#include "flashvm/property.h"
#include "flashvm/gc.h"
#include <cassert>
static unsigned calls, mode;
static FVM_Result name(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; return mode ? fvm_value_string_bytes(out, "n\0m", 3) : fvm_value_string(out, calls == 1 ? "x" : "hidden"); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        FVM_Object *o = 0, *key = 0; FVM_Value value, klass, proto, arg, out;
        assert(fvm_object_create(c, &o) == FVM_OK); assert(fvm_gc_pin(c, o) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, o, "", &value) == FVM_OK); assert(fvm_object_set(c, o, "undefined", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "undefined", 1, 0) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK); assert(fvm_get_global(c, "Object", &klass) == FVM_OK);
        assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK); assert(fvm_object_set_prototype(c, o, proto.object) == FVM_OK);
        assert(fvm_object_set(c, o, "x", &value) == FVM_OK); assert(fvm_object_set(c, o, "hidden", &value) == FVM_OK);
        assert(fvm_object_set_property_flags(c, o, "hidden", 1, 0) == FVM_OK);
        assert(fvm_object_set_bytes(c, o, "n\0m", 3, &value) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK); assert(fvm_gc_pin(c, key) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, key, "toString", name, 0, 0) == FVM_OK);
        fvm_value_undefined(&arg); arg.type = FVM_VALUE_OBJECT; arg.object = key;
        const char *methods[] = {"hasOwnProperty", "propertyIsEnumerable", "isPropertyEnumerable"};
        for (unsigned m = 0; m < 3; ++m) {
            calls = mode = 0; assert(fvm_object_call(c, o, methods[m], 1, &arg, &out) == FVM_OK && out.boolean && calls == 1);
            mode = 1; calls = 0; assert(fvm_object_call(c, o, methods[m], 1, &arg, &out) == FVM_OK && out.boolean && calls == 1);
            assert(fvm_object_call(c, o, methods[m], 0, 0, &out) == FVM_OK && !out.boolean);
            FVM_Value undefined; fvm_value_undefined(&undefined);
            assert(fvm_object_call(c, o, methods[m], 1, &undefined, &out) == FVM_OK);
            assert(out.boolean == (m == 0 || (version && version < 7)));
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
