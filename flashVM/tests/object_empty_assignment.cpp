/* Loaded SetMember coerces then ignores an empty key; unloaded host properties remain writable. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/property.h"
#include "flashvm/gc.h"
static unsigned names;
static FVM_Result name(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++names; return fvm_value_string(out, ""); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK); assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        FVM_Object *existing = 0; FVM_Value value;
        assert(fvm_object_create(c, &existing) == FVM_OK); assert(fvm_gc_pin(c, existing) == FVM_OK);
        fvm_value_number(&value, 9); assert(fvm_object_set(c, existing, "", &value) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK); object(c, "target", existing);
        FVM_Object *o = 0, *key = 0; assert(fvm_construct(c, "Object", 0, 0, &o) == FVM_OK);
        fvm_value_number(&value, 81); assert(fvm_object_set(c, o, "", &value) == FVM_OK);
        int own = 0; assert(fvm_object_has_own(c, o, "", &own) == FVM_OK && own == (version == 0));
        assert(fvm_object_set(c, existing, "", &value) == FVM_OK);
        assert(fvm_object_get(c, existing, "", &value) == FVM_OK && value.number == (version ? 9 : 81));
        assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, key, "toString", name, 0, 0) == FVM_OK); object(c, "key", key);
        Bytes code; global(code, "target"); global(code, "key"); integer(code, 7); code.push_back(0x4f); code.push_back(0);
        names = 0; assert(fvm_avm1_execute(c, code.begin(), code.size()) == FVM_OK && names == 1);
        assert(fvm_object_get(c, existing, "", &value) == FVM_OK && value.number == (version ? 9 : 7));
        const char nul[] = {'\0'}; fvm_value_number(&value, 81); assert(fvm_object_set_bytes(c, existing, nul, 1, &value) == FVM_OK);
        assert(fvm_object_has_own_bytes(c, existing, nul, 1, &own) == FVM_OK && own);
        Bytes result_code; integer(result_code, 7); assert(run(c, result_code).number == 7);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
