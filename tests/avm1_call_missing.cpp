/* Missing fixed call operands preserve top-relative roles and zero-argument invocation. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned invoked;
static bool throw_call;
static FVM_Result zero_call(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    assert(frame->argc==0 && fvm_gc_collect(c,0)==FVM_OK);++invoked;
    fvm_value_number(out,42);if(throw_call)return fvm_exception_raise(c,out);return FVM_OK;
}
static FVM_Result forbidden_name(FVM_Context *,const FVM_ScriptFrame *,FVM_Value *,void *)
{assert(false);return FVM_ERROR;}
static void global_object(FVM_Context *c,const char *key,FVM_Object *o)
{FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;assert(fvm_set_global(c,key,&v)==FVM_OK);}
static void fetch(Bytes &b,const char *key)
{text(b,key);b.push_back(0x1c);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const unsigned char ops[]={0x3d,0x40,0x52,0x53};
    for(unsigned profile=0;profile<6;++profile)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile+3);
        assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        const unsigned char empty[]={0};FVM_Object *fn=0,*name_obj=0;assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,zero_call,0,0)==FVM_OK);global_object(c,"callee",fn);
        assert(fvm_object_create(c,&name_obj)==FVM_OK);attach(c,name_obj,"toString",forbidden_name,0);global_object(c,"nameObject",name_obj);
        for(unsigned op=0;op<4;++op)for(unsigned provided=0;provided<(op<2?2:3);++provided)
        {
            Bytes b;
            if(op<2){if(provided)text(b,"callee");}
            else if(provided==1)fetch(b,"nameObject");
            else if(provided==2){fetch(b,"callee");b.push_back(0x96);u16(b,1);b.push_back(3);}
            b.push_back(ops[op]);text(b,"result");b.push_back(0x4d);b.push_back(0x1d);b.push_back(0);
            invoked=0;throw_call=false;FVM_GCRootFrame *roots=c->gc_frames;FVM_Result r=fvm_avm1_execute(c,b.begin(),b.size());
            assert(c->gc_frames==roots && !c->avm1_execution_depth && !c->script_depth);
            if(!profile){assert(r==FVM_ERROR && !invoked);continue;}
            assert(r==FVM_OK);FVM_Value out;assert(fvm_get_global(c,"result",&out)==FVM_OK);
            bool called=op<2?provided!=0:provided==2;assert(invoked==(called?1u:0u));
            if(!called)assert(out.type==FVM_VALUE_UNDEFINED);
            else if(op==0 || op==2)assert(out.type==FVM_VALUE_NUMBER && out.number==42);
            else assert(out.type==FVM_VALUE_OBJECT);
            if(called){throw_call=true;invoked=0;r=fvm_avm1_execute(c,b.begin(),b.size());assert(r==FVM_EXCEPTION && invoked==1 && c->gc_frames==roots && !c->avm1_execution_depth && !c->script_depth);assert(fvm_exception_clear(c)==FVM_OK);throw_call=false;}
            assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
