/* Native class bridges retain detached prototype results through collecting/throwing accessors. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
static unsigned failure,gets,sets;
static FVM_Object *super_expected;
static FVM_Result read_empty(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *out,void *)
{fvm_value_undefined(out);return FVM_OK;}
static FVM_Result read_parent(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    int removed=0;++gets;
    assert(fvm_object_delete(c,frame->receiver,"prototype",&removed)==FVM_OK && removed);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    *out=class_value(frame->capture);return failure==1?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Result write_child(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++sets;assert(frame->argc==1 && frame->argv[0].type==FVM_VALUE_OBJECT);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    FVM_Value parent,value,ctor;assert(fvm_object_get(c,frame->argv[0].object,"__proto__",&parent)==FVM_OK && parent.type==FVM_VALUE_OBJECT);
    assert(fvm_object_get(c,parent.object,"alive",&value)==FVM_OK && value.number==17);
    assert(fvm_object_get(c,frame->argv[0].object,"constructor",&ctor)==FVM_OK && ctor.object==super_expected);
    assert(fvm_set_global(c,"published",frame->argv)==FVM_OK);fvm_value_number(out,88);
    return failure==2?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Object *class_callback(FVM_Context *c,FVM_ScriptBody body,FVM_Object *capture)
{
    const unsigned char empty[]={0};FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK);
    assert(fvm_function_set_body(c,fn,body,0,capture)==FVM_OK);return fn;
}
static void class_accessor(FVM_Context *c,FVM_Object *object,FVM_Object *get,FVM_Object *set)
{
    FVM_Value args[3],out;fvm_value_string(args,"prototype");args[1]=class_value(get);
    if(set)args[2]=class_value(set);else{fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;}
    assert(fvm_object_call(c,object,"addProperty",3,args,&out)==FVM_OK && out.boolean);
}
static unsigned boxes;
static FVM_Result class_box(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++boxes;assert(frame->argc==1 && frame->argv[0].type==FVM_VALUE_NUMBER && frame->argv[0].number==42);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    fvm_value_number(out,77);return failure?fvm_exception_raise(c,out):FVM_OK;
}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(failure=0;failure<3;++failure)
    {
        FVM_Context *c=0;FVM_Object *sub=0,*super=0,*parent=0;FVM_Value value;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_construct(c,"Object",0,0,&sub)==FVM_OK && fvm_construct(c,"Object",0,0,&super)==FVM_OK && fvm_object_create(c,&parent)==FVM_OK);
        fvm_value_number(&value,17);assert(fvm_object_set(c,parent,"alive",&value)==FVM_OK);
        class_accessor(c,super,class_callback(c,read_parent,parent),0);
        class_accessor(c,sub,class_callback(c,read_empty,0),class_callback(c,write_child,0));
        value=class_value(sub);assert(fvm_set_global(c,"Sub",&value)==FVM_OK);value=class_value(super);assert(fvm_set_global(c,"ParentClass",&value)==FVM_OK);
        super_expected=super;gets=sets=0;Program p;p.variable("Sub");p.variable("ParentClass");p.op(0x69);p.number(42);p.finish();
        class_run(c,p,failure?FVM_EXCEPTION:FVM_OK);assert(gets==1 && sets==(failure==1?0U:1U));
        assert(fvm_gc_collect(c,0)==FVM_OK);
        if(failure!=1)assert(fvm_get_global(c,"published",&value)==FVM_OK && value.type==FVM_VALUE_OBJECT);
        fvm_context_destroy(c);
    }
    for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(failure=0;failure<2;++failure)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *number=class_create(c,"Number");assert(fvm_function_set_body(c,number,class_box,0,0)==FVM_OK);
        Program p;p.variable("Object");p.number(42);p.op(0x2b);p.finish();boxes=0;
        FVM_Value out=class_run(c,p,failure?FVM_EXCEPTION:FVM_OK);assert(boxes==1);
        if(!failure)assert(out.type==FVM_VALUE_NULL);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
