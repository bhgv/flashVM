/* Native clone/remove/drag Actions match isolated host and loaded interpreter runs. */
#define FVM_DISPLAY_FIXTURE_ONLY
#include "jit_display.cpp"
#include "flashvm/input.h"
static FVM_Result scene_forbidden(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{assert(false);return FVM_ERROR;}
static FVM_Object *scene_child(FVM_Context *c,FVM_Object *root,const char *name,int depth)
{
    FVM_Value args[2],out;fvm_value_string(args,name);fvm_value_number(args+1,depth);
    assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK && fvm_gc_pin(c,out.object)==FVM_OK);return out.object;
}
static void scene_position(FVM_Context *c,FVM_Object *clip,double x,double y)
{FVM_Affine matrix;assert(fvm_movieclip_get_matrix(c,clip,&matrix)==FVM_OK && matrix.x0==x && matrix.y0==y);}
static void scene_run(FVM_Context *c,FVM_Object *root,Program &p,FVM_Result expected=FVM_OK)
{p.finish();FVM_Value out=class_run(c,p,expected,root);if(expected==FVM_OK)assert(out.type==FVM_VALUE_NUMBER && out.number==77);}
static void scene_drag(Program &p,bool bounded)
{
    if(bounded){p.number(0);p.number(0);p.number(10);p.number(10);}
    p.number(bounded?1:2);p.number(bounded?1:2);p.text("child");p.op(0x27);
}
#ifndef FVM_SCENE_FIXTURE_ONLY
static FVM_Object *scene_expected;
static unsigned scene_probes;
static FVM_Result scene_probe(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{assert(c->avm1_target_clip==scene_expected && c->avm1_base_clip==scene_expected);++scene_probes;return FVM_OK;}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_Object *child=0,*root=display_setup(c,&child);FVM_Value value,out;
        const char *methods[]={"duplicateMovieClip","removeMovieClip","startDrag","stopDrag"};
        for(unsigned i=0;i<4;++i)assert(fvm_object_define_method_rooted(c,child,methods[i],scene_forbidden,0,0)==FVM_OK);
        fvm_value_number(&value,17);assert(fvm_object_set(c,child,"_x",&value)==FVM_OK && fvm_object_set(c,child,"custom",&value)==FVM_OK);
        Program clone;clone.number(77);clone.text("child");clone.text("copy");clone.number(16387);clone.op(0x24);scene_run(c,root,clone);
        assert(fvm_object_get(c,root,"copy",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT);FVM_Object *copy=out.object;
        assert(fvm_gc_pin(c,copy)==FVM_OK && fvm_object_get(c,copy,"_x",&out)==FVM_OK && out.number==17);
        assert(fvm_object_get(c,copy,"custom",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        assert(fvm_object_call(c,copy,"getDepth",0,0,&out)==FVM_OK && out.number==3);
        FVM_Value holder;assert(fvm_get_global(c,"clip",&holder)==FVM_OK);value=class_value(copy);assert(fvm_object_set(c,holder.object,"native",&value)==FVM_OK);
        Program remove;remove.number(77);display_target(remove);remove.op(0x25);scene_run(c,root,remove);
        assert(fvm_exec_clip_detached(c,copy,&out)==FVM_OK && out.boolean);
        FVM_Object *fixed=scene_child(c,root,"fixed",-1);Program keep;keep.number(77);keep.text("fixed");keep.op(0x25);scene_run(c,root,keep);
        assert(fvm_exec_clip_detached(c,fixed,&out)==FVM_OK && !out.boolean);
        FVM_Affine initial={1,0,0,1,5,7};assert(fvm_movieclip_set_matrix(c,child,&initial)==FVM_OK);
        Program drag;drag.number(77);scene_drag(drag,true);scene_run(c,root,drag);
        assert(fvm_exec_drag_target(c,&out)==FVM_OK && out.object==child && c->drag_lock_center && c->drag_constrained);scene_position(c,child,5,7);
        assert(fvm_input_set_pointer(c,30,40)==FVM_OK && fvm_exec_drag_update(c)==FVM_OK);scene_position(c,child,10,10);
        assert(fvm_input_set_pointer(c,8,9)==FVM_OK);Program end;end.number(77);end.op(0x28);scene_run(c,root,end);scene_position(c,child,8,9);
        assert(fvm_exec_drag_target(c,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        assert(fvm_movieclip_set_matrix(c,child,&initial)==FVM_OK && fvm_input_set_pointer(c,0,0)==FVM_OK);
        Program free;free.number(77);scene_drag(free,false);scene_run(c,root,free);
        assert(!c->drag_lock_center && !c->drag_constrained && fvm_input_set_pointer(c,4,6)==FVM_OK && fvm_exec_drag_update(c)==FVM_OK);scene_position(c,child,9,13);
        assert(fvm_exec_drag_end(c)==FVM_OK);
        scene_expected=root;scene_probes=0;assert(fvm_avm1_set_trace_rooted(c,scene_probe,0,0)==FVM_OK);
        Program selected;selected.number(77);selected.text("child");selected.op(0x20);selected.text("");selected.op(0x25);selected.text("probe");selected.op(0x26);scene_run(c,root,selected);
        assert(scene_probes==1 && fvm_exec_clip_detached(c,child,&out)==FVM_OK && out.boolean);
        assert(fvm_avm1_set_trace_rooted(c,0,0,0)==FVM_OK);
        const unsigned char missing[]={0x24,0x25,0x27};
        for(unsigned i=0;i<3;++i){Program p;p.op(missing[i]);p.number(77);scene_run(c,root,p,version?FVM_OK:FVM_ERROR);assert(fvm_exec_drag_end(c)==FVM_OK);}
        assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
