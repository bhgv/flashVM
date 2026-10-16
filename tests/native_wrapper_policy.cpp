/* Trusted internal entry policy changes invalidate wrappers while saved Functions keep their policy. */
#include "../src/core/runtime.hpp"
#include "flashvm/function.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include <cassert>
static bool policy_trusted;
static FVM_Result policy_native(FVM_Context *c, FVM_Object *, unsigned, const FVM_Value *, FVM_Value *out, void *)
{
    assert((c->gc_native_depth == 0) == policy_trusted);
    if (policy_trusted) assert(fvm_gc_collect(c, 0) == FVM_OK);
    fvm_value_number(out, 42); return FVM_OK;
}
static FVM_Result policy_read(FVM_Context *c, FVM_Object *owner, unsigned global, FVM_Value *out)
{ return global ? fvm_get_global(c, "policy", out) : fvm_object_get(c, owner, "policy", out); }
int main()
{
    FVM_Config config; FVM_Runtime *runtime = 0; assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version) for (unsigned global = 0; global < 2; ++global) {
        if (version && version < 4) continue;
        FVM_Context *c = 0; assert(fvm_context_create(runtime, &c) == FVM_OK);
        if (version) { unsigned char swf[] = {'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0}; swf[3] = (unsigned char)version; assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK); }
        assert(fvm_standard_install(c) == FVM_OK);
        FVM_Object *owner = c->global;
        if (global) assert(fvm_register_function(c, "policy", policy_native, 0) == FVM_OK);
        else assert(fvm_object_create(c, &owner) == FVM_OK && fvm_gc_pin(c, owner) == FVM_OK && fvm_object_define_method_rooted(c, owner, "policy", policy_native, 0, 0) == FVM_OK);
        FVM_Value original, trusted, restored, out;
        assert(policy_read(c, owner, global, &original) == FVM_OK && fvm_gc_pin(c, original.object) == FVM_OK);
        policy_trusted = false; assert(fvm_function_invoke(c, original.object, owner, 0, 0, &out) == FVM_OK && out.number == 42);
        FVM_NativeEntry &entry = global ? c->functions.find("policy")->second : owner->methods.find("policy")->second;
        entry.collector_entry = true; /* Internal fixture only: public callbacks remain guarded. */
        assert(policy_read(c, owner, global, &trusted) == FVM_OK && trusted.object != original.object && trusted.object->invocable.collector_entry && fvm_gc_pin(c, trusted.object) == FVM_OK);
        policy_trusted = true; assert(fvm_function_invoke(c, trusted.object, owner, 0, 0, &out) == FVM_OK && out.number == 42);
        entry.collector_entry = false;
        assert(policy_read(c, owner, global, &restored) == FVM_OK && restored.object != trusted.object && !restored.object->invocable.collector_entry);
        policy_trusted = false; assert(fvm_function_invoke(c, restored.object, owner, 0, 0, &out) == FVM_OK && fvm_function_invoke(c, original.object, owner, 0, 0, &out) == FVM_OK);
        policy_trusted = true; assert(fvm_function_invoke(c, trusted.object, owner, 0, 0, &out) == FVM_OK);
        assert(!c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c, 0) == FVM_OK); fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime); return 0;
}
