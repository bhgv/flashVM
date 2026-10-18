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
        FVM_Value value, out; FVM_Object *str = 0, *fn = 0;
        fvm_value_string_bytes(&value, "a\0b", 3); assert(fvm_construct(c, "String", 1, &value, &str) == FVM_OK); assert(fvm_gc_pin(c, str) == FVM_OK);
        assert(fvm_function_create_native(c, unexpected, 0, 0, &fn) == FVM_OK); value = object(fn);
        assert(fvm_object_set(c, str, "toString", &value) == FVM_OK);
        FVM_Value klass, prototype; assert(fvm_get_global(c, "String", &klass) == FVM_OK); assert(fvm_object_get(c, klass.object, "prototype", &prototype) == FVM_OK);
        const char *methods[] = {"valueOf","toString"};
        for (unsigned method = 0; method < 2; ++method) {
            assert(borrow(c, prototype.object, methods[method], str, 0, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
            for (unsigned kind = 0; kind < 5; ++kind) {
                FVM_Object *receiver = 0;
                if (kind == 4) receiver = fn;
                else { fvm_value_number(&value, 1); assert(fvm_construct(c, kind == 0 ? "Object" : kind == 1 ? "Number" : kind == 2 ? "Boolean" : "Array", kind ? 1 : 0, kind ? &value : 0, &receiver) == FVM_OK); }
                value = object(fn); assert(fvm_object_set(c, receiver, "toString", &value) == FVM_OK); assert(fvm_object_set(c, receiver, "valueOf", &value) == FVM_OK);
                assert(borrow(c, prototype.object, methods[method], receiver, 0, 0, &out) == (version ? FVM_OK : FVM_INVALID_ARGUMENT));
                if (version) assert(out.type == FVM_VALUE_UNDEFINED);
                assert(c->gc_frames == 0 && c->string_depth == 0 && c->primitive_depth == 0 && c->avm1_execution_depth == 0);
            }
        }
        assert(fvm_gc_string_mode(c, FVM_GC_STRINGS_TRACED) == FVM_OK); assert(fvm_gc_collect(c, 0) == FVM_OK);
        assert(borrow(c, prototype.object, "valueOf", str, 0, 0, &out) == FVM_OK && fvm_value_string_size(&out) == 3 && !std::memcmp(out.string, "a\0b", 3));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
