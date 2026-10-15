/* Named enumeration retains collecting/throwing names and detached accessor results. */
#define FVM_ENUMERATE_FIXTURE_ONLY
#include "jit_enumerate.cpp"
static unsigned enum_steps,enum_failure;
static bool enum_qualified;
static FVM_Result enum_name(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    assert(++enum_steps==1);FVM_Value value;fvm_value_undefined(&value);
    assert(fvm_set_global(c,"selector",&value)==FVM_OK);FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    assert(fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==17);
    fvm_value_string(out,enum_qualified?"holder:source":"source");return enum_failure==1?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Result enum_get(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++enum_steps;int removed=0;
    assert(fvm_object_delete(c,frame->receiver,"source",&removed)==FVM_OK && removed);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    *out=class_value(frame->capture);return enum_failure==2?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Object *enum_callback(FVM_Context *c,FVM_ScriptBody body,FVM_Object *capture)
{
    const unsigned char empty[]={0};FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,body,0,capture)==FVM_OK);return fn;
}
static FVM_Object *enum_callbacks(FVM_Context *c)
{
    FVM_Object *object=0,*selector=0,*holder=0;FVM_Value value,args[3],out;
    assert(fvm_object_create(c,&object)==FVM_OK && fvm_object_create(c,&selector)==FVM_OK);
    fvm_value_number(&value,17);assert(fvm_object_set(c,object,"live",&value)==FVM_OK && fvm_object_set(c,selector,"alive",&value)==FVM_OK);
    value=class_value(enum_callback(c,enum_name,0));assert(fvm_object_set(c,selector,"toString",&value)==FVM_OK);
    value=class_value(selector);assert(fvm_set_global(c,"selector",&value)==FVM_OK);
    assert(fvm_object_create(c,&holder)==FVM_OK);value=class_value(holder);assert(fvm_set_global(c,"holder",&value)==FVM_OK);
    FVM_Value constructor,prototype;
    assert(fvm_get_global(c,"Object",&constructor)==FVM_OK && fvm_object_get(c,constructor.object,"prototype",&prototype)==FVM_OK);
    assert(fvm_object_set_prototype(c,holder,prototype.object)==FVM_OK);
    fvm_value_string(args,"source");args[1]=class_value(enum_callback(c,enum_get,object));fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;
    assert(fvm_object_call(c,holder,"addProperty",3,args,&out)==FVM_OK && out.boolean);return holder;
}
#ifndef FVM_ENUMERATE_CALLBACK_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(unsigned qualified=0;qualified<2;++qualified)for(enum_failure=0;enum_failure<3;++enum_failure)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);FVM_Object *holder=enum_callbacks(c);enum_steps=0;enum_qualified=qualified!=0;
        Program p;if(version)p.variable("selector");else p.text(enum_qualified?"holder:source":"source");p.op(0x46);p.finish();
        FVM_Result expected=enum_failure && (version || enum_failure==2)?FVM_EXCEPTION:FVM_OK;
        FVM_Value out=class_run(c,p,expected,holder);assert(enum_steps==(version?(enum_failure==1?1U:2U):1U));
        if(expected==FVM_OK)assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"live"));
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
