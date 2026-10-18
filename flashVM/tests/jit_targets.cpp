/* Native target selection matches versioned interpretation and restores suspended target scopes. */
#define main target_reference_fixture_main
#include "jit_references.cpp"
#undef main
#include "flashvm/movieclip.h"
#include "flashvm/exception.h"
#include <cstdio>
static FVM_Object *base_expected,*target_expected;
static bool target_throws;
static unsigned probes;
static FVM_Result target_probe(FVM_Context *c,FVM_Object *self,unsigned argc,const FVM_Value *,
                               FVM_Value *out,void *)
{
    assert(!argc && c->avm1_base_clip==base_expected && c->avm1_target_clip==target_expected);
    if(c->swf_loaded && c->swf_info.version>=5)
        assert(self==(target_expected?target_expected:base_expected) && c->script_this==base_expected);
    ++probes;fvm_value_number(out,42);
    return target_throws?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Result target_path(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{
    FVM_Result status=fvm_gc_collect(c,0);return status==FVM_OK?fvm_value_string(out,"alias"):status;
}
static FVM_Object *target_setup(FVM_Context *c)
{
    FVM_Object *root=0;
    assert(fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK);
    assert(fvm_gc_pin(c,root)==FVM_OK);
    assert(fvm_register_function_rooted(c,"probe",target_probe,0,0)==FVM_OK);return root;
}
static FVM_Object *target_child(FVM_Context *c,FVM_Object *root,const char *name,unsigned depth)
{
    FVM_Value args[2],out;fvm_value_string(args,name);fvm_value_number(args+1,depth);
    assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);return out.object;
}
static void target_literal(Program &p,const char *path)
{
    size_t n=std::strlen(path)+1;p.op(0x8b);p.op((unsigned char)n);p.op((unsigned char)(n>>8));
    for(size_t i=0;i<n;++i)p.op((unsigned char)path[i]);
}
static void target_finish(Program &p)
{p.number(0);p.text("probe");p.op(0x3d);p.finish();}
#if !defined(FVM_TARGET_FIXTURE_ONLY) || defined(FVM_TARGET_COMPARE_FIXTURE)
static void target_compare(FVM_Context *c,FVM_Object *root,const Program &p,FVM_Object *selected,
                           FVM_Result expected=FVM_OK,FVM_Object *coercion=0)
{
    FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
    const char *parameters[]={"input"};FVM_Value arg;fvm_value_undefined(&arg);
    arg.type=FVM_VALUE_OBJECT;arg.object=coercion;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),1,parameters,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        assert(fvm_execution_set_mode(c,mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_set_body(c,fn,mode==1?fvm_jit_body:0,mode==1?code:0,0)==FVM_OK);
        FVM_GCRootFrame *roots=c->gc_frames;unsigned long entries=c->execution_mode.stats.native_entries;
        base_expected=root;target_expected=selected;probes=0;
        FVM_Result status=fvm_function_invoke(c,fn,root,coercion?1:0,coercion?&arg:0,&out);
        if(status!=expected)std::fprintf(stderr,"target version=%u mode=%u status=%d expected=%d first=%u probes=%u throws=%d\n",c->swf_loaded?c->swf_info.version:0,mode,status,expected,p.bytes[0],probes,target_throws);
        assert(status==expected);
        assert(c->execution_mode.stats.native_entries==entries+(mode!=0));
        assert(c->gc_frames==roots && !c->script_depth && !c->avm1_target_active &&
               !c->avm1_base_clip && !c->avm1_target_clip && !code->active);
        if(expected==FVM_OK)assert(probes==1 && out.type==FVM_VALUE_NUMBER && out.number==42);
        if(expected==FVM_EXCEPTION)assert(probes==1 && fvm_exception_clear(c)==FVM_OK);
    }
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
}
#endif
#ifndef FVM_TARGET_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_Object *root=target_setup(c),*child=target_child(c,root,"child",2);
        FVM_Value alias;fvm_value_undefined(&alias);alias.type=FVM_VALUE_OBJECT;alias.object=child;
        assert(fvm_object_set(c,root,"alias",&alias)==FVM_OK);
        Program literal;target_literal(literal,"alias");target_finish(literal);
        target_compare(c,root,literal,child);target_throws=true;
        target_compare(c,root,literal,child,FVM_EXCEPTION);target_throws=false;
        Program dynamic;dynamic.text("child");dynamic.op(0x20);target_finish(dynamic);
        target_compare(c,root,dynamic,child);
        FVM_Object *conversion=0,*coercion=0;const unsigned char empty[]={0};
        assert(fvm_object_create(c,&coercion)==FVM_OK && fvm_gc_pin(c,coercion)==FVM_OK);
        assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&conversion)==FVM_OK);
        assert(fvm_function_set_body(c,conversion,target_path,0,0)==FVM_OK);alias.object=conversion;
        assert(fvm_object_set(c,coercion,"toString",&alias)==FVM_OK);
        Program converted;converted.variable("input");converted.op(0x20);target_finish(converted);
        target_compare(c,root,converted,child,FVM_OK,coercion);
        Program reset;target_literal(reset,"child");target_literal(reset,"");target_finish(reset);
        target_compare(c,root,reset,root);
        Program missing;target_literal(missing,"missing");target_finish(missing);
        target_compare(c,root,missing,0);
        Program absent;absent.op(0x20);target_finish(absent);
        target_compare(c,root,absent,version && version<=6?root:0,version?FVM_OK:FVM_ERROR);
        assert(fvm_exec_detach_clip(c,child)==FVM_OK);
        target_compare(c,root,literal,version?0:child);
        fvm_context_destroy(c);
    }
    const unsigned char bad[][7]={{0x8b,0,0,0},{0x8b,1,0,'x',0},{0x8b,3,0,'x',0,0,0}};
    const size_t sizes[]={4,5,7};FVM_JitCode *code=0;
    for(unsigned i=0;i<3;++i)assert(fvm_jit_compile(bad[i],sizes[i],&code)==FVM_INVALID_ARGUMENT && !code);
    fvm_runtime_destroy(rt);return 0;
}
#endif
