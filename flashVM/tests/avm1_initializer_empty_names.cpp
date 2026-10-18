/* Loaded Object literals coerce then ignore empty property names; raw host mode retains keys. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
static unsigned calls;
static FVM_Result empty_name(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++calls; return fvm_value_string(out, ""); }
int main()
{
    FVM_Config cfg; FVM_Runtime *rt = 0; assert(fvm_config_init(&cfg) == FVM_OK);
    assert(fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(rt, &c) == FVM_OK);
        if (version) {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0}; swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        assert(fvm_standard_install(c) == FVM_OK);
        Bytes code; text(code, ""); text(code, "value"); integer(code, 1); code.push_back(0x43);
        FVM_Value result = run(c, code), value; int own = 0;
        assert(fvm_object_has_own_bytes(c, result.object, "", 0, &own) == FVM_OK && own == (version == 0));
        assert(fvm_object_get_bytes(c, result.object, "", 0, &value) == FVM_OK);
        assert(version ? value.type == FVM_VALUE_UNDEFINED : value.type == FVM_VALUE_STRING);
        FVM_Object *key = 0; assert(fvm_construct(c, "Object", 0, 0, &key) == FVM_OK);
        assert(fvm_object_define_method_rooted(c, key, "toString", empty_name, 0, 0) == FVM_OK); object(c, "key", key);
        code.clear(); global(code, "key"); text(code, "value"); integer(code, 1); code.push_back(0x43); calls = 0;
        result = run(c, code); assert(calls == 1);
        assert(fvm_object_has_own_bytes(c, result.object, "", 0, &own) == FVM_OK && own == (version == 0));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
