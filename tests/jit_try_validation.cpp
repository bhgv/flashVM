/* Try framing/nesting, root catch activations and exact instruction/error boundaries. */
#define FVM_TRY_FIXTURE_ONLY
#include "jit_try.cpp"
#include "flashvm/debug.h"
static unsigned setter_calls,final_calls,breakpoint_hits;
static unsigned long breakpoint_offset;
static FVM_Result try_breakpoint(FVM_Context *,const FVM_DebugBreakpoint *point,void *)
{assert(point->offset==breakpoint_offset && !std::strcmp(point->function,"<function>"));++breakpoint_hits;return FVM_OK;}
static FVM_Result try_setter(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{++setter_calls;fvm_value_number(out,17);return fvm_exception_raise(c,out);}
static FVM_Result try_final(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++final_calls;fvm_value_undefined(out);return FVM_OK;}
static void root_context(FVM_Runtime *rt,bool loaded,FVM_Context **out)
{
    assert(fvm_context_create(rt,out)==FVM_OK);const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};if(loaded)assert(fvm_load_memory(*out,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(*out)==FVM_OK);
}
int main()
{
    if(!fvm_jit_available())return 0;const unsigned char good[]={0};FVM_JitCode *code=0,*keep=0;assert(fvm_jit_compile(good,sizeof(good),&code)==FVM_OK);keep=code;
    Program empty,valid;try_block(valid,empty,empty,empty,0);valid.op(0);
    for(unsigned kind=0;kind<5;++kind)
    {
        Program bad=valid;
        if(kind==0)bad.bytes[3]=8;if(kind==1)bad.bytes[6]=1;if(kind==2)bad.bytes[11]='x';if(kind==3)bad.bytes[4]=2;if(kind==4)bad.bytes[1]=7;
        assert(fvm_jit_compile(bad.bytes.begin(),bad.bytes.size(),&code)==FVM_INVALID_ARGUMENT && code==keep);
    }
    Program broken;broken.op(0x96);Program malformed;try_block(malformed,broken,empty,empty,0);malformed.op(0);assert(fvm_jit_compile(malformed.bytes.begin(),malformed.bytes.size(),&code)==FVM_INVALID_ARGUMENT && code==keep);
    Program body;body.number(42);body.op(0x17);Program caught;caught.op(0);Program crossing;crossing.op(0x99);definition_word(crossing,2);definition_word(crossing,12+body.bytes.size());try_block(crossing,body,caught,empty,1);crossing.op(0);assert(fvm_jit_compile(crossing.bytes.begin(),crossing.bytes.size(),&code)==FVM_INVALID_ARGUMENT && code==keep);
    Program unsupported;unsupported.op(0x08);Program rejected;try_block(rejected,unsupported,empty,empty,0);rejected.op(0);assert(fvm_jit_compile(rejected.bytes.begin(),rejected.bytes.size(),&code)==FVM_UNSUPPORTED && code==keep);
    Program deep=try_exit(42);for(unsigned i=0;i<65;++i){Program outer;try_block(outer,deep,empty,empty,2);deep=outer;}deep.op(0);assert(fvm_jit_compile(deep.bytes.begin(),deep.bytes.size(),&code)==FVM_UNSUPPORTED && code==keep);assert(fvm_jit_destroy(code)==FVM_OK);
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned native=0;native<2;++native)for(unsigned reg=0;reg<2;++reg)
    {
        FVM_Context *c=0;root_context(rt,loaded!=0,&c);FVM_Value value;fvm_value_number(&value,99);assert(fvm_set_global(c,"e",&value)==FVM_OK);fvm_value_number(&value,0);assert(fvm_set_global(c,"seen",&value)==FVM_OK);
        Program binding;binding.text("local");binding.number(17);binding.op(0x3c);binding.text("seen");if(reg)definition_register(binding,0);else binding.variable("e");binding.op(0x1d);
        Program p;try_block(p,try_exit(42,true),binding,empty,reg?5:1,"e",0);p.text("after");p.variable("e");p.op(0x1d);p.op(0);
        assert(fvm_execution_set_mode(c,native?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK && fvm_avm1_execute(c,p.bytes.begin(),p.bytes.size())==FVM_OK);
        assert(fvm_get_global(c,"seen",&value)==FVM_OK && value.number==42 && fvm_get_global(c,"after",&value)==FVM_OK && value.number==(loaded && !reg?42:99));
        assert(fvm_get_global(c,"local",&value)==FVM_OK && (loaded?value.type==FVM_VALUE_NUMBER && value.number==17:value.type==FVM_VALUE_UNDEFINED));assert(c->lexical_scopes.empty() && !c->gc_frames && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    for(unsigned loaded=0;loaded<2;++loaded)for(unsigned native=0;native<2;++native)
    {
        FVM_Context *c=0;root_context(rt,loaded!=0,&c);const unsigned char end[]={0};FVM_Object *getter=0,*setter=0;FVM_Value args[3],out,constructor,prototype;
        assert(fvm_get_global(c,"Object",&constructor)==FVM_OK && fvm_object_get(c,constructor.object,"prototype",&prototype)==FVM_OK && fvm_object_set_prototype(c,c->global,prototype.object)==FVM_OK);
        assert(fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&getter)==FVM_OK && fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&setter)==FVM_OK && fvm_function_set_body(c,setter,try_setter,0,0)==FVM_OK);
        fvm_value_string(args,"e");args[1]=class_value(getter);args[2]=class_value(setter);assert(fvm_object_call(c,c->global,"addProperty",3,args,&out)==FVM_OK && out.boolean && fvm_register_function_rooted(c,"final",try_final,0,0)==FVM_OK);
        Program final;final.number(0);final.text("final");final.op(0x3d);final.op(0x17);Program p;try_block(p,try_exit(42,true),empty,final);p.op(0);setter_calls=final_calls=0;
        assert(fvm_execution_set_mode(c,native?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK && fvm_avm1_execute(c,p.bytes.begin(),p.bytes.size())==(loaded?FVM_EXCEPTION:FVM_OK));assert(setter_calls==loaded && final_calls==!loaded);
        if(loaded)assert(fvm_exception_get(c,&out)==FVM_OK && out.number==17 && fvm_exception_clear(c)==FVM_OK);assert(c->lexical_scopes.empty() && !c->gc_frames && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    FVM_Context *c=0;root_context(rt,false,&c);Program nested=try_exit(42);for(unsigned i=0;i<64;++i){Program outer;try_block(outer,nested,empty,empty,2);nested=outer;}class_mode=2;assert(with_run(c,nested).number==42);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    for(unsigned loaded=0;loaded<2;++loaded)for(class_mode=0;class_mode<3;++class_mode)for(unsigned end=0;end<2;++end)for(unsigned success=0;success<2;++success)
    {
        assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=8+end-(success?0:1);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);root_context(rt,loaded!=0,&c);
        Program final;final.number(22);final.op(0x17);if(end)final.op(0);Program p;p.number(77);try_block(p,try_exit(42,true,end!=0),try_exit(11,false,end!=0),final);FVM_Value out=with_run(c,p,success?FVM_OK:FVM_ERROR);if(success)assert(out.type==FVM_VALUE_NUMBER && out.number==11);assert(c->avm1_steps==8+end && c->lexical_scopes.empty());fvm_context_destroy(c);fvm_runtime_destroy(rt);
    }
    for(unsigned native=0;native<2;++native)
    {
        assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=3;assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);root_context(rt,false,&c);assert(fvm_register_function_rooted(c,"final",try_final,0,0)==FVM_OK);
        Program final;final.number(0);final.text("final");final.op(0x3d);final.op(0x17);Program p;p.number(77);size_t throw_at=p.bytes.size()+12+8;try_block(p,try_exit(42,true),empty,final,2);p.finish();final_calls=0;
        assert(fvm_execution_set_mode(c,native?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK && fvm_avm1_execute(c,p.bytes.begin(),p.bytes.size())==FVM_ERROR && c->avm1_steps==4 && c->debug_offset==throw_at && final_calls==0 && !c->exception_pending);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    }
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);root_context(rt,false,&c);Program final;final.number(17);final.op(0x17);Program p;Program raised=try_exit(42,true);breakpoint_offset=12+raised.bytes.size();try_block(p,raised,try_exit(11),final);p.finish();FVM_Object *fn=0;FVM_Value out;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==11);
    unsigned long before=c->execution_mode.stats.native_entries;breakpoint_hits=0;assert(fvm_debug_set_breakpoint_handler(c,try_breakpoint,0)==FVM_OK && fvm_debug_set_breakpoint(c,"<function>",breakpoint_offset,1)==FVM_OK);
    assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==11 && breakpoint_hits==1 && c->execution_mode.stats.native_entries==before);
    assert(fvm_debug_set_breakpoint(c,"<function>",breakpoint_offset,0)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==11 && c->execution_mode.stats.native_entries==before+1);fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
