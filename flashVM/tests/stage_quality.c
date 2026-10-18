/* Context-local quality and real ToggleQuality dispatch, including actual native function entries. */
#include "flashvm/stage.h"
#include "flashvm/avm1.h"
#include "flashvm/execution_mode.h"
#include "flashvm/function.h"
#include "flashvm/jit.h"
#include <assert.h>
static void expect(FVM_Context *c, FVM_StageQuality wanted)
{
    FVM_StageQuality actual;
    assert(fvm_stage_get_quality(c,&actual)==FVM_OK && actual==wanted);
}
int main(void)
{
    FVM_Config cfg;FVM_Runtime *rt;FVM_Context *c,*other;
    FVM_StageQuality q=FVM_STAGE_QUALITY_BEST;
    const unsigned char toggle[]={8,0};
    const unsigned char function[]={0x96,5,0,7,42,0,0,0,8,0x3e,0};
    unsigned version,mode;
    FVM_JitCode *compiled=0;
    if(fvm_jit_available()){assert(fvm_jit_compile(toggle,sizeof(toggle),&compiled)==FVM_OK);assert(fvm_jit_destroy(compiled)==FVM_OK);}
    else assert(fvm_jit_compile(toggle,sizeof(toggle),&compiled)==FVM_UNSUPPORTED && !compiled);
    assert(fvm_stage_get_quality(0,&q)==FVM_INVALID_ARGUMENT && q==FVM_STAGE_QUALITY_BEST);
    assert(fvm_stage_set_quality(0,FVM_STAGE_QUALITY_HIGH)==FVM_INVALID_ARGUMENT);
    assert(fvm_stage_toggle_quality(0)==FVM_INVALID_ARGUMENT);
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&other)==FVM_OK);
    for(version=1;version<=8;++version)for(mode=0;mode<2;++mode)
    {
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
        FVM_Object *fn;FVM_Value out;
        assert(fvm_context_create(rt,&c)==FVM_OK);expect(c,FVM_STAGE_QUALITY_HIGH);
        assert(fvm_stage_get_quality(c,0)==FVM_INVALID_ARGUMENT);
        assert(fvm_stage_set_quality(c,(FVM_StageQuality)-1)==FVM_INVALID_ARGUMENT);
        assert(fvm_stage_set_quality(c,(FVM_StageQuality)4)==FVM_INVALID_ARGUMENT);
        expect(c,FVM_STAGE_QUALITY_HIGH);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_BEST)==FVM_OK);
        swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        expect(c,FVM_STAGE_QUALITY_BEST);
        assert(fvm_execution_set_mode(c,mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);expect(c,FVM_STAGE_QUALITY_LOW);
        assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);expect(c,FVM_STAGE_QUALITY_BEST);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_MEDIUM)==FVM_OK);
        assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);expect(c,FVM_STAGE_QUALITY_HIGH);
        assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);expect(c,FVM_STAGE_QUALITY_LOW);
        assert(fvm_avm1_execute(c,toggle,sizeof(toggle))==FVM_OK);expect(c,FVM_STAGE_QUALITY_HIGH);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_BEST)==FVM_OK);
        assert(fvm_stage_toggle_quality(c)==FVM_OK);
        assert(fvm_stage_set_quality(c,FVM_STAGE_QUALITY_LOW)==FVM_OK);
        assert(fvm_stage_toggle_quality(c)==FVM_OK);expect(c,FVM_STAGE_QUALITY_HIGH);
        assert(fvm_function_create_avm1(c,function,sizeof(function),0,0,0,&fn)==FVM_OK);
        {FVM_ExecutionStats before,after;before.struct_size=sizeof(before);after.struct_size=sizeof(after);
        assert(fvm_execution_get_stats(c,&before)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==42);
        assert(fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries+(mode && fvm_jit_available()?1UL:0UL));}
        expect(c,FVM_STAGE_QUALITY_LOW);expect(other,FVM_STAGE_QUALITY_HIGH);
        fvm_context_destroy(c);
    }
    fvm_context_destroy(other);fvm_runtime_destroy(rt);return 0;
}
