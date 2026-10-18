/* Dynamic frame requests collect/throw before target resolution and preserve conversion order. */
#define FVM_TIMELINE_FIXTURE_ONLY
#include "jit_timeline.cpp"
static unsigned timeline_conversions;
static bool timeline_throw,timeline_named;
static FVM_Result timeline_convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++timeline_conversions;FVM_Value value;fvm_value_undefined(&value);
    assert(fvm_set_global(c,"request",&value)==FVM_OK);FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    assert(fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==17);
    fvm_value_string(out,timeline_named?"two":"002");return timeline_throw?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Result timeline_switch(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    ++timeline_conversions;FVM_Value path;fvm_value_string(&path,"child");FVM_Result status=fvm_exec_select_target(c,&path);
    return status==FVM_OK?fvm_value_string(out,"2"):status;
}
static void timeline_request(FVM_Context *c,bool switching)
{
    FVM_Object *object=0,*fn=0;FVM_Value value;const unsigned char empty[]={0};
    assert(fvm_object_create(c,&object)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,object,"alive",&value)==FVM_OK);
    assert(fvm_object_define_method_rooted(c,object,"valueOf",timeline_forbidden,0,0)==FVM_OK);
    if(switching)assert(fvm_object_define_method_rooted(c,object,"toString",timeline_switch,0,0)==FVM_OK);
    else
    {
        assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,timeline_convert,0,0)==FVM_OK);
        value=class_value(fn);assert(fvm_object_set(c,object,"toString",&value)==FVM_OK);
    }
    value=class_value(object);assert(fvm_set_global(c,"request",&value)==FVM_OK);
}
#ifndef FVM_TIMELINE_CALLBACK_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_Object *child=0,*root=timeline_setup(c,&child);
        for(unsigned named=0;named<2;++named)for(unsigned throwing=0;throwing<2;++throwing)
        {
            FVM_Value initial;fvm_value_number(&initial,1);assert(fvm_exec_frame_control(c,root,FVM_EXEC_GOTO_FRAME,&initial,1)==FVM_OK);
            timeline_named=named!=0;timeline_throw=throwing!=0;timeline_conversions=0;timeline_request(c,false);
            Program p;p.number(31);p.variable("request");timeline_dynamic(p,3,1);p.finish();
            FVM_Value out=class_run(c,p,throwing?FVM_EXCEPTION:FVM_OK,root);assert(timeline_conversions==1);
            if(!throwing)assert(out.number==31);timeline_state(root,throwing?1:(named?2:3),true);
        }
        FVM_Value initial;fvm_value_number(&initial,1);assert(fvm_exec_frame_control(c,root,FVM_EXEC_GOTO_FRAME,&initial,1)==FVM_OK);
        timeline_conversions=0;timeline_request(c,true);Program p;p.number(31);p.variable("request");timeline_dynamic(p,0);p.finish();
        FVM_Value out=class_run(c,p,FVM_OK,root);assert(out.number==31 && timeline_conversions==1);
        timeline_state(root,1,true);timeline_state(child,1,false);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
