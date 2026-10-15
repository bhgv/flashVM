/* Native legacy comparisons preserve coercion order, collecting hooks and right-side metamethod fallback. */
#define FVM_LEGACY_FIXTURE_ONLY
#include "jit_legacy.cpp"
#include "flashvm/metamethod.h"
static unsigned legacy_order[16],legacy_count,meta_calls;
static bool legacy_throw,meta_empty;
static double expected_owner,expected_left,expected_right;
static FVM_Result legacy_number(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *raw)
{
    unsigned id=*(unsigned *)raw;assert(legacy_count<16);legacy_order[legacy_count++]=id;
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;fvm_value_number(out,id);
    return legacy_throw && id==2?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Value legacy_object(FVM_Context *c,unsigned *id)
{
    const unsigned char empty[]={0};FVM_Object *object=0,*fn=0;FVM_Value value;
    assert(fvm_object_create(c,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK);
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK);
    assert(fvm_function_set_body(c,fn,legacy_number,id,0)==FVM_OK);
    fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
    assert(fvm_object_set(c,object,"valueOf",&value)==FVM_OK);value.object=object;return value;
}
static FVM_Result legacy_hook(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++meta_calls;assert(frame->argc==4 && frame->argv[0].number==expected_owner &&
        frame->argv[1].type==FVM_VALUE_OBJECT && frame->argv[2].number==expected_left && frame->argv[3].number==expected_right);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    fvm_value_string(out,meta_empty?"":"hook");return legacy_throw?fvm_exception_raise(c,out):FVM_OK;
}
#ifndef FVM_LEGACY_CALLBACK_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);unsigned ids[]={1,2};FVM_Value args[2];
        args[0]=legacy_object(c,ids);args[1]=legacy_object(c,ids+1);
        for(unsigned less=0;less<2;++less)
        {
            Program p=string_program(less?0x0f:0x0e,2);legacy_count=0;legacy_throw=false;
            string_check(c,p,2,args);assert(legacy_count==6);
            for(unsigned i=0;i<3;++i)assert(legacy_order[i*2]==(less?1U:2U) && legacy_order[i*2+1]==(less?2U:1U));
            legacy_count=0;legacy_throw=true;FVM_Value unused;fvm_value_undefined(&unused);
            string_check(c,p,2,args,&unused,FVM_EXCEPTION);assert(legacy_count==(less?6U:3U));
            for(unsigned i=0;i<legacy_count;++i)assert(legacy_order[i]==(less?(i%2?2U:1U):2U));
        }
        legacy_throw=false;
        for(unsigned less=0;less<2;++less)for(unsigned attachment=0;attachment<3;++attachment)
        {
            unsigned right=attachment==1?1:0;
            FVM_Object *meta=0,*fn=0;const unsigned char empty[]={0};FVM_Value value,expected;
            assert(fvm_object_create(c,&meta)==FVM_OK && fvm_gc_pin(c,meta)==FVM_OK);
            assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK);
            assert(fvm_function_set_body(c,fn,legacy_hook,0,0)==FVM_OK);
            fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=fn;
            assert(fvm_object_set(c,meta,less?"__lt":"__eq",&value)==FVM_OK);
            expected_left=less?2:1;expected_right=less?1:2;expected_owner=right?expected_right:expected_left;
            fvm_value_number(args,expected_left);fvm_value_number(args+1,expected_right);
            assert(fvm_value_set_metatable(c,args+right,meta)==FVM_OK);
            if(attachment==2)assert(fvm_value_set_metatable(c,args+1,meta)==FVM_OK);
            Program p=string_program(less?0x0f:0x0e,2);
            meta_calls=0;fvm_value_boolean(&expected,1);string_check(c,p,2,args,&expected);assert(meta_calls==3);
            meta_calls=0;meta_empty=true;fvm_value_boolean(&expected,0);string_check(c,p,2,args,&expected);assert(meta_calls==3);meta_empty=false;
            meta_calls=0;legacy_throw=true;string_check(c,p,2,args,&expected,FVM_EXCEPTION);assert(meta_calls==3);legacy_throw=false;
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
