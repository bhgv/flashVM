#include "flashvm/execution.h"
#include "flashvm/function.h"
#include "flashvm/standard.h"
#include "flashvm/debug.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
#include <limits>
struct State { FVM_Object *nested; unsigned calls; int overflow; };
static FVM_Result generated(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    State *state=(State *)raw;++state->calls;
    if(state->overflow)
    {
        assert(fvm_exec_checkpoint(c,std::numeric_limits<unsigned long>::max(),50,0)==FVM_OK);
        assert(fvm_exec_checkpoint(c,1,51,0)==FVM_ERROR);
        assert(c->avm1_steps==std::numeric_limits<unsigned long>::max());
        return FVM_ERROR;
    }
    assert(fvm_exec_checkpoint(c,2,50,FVM_EXEC_POLL)==FVM_OK);
    assert(c->debug_line==7 && c->debug_source==fvm_detail::string("generated.as") && c->debug_offset==50);
    FVM_Object *temporary=0;assert(fvm_object_create(c,&temporary)==FVM_OK);
    fvm_value_undefined(&frame->registers[0]);frame->registers[0].type=FVM_VALUE_OBJECT;frame->registers[0].object=temporary;
    c->gc_allocations=std::numeric_limits<unsigned long>::max();
    assert(fvm_exec_checkpoint(c,0,50,FVM_EXEC_POLL)==FVM_OK);
    FVM_Value alive;fvm_value_number(&alive,17);assert(fvm_object_set(c,temporary,"alive",&alive)==FVM_OK);
    assert(fvm_exec_checkpoint(c,0,50,2)==FVM_INVALID_ARGUMENT && c->avm1_steps==2);
    assert(fvm_function_invoke(c,state->nested,0,0,0,out)==FVM_OK && out->number==7);
    assert(c->avm1_steps==4 && c->debug_line==7 && c->debug_offset==50);
    return fvm_exec_checkpoint(c,1,51,0);
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=4;
    assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    const unsigned char code[]={0x96,5,0,7,7,0,0,0,0x3e,0};FVM_Object *nested=0,*fn=0;
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&nested)==FVM_OK && fvm_gc_pin(c,nested)==FVM_OK);
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
    State state={nested,0,0};assert(fvm_function_set_body(c,fn,generated,&state,0)==FVM_OK);
    assert(fvm_debug_map_action(c,"<function>",50,"generated.as",7)==FVM_OK);
    FVM_GCRootFrame *roots=c->gc_frames;FVM_Value out;
    assert(fvm_exec_checkpoint(c,1,0,FVM_EXEC_POLL)==FVM_INVALID_ARGUMENT);
    for(unsigned i=0;i<2;++i)
    {
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_ERROR && c->avm1_steps==5);
        assert(!c->avm1_execution_depth && !c->script_depth && c->gc_frames==roots && !c->avm1_target_active);
        assert(fvm_function_invoke(c,nested,0,0,0,&out)==FVM_OK && c->avm1_steps==2);
    }
    rt->config.instruction_limit=0;state.overflow=1;
    assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_ERROR && c->avm1_steps==std::numeric_limits<unsigned long>::max());
    assert(!c->avm1_execution_depth && c->gc_frames==roots);
    assert(fvm_function_invoke(c,nested,0,0,0,&out)==FVM_OK && c->avm1_steps==2 && state.calls==3);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
