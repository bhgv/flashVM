/* Native ToggleQuality scalar transitions, control-flow and context isolation. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/stage.h"
static void quality_expect(FVM_Context *c, FVM_StageQuality expected)
{
    FVM_StageQuality actual;
    assert(fvm_stage_get_quality(c, &actual) == FVM_OK && actual == expected);
}

#ifndef FVM_QUALITY_FIXTURE_ONLY
int main()
{
    if (!fvm_jit_available()) return 0;
    FVM_Config cfg; FVM_Runtime *rt = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK);
    for (unsigned version = 0; version <= 8; ++version)
    for (class_mode = 0; class_mode < 3; ++class_mode)
    {
        FVM_Context *c = 0, *peer = 0;
        assert(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &peer) == FVM_OK);
        if (version)
        {
            unsigned char swf[] = {'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
            swf[3] = (unsigned char)version;
            assert(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK);
        }
        Program toggle; toggle.number(42); toggle.op(8); toggle.finish();
        for (unsigned start = 0; start <= 3; ++start)
        {
            FVM_StageQuality state = (FVM_StageQuality)start;
            assert(fvm_stage_set_quality(c, state) == FVM_OK);
            for (unsigned step = 0; step < 6; ++step)
            {
                state = state == FVM_STAGE_QUALITY_HIGH || state == FVM_STAGE_QUALITY_BEST
                    ? FVM_STAGE_QUALITY_LOW : start == FVM_STAGE_QUALITY_BEST
                    ? FVM_STAGE_QUALITY_BEST : FVM_STAGE_QUALITY_HIGH;
                FVM_Value value = class_run(c, toggle);
                assert(value.type == FVM_VALUE_NUMBER && value.number == 42);
                quality_expect(c, state); quality_expect(peer, FVM_STAGE_QUALITY_HIGH);
            }
        }
        /* Explicit selection forgets remembered BEST even after a previous native toggle. */
        assert(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_BEST) == FVM_OK);
        class_run(c, toggle); quality_expect(c, FVM_STAGE_QUALITY_LOW);
        assert(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_LOW) == FVM_OK);
        class_run(c, toggle); quality_expect(c, FVM_STAGE_QUALITY_HIGH);
        Program skipped; skipped.number(42); skipped.op(0x99); skipped.op(2);
        skipped.op(0); skipped.op(1); skipped.op(0); skipped.op(8); skipped.finish();
        assert(class_run(c, skipped).number == 42); quality_expect(c, FVM_STAGE_QUALITY_HIGH);
        unsigned long limit = rt->config.instruction_limit;
        rt->config.instruction_limit = 1;
        class_run(c, toggle, FVM_ERROR); quality_expect(c, FVM_STAGE_QUALITY_HIGH);
        rt->config.instruction_limit = 2;
        class_run(c, toggle, FVM_ERROR); quality_expect(c, FVM_STAGE_QUALITY_LOW);
        rt->config.instruction_limit = limit;
        fvm_context_destroy(c); fvm_context_destroy(peer);
    }
    fvm_runtime_destroy(rt); return 0;
}

#endif
