/* Native quality helper allocates nothing and rejects invalid lifecycle phases atomically. */
#define main quality_allocation_reference_main
#include "native_instance_allocation.cpp"
#undef main
#define FVM_QUALITY_FIXTURE_ONLY
#include "jit_stage_quality.cpp"
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0; FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    assert(fvm_context_create(rt, &c) == FVM_OK);
    FVM_JitInvocation state; state.context = c;
    assert(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_BEST) == FVM_OK);
    allowance = 0;
    assert(fvm_jit_toggle_quality(&state, 0) == FVM_OK); quality_expect(c, FVM_STAGE_QUALITY_LOW);
    assert(fvm_jit_toggle_quality(&state, 0) == FVM_OK); quality_expect(c, FVM_STAGE_QUALITY_BEST);
    c->destroying = true;
    assert(fvm_jit_toggle_quality(&state, 0) == FVM_INVALID_ARGUMENT); c->destroying = false;
    c->gc_collecting = true;
    assert(fvm_jit_toggle_quality(&state, 0) == FVM_INVALID_ARGUMENT); c->gc_collecting = false;
    c->gc_releasing = true;
    assert(fvm_jit_toggle_quality(&state, 0) == FVM_INVALID_ARGUMENT); c->gc_releasing = false;
    quality_expect(c, FVM_STAGE_QUALITY_BEST);
    state.context = 0; assert(fvm_jit_toggle_quality(&state, 0) == FVM_INVALID_ARGUMENT);
    allowance = -1;
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
