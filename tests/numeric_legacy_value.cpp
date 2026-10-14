#define FVM_ARRAY_LEGACY_FIXTURE_ONLY
#include "array_legacy_join.cpp"
#include "flashvm/gc.h"
static FVM_Result unexpected(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *, void *)
{ assert(false); return FVM_ERROR; }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *fn = 0, *radix = 0; FVM_Value value, arg, out;
        assert(fvm_function_create_native(c, unexpected, 0, 0, &fn) == FVM_OK); assert(fvm_gc_pin(c, fn) == FVM_OK);
        assert(fvm_construct(c, "Object", 0, 0, &radix) == FVM_OK); value = object(fn); assert(fvm_object_set(c, radix, "valueOf", &value) == FVM_OK); arg = object(radix);
        const char *classes[] = {"Number","Boolean"}, *methods[] = {"valueOf","toString"};
        for (unsigned cls = 0; cls < 2; ++cls) {
            FVM_Value klass, proto; assert(fvm_get_global(c, classes[cls], &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &proto) == FVM_OK);
            for (unsigned method = 0; method < 2; ++method) for (unsigned kind = 0; kind < 5; ++kind) {
                FVM_Object *receiver = 0;
                if (kind == 4) receiver = fn;
                else { fvm_value_number(&value, 1); assert(fvm_construct(c, kind == 0 ? "Object" : kind == 1 ? "String" : kind == 2 ? classes[1-cls] : "Array", kind ? 1 : 0, kind ? &value : 0, &receiver) == FVM_OK); }
                value = object(fn); assert(fvm_object_set(c, receiver, "toString", &value) == FVM_OK); assert(fvm_object_set(c, receiver, "valueOf", &value) == FVM_OK);
                assert(borrow(c, proto.object, methods[method], receiver, 1, &arg, &out) == (version ? FVM_OK : FVM_INVALID_ARGUMENT)); if (version) assert(out.type == FVM_VALUE_UNDEFINED);
                assert(c->gc_frames == 0 && c->primitive_depth == 0 && c->string_depth == 0 && c->avm1_execution_depth == 0);
            }
            FVM_Object *valid = 0; fvm_value_number(&value, 81); assert(fvm_construct(c, classes[cls], 1, &value, &valid) == FVM_OK); assert(fvm_gc_pin(c, valid) == FVM_OK);
            value = object(fn); assert(fvm_object_set(c, valid, "toString", &value) == FVM_OK); assert(fvm_object_set(c, valid, "valueOf", &value) == FVM_OK);
            assert(borrow(c, proto.object, "valueOf", valid, 0, 0, &out) == FVM_OK && (cls ? out.type == FVM_VALUE_BOOLEAN && out.boolean : out.type == FVM_VALUE_NUMBER && out.number == 81));
            assert(borrow(c, proto.object, "toString", valid, 0, 0, &out) == FVM_OK && !std::strcmp(out.string, cls ? "true" : "81"));
        }
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
