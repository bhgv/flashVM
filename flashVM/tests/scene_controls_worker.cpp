/* Real workers observe cancellation while applying native-only authored controls. */
#include "scene_controls_fixture.hpp"
#include "flashvm/worker.h"
struct SceneStop { unsigned calls; };
static FVM_Result stop_scene(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{
    SceneStop *p=static_cast<SceneStop *>(raw);++p->calls;FVM_Worker *self=0;
    assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);
    fvm_value_undefined(out);return FVM_OK;
}
static FVM_Result worker_scene(FVM_Context *c,const FVM_Value *input,FVM_Value *out,void *)
{
    assert(fvm_standard_install(c)==FVM_OK);SceneControlProbe p;FVM_Object *root=scene_control_root(c,p);SceneStop state={0};
    FVM_Object *control=0;assert(fvm_object_create(c,&control)==FVM_OK);FVM_Value value;fvm_value_number(&value,9);control->properties["op"]=value;
    fvm_value_number(&value,0x112233);control->properties["background"]=value;
    for(unsigned i=1;i<200;++i)p.list->indexed[i]=tick_object(control);
    FVM_Object *fn=0;assert(fvm_function_create_native(c,stop_scene,&state,0,&fn)==FVM_OK);
    FVM_Value klass,mc,args[2],ignored;assert(fvm_get_global(c,"MovieClip",&mc)==FVM_OK);value=get(c,mc.object,"prototype");assert(fvm_object_set(c,fn,"prototype",&value)==FVM_OK);
    assert(fvm_get_global(c,"Object",&klass)==FVM_OK);assert(fvm_value_string(args,"Hero")==FVM_OK);args[1]=tick_object(fn);
    assert(fvm_object_call(c,klass.object,"registerClass",2,args,&ignored)==FVM_OK);
    if(!input->boolean){FVM_Worker *self=0;assert(fvm_worker_current(c,&self)==FVM_OK && fvm_worker_stop(self)==FVM_OK);}
    assert(fvm_movieclip_advance(c,root,1)==FVM_ERROR);tick_clean(c);
    assert(state.calls==(input->boolean?1U:0U));
    if(input->boolean)assert(c->avm1_steps>200 && c->avm1_steps<400);
    fvm_value_boolean(out,1);return FVM_OK;
}
int main()
{
    FVM_Config cfg;assert(fvm_config_init(&cfg)==FVM_OK);cfg.instruction_limit=0;
    for(unsigned i=0;i<2;++i){FVM_Value input,out;fvm_value_boolean(&input,i!=0);FVM_Worker *worker=0;
        assert(fvm_worker_run(&cfg,worker_scene,&input,0,&worker)==FVM_OK && fvm_worker_join(worker,&out)==FVM_OK && out.boolean);fvm_worker_destroy(worker);}
}
