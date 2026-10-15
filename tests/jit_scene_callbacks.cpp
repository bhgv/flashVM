/* Native scene bridges preserve collecting coercion order and detached drag target roots. */
#define FVM_SCENE_FIXTURE_ONLY
#include "jit_scene.cpp"
#include <cstdio>
static unsigned scene_sequence,scene_failure;
static bool scene_cloning,scene_detach;
static FVM_Object *scene_clip;
static FVM_Result scene_convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    unsigned id=*(unsigned *)raw;assert(++scene_sequence==id);char key[16];std::sprintf(key,"g%u",id);
    FVM_Value value;fvm_value_undefined(&value);assert(fvm_set_global(c,key,&value)==FVM_OK);
    if(scene_detach && id==2)assert(fvm_exec_detach_clip(c,scene_clip)==FVM_OK);
    FVM_Result status=fvm_gc_collect(c,0);if(status!=FVM_OK)return status;
    assert(fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==17);
    if(scene_detach && id>=2)assert(fvm_object_get(c,scene_clip,"alive",&value)==FVM_OK && value.number==17);
    if(scene_cloning)
    {if(id==1)fvm_value_number(out,16387);else fvm_value_string(out,id==2?"copy":"child");}
    else
    {if(id==1)fvm_value_string(out,"child");else fvm_value_number(out,id<4?1:(id<6?10:0));}
    return scene_failure==id?fvm_exception_raise(c,out):FVM_OK;
}
static void scene_coercion(FVM_Context *c,unsigned *id)
{
    FVM_Object *object=0,*fn=0;FVM_Value value;const unsigned char empty[]={0};char key[16];std::sprintf(key,"g%u",*id);
    assert(fvm_object_create(c,&object)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,object,"alive",&value)==FVM_OK);
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,scene_convert,id,0)==FVM_OK);
    value=class_value(fn);bool text=scene_cloning?*id!=1:*id==1;
    assert(fvm_object_set(c,object,text?"toString":"valueOf",&value)==FVM_OK);value=class_value(object);assert(fvm_set_global(c,key,&value)==FVM_OK);
}
static bool scene_flush_failure;
static unsigned scene_flush_reads;
static FVM_Result scene_read_x(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    ++scene_flush_reads;assert(c->drag_updating && fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);
    fvm_value_number(out,0);return scene_flush_failure?fvm_exception_raise(c,out):FVM_OK;
}
static void scene_flush_case(FVM_Runtime *rt,unsigned version)
{
    FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
    assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
    FVM_Object *child=0,*root=display_setup(c,&child);int removed=0;
    assert(fvm_object_delete(c,child,"_x",&removed)==FVM_OK && removed && fvm_object_define_accessor_rooted(c,child,"_x",scene_read_x,0,0,0)==FVM_OK);
    assert(fvm_exec_drag_begin(c,child,1,0)==FVM_OK && fvm_input_set_pointer(c,8,9)==FVM_OK);
    scene_flush_reads=0;scene_flush_failure=true;Program failed;failed.number(77);failed.op(0x28);scene_run(c,root,failed,FVM_EXCEPTION);
    assert(scene_flush_reads && c->drag_clip==child && !c->drag_updating);
    scene_flush_failure=false;Program retry;retry.number(77);retry.op(0x28);scene_run(c,root,retry);
    assert(!c->drag_clip && !c->drag_updating);scene_position(c,child,8,9);fvm_context_destroy(c);
}
#ifndef FVM_SCENE_CALLBACK_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)for(unsigned kind=0;kind<3;++kind)
    {
        if(version && version<4)continue;scene_cloning=kind==0;scene_detach=kind==2;unsigned count=scene_cloning?3:7;
        if(!kind)scene_flush_case(rt,version);
        for(scene_failure=0;scene_failure<=count;++scene_failure)
        {
            FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
            assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
            FVM_Object *child=0,*root=display_setup(c,&child);scene_clip=child;FVM_Value value;
            fvm_value_number(&value,17);assert(fvm_object_set(c,child,"alive",&value)==FVM_OK);
            if(scene_detach)
            {
                FVM_Value holder;int removed=0;assert(fvm_get_global(c,"clip",&holder)==FVM_OK && fvm_object_delete(c,holder.object,"native",&removed)==FVM_OK && removed && fvm_gc_unpin(c,child)==FVM_OK);
            }
            unsigned ids[]={1,2,3,4,5,6,7};for(unsigned i=0;i<count;++i)scene_coercion(c,ids+i);scene_sequence=0;
            Program p;p.number(77);
            if(scene_cloning){p.variable("g3");p.variable("g2");p.variable("g1");p.op(0x24);}
            else{for(unsigned i=7;i;--i){char key[16];std::sprintf(key,"g%u",i);p.variable(key);}p.op(0x27);}
            scene_run(c,root,p,scene_failure?FVM_EXCEPTION:FVM_OK);assert(scene_sequence==(scene_failure?scene_failure:count));
            if(!scene_failure && !scene_cloning){assert(c->drag_clip==child && c->drag_lock_center && c->drag_constrained);assert(fvm_gc_collect(c,0)==FVM_OK && fvm_object_get(c,child,"alive",&value)==FVM_OK && value.number==17);}
            assert(fvm_exec_drag_end(c)==FVM_OK);fvm_context_destroy(c);
        }
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
