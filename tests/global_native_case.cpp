/* Registered native global names share loaded property case selection and Function identity. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/execution.h"
static unsigned native_case_calls;
static FVM_Result native_case(FVM_Context *, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{ ++native_case_calls; fvm_value_number(out, 42); return FVM_OK; }
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    const char *stored[] = {"NativeMix", "\xc3\x84\0Key"}, *queried[] = {"nATIVEmIX", "\xc3\xa4\0kEY"}; const size_t sizes[] = {9, 6};
    for (unsigned version = 0; version <= 8; ++version) for (unsigned which = 0; which < 2; ++which) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK && fvm_register_function_rooted_bytes(c, stored[which], sizes[which], native_case, 0, 0) == FVM_OK);
        bool folded = version && version <= 6; FVM_Value canonical, alias, out, name; native_case_calls = 0;
        assert(fvm_get_global_bytes(c, stored[which], sizes[which], &canonical) == FVM_OK && canonical.type == FVM_VALUE_OBJECT);
        assert(fvm_get_global_bytes(c, queried[which], sizes[which], &alias) == FVM_OK && (folded ? alias.type == FVM_VALUE_OBJECT && alias.object == canonical.object : alias.type == FVM_VALUE_UNDEFINED));
        fvm_value_string_bytes(&name, queried[which], sizes[which]); FVM_Object *receiver = 0; int present = 0;
        assert(fvm_exec_variable_lookup(c, 0, 0, 0, 0, 0, &name, &out, &receiver, &present) == FVM_OK && !!present == folded && (folded ? out.object == canonical.object : out.type == FVM_VALUE_UNDEFINED));
        assert(fvm_object_call_bytes(c, c->global, queried[which], sizes[which], 0, 0, &out) == (folded ? FVM_OK : FVM_UNSUPPORTED));
        if (folded) assert(out.type == FVM_VALUE_NUMBER && out.number == 42);
        assert(fvm_call_bytes(c, queried[which], sizes[which], 0, 0, &out) == (folded ? FVM_OK : FVM_UNSUPPORTED));
        if (folded) assert(out.type == FVM_VALUE_NUMBER && out.number == 42);
        assert(native_case_calls == (folded ? 2U : 0U));
        if (!which && version && fvm_jit_available()) for (class_mode = 0; class_mode < 3; ++class_mode) {
            Program code; code.number(0); code.text(queried[which]); code.op(0x3d); code.finish(); out = class_run(c, code);
            assert(folded ? out.type == FVM_VALUE_NUMBER && out.number == 42 : out.type == FVM_VALUE_UNDEFINED);
        }
        assert(fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime); return 0;
}
