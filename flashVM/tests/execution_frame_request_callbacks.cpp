/* Frame coercion collects through toString only, preserves aliases and restores roots on throws. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned strings,numbers;static bool fail_request;
static FVM_Result string_request(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{++strings;assert(fvm_gc_collect(c,0)==FVM_OK);FVM_Value alive;assert(fvm_object_get(c,frame->receiver,"alive",&alive)==FVM_OK && alive.number==17);if(fail_request){fvm_value_number(out,77);return fvm_exception_raise(c,out);}return fvm_value_string(out,"002");}
static FVM_Result number_request(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *out,void *)
{++numbers;fvm_value_number(out,99);return FVM_OK;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        for(unsigned throwing=0;throwing<2;++throwing)
        {
            FVM_Object *obj=0;assert(fvm_object_create(c,&obj)==FVM_OK);FVM_Value value;fvm_value_number(&value,17);assert(fvm_object_set(c,obj,"alive",&value)==FVM_OK);
            attach(c,obj,"toString",string_request,0);attach(c,obj,"valueOf",number_request,0);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=obj;
            fail_request=throwing!=0;strings=numbers=0;FVM_GCRootFrame *roots=c->gc_frames;
            FVM_Result r=fvm_exec_frame_request(c,&value,1,3,&value);assert(c->gc_frames==roots && strings==1 && !numbers);
            if(throwing){assert(r==FVM_EXCEPTION && value.type==FVM_VALUE_OBJECT && value.object==obj);assert(fvm_exception_clear(c)==FVM_OK);}else assert(r==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==5);
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
