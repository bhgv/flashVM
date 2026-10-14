/* TargetPath boxes primitive values, retains returned clips during GC and propagates exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static bool raising;
static unsigned boxes;
static FVM_Result box_path(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++boxes;assert(frame->argc==1 && frame->argv[0].number==17 && fvm_gc_collect(c,0)==FVM_OK);
    if(raising){fvm_value_number(out,77);return fvm_exception_raise(c,out);}
    fvm_value_undefined(out);out->type=FVM_VALUE_OBJECT;out->object=frame->capture;return FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;FVM_Object *root=0,*fn=0;FVM_Value args[2],clip,input,out;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);
        fvm_value_string(args,"child");fvm_value_number(args+1,1);
        assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&clip)==FVM_OK);
        const unsigned char body[]={0};
        assert(fvm_function_create_avm1(c,body,sizeof(body),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,box_path,0,clip.object)==FVM_OK);
        fvm_value_undefined(&input);input.type=FVM_VALUE_OBJECT;input.object=fn;
        assert(fvm_set_global(c,"Number",&input)==FVM_OK);
        FVM_GCRootFrame *roots=c->gc_frames;raising=false;fvm_value_number(&input,17);
        assert(fvm_exec_target_path(c,&input,&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"_level0.child"));
        Bytes code;text(code,"result");integer(code,17);code.push_back(0x45);code.push_back(0x1d);code.push_back(0);
        assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK && fvm_get_global(c,"result",&out)==FVM_OK);
        assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"_level0.child"));
        raising=true;
        assert(fvm_exec_target_path(c,&input,&input)==FVM_EXCEPTION && input.type==FVM_VALUE_NUMBER && input.number==17);
        assert(c->gc_frames==roots && fvm_exception_clear(c)==FVM_OK);raising=false;
        Bytes empty;empty.push_back(0x45);text(empty,"empty_result");empty.push_back(0x4d);empty.push_back(0x1d);empty.push_back(0);
        assert(fvm_avm1_execute(c,empty.begin(),empty.size())==FVM_OK && fvm_get_global(c,"empty_result",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        fvm_context_destroy(c);
    }
    assert(boxes==15);fvm_runtime_destroy(rt);return 0;
}
