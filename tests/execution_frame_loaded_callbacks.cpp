/* Frame-value coercion uses toString, roots receivers and restores callback target scope. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned mode,conversions;
static FVM_Result forbidden(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{assert(0);return FVM_ERROR;}
static FVM_Result forbidden_value(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *,void *)
{assert(0);return FVM_ERROR;}
static FVM_Result frame_text(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++conversions;assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value value;
    assert(fvm_object_get(c,frame->receiver,"alive",&value)==FVM_OK && value.number==17);
    fvm_value_string(&value,"child");assert(fvm_exec_select_target(c,&value)==FVM_OK);
    if(mode==2){fvm_value_number(out,77);return fvm_exception_raise(c,out);}
    return fvm_value_string(out,mode?"16001":"16002");
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;FVM_Object *root=0,*child=0;FVM_Value args[2],value;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",3,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        fvm_value_string(args,"child");fvm_value_number(args+1,1);
        assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&value)==FVM_OK);child=value.object;
        assert(fvm_object_define_accessor_rooted(c,child,"_framesloaded",forbidden,0,0,0)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,child,"_totalframes",forbidden,0,0,0)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,root,"_framesloaded",forbidden,0,0,0)==FVM_OK);
        assert(fvm_object_define_accessor_rooted(c,root,"_totalframes",forbidden,0,0,0)==FVM_OK);
        c->avm1_target_active=true;c->avm1_base_clip=root;
        for(mode=0;mode<3;++mode)
        {
            FVM_Object *object=0;FVM_Value input,out;
            assert(fvm_object_create(c,&object)==FVM_OK);fvm_value_number(&value,17);
            assert(fvm_object_set(c,object,"alive",&value)==FVM_OK);
            attach(c,object,"toString",frame_text,0);attach(c,object,"valueOf",forbidden_value,0);
            fvm_value_undefined(&input);input.type=FVM_VALUE_OBJECT;input.object=object;
            c->avm1_target_clip=root;FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);
            FVM_Result r=fvm_exec_frame_loaded(c,FVM_EXEC_FRAME_CHECK_VALUE,&input,&out);
            assert(c->gc_frames==roots && !c->string_depth && c->avm1_target_clip==root);
            if(mode==2)assert(r==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);
            else assert(r==FVM_OK && out.type==FVM_VALUE_BOOLEAN && out.boolean==(mode==1));
        }
        fvm_context_destroy(c);
    }
    assert(conversions==15);fvm_runtime_destroy(rt);return 0;
}
