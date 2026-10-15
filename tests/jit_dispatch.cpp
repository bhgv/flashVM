/* Native URL Actions preserve counted requests, host failures and loaded literal-byte policy. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/avm1.h"
static unsigned dispatch_deliveries;
static FVM_Result dispatch_status=FVM_OK;
static FVM_Result dispatch_sink(FVM_Context *c,FVM_Object *receiver,unsigned argc,const FVM_Value *args,FVM_Value *,void *)
{
    assert(receiver==c->global && argc==3 && args[0].type==FVM_VALUE_STRING && args[1].type==FVM_VALUE_STRING && args[2].type==FVM_VALUE_NUMBER);++dispatch_deliveries;
    if(dispatch_status!=FVM_OK)return dispatch_status==FVM_EXCEPTION?fvm_exception_raise(c,args):dispatch_status;
    FVM_Result status=fvm_set_global(c,"deliveredUrl",args);if(status==FVM_OK)status=fvm_set_global(c,"deliveredTarget",args+1);if(status==FVM_OK)status=fvm_set_global(c,"deliveredFlags",args+2);return status;
}
static void dispatch_literal(Program &p,const char *url,const char *target)
{
    size_t a=std::strlen(url)+1,b=std::strlen(target)+1,n=a+b;p.op(0x83);p.op((unsigned char)n);p.op((unsigned char)(n>>8));
    for(size_t i=0;i<a;++i)p.op((unsigned char)url[i]);for(size_t i=0;i<b;++i)p.op((unsigned char)target[i]);
}
static void dispatch_dynamic(Program &p,unsigned flags)
{p.op(0x9a);p.op(1);p.op(0);p.op((unsigned char)flags);}
static void dispatch_call(Program &p)
{p.op(0x9e);p.op(0);p.op(0);}
static void dispatch_run(FVM_Context *c,Program &p,FVM_Result expected=FVM_OK)
{p.finish();FVM_Value out=class_run(c,p,expected);if(expected==FVM_OK)assert(out.type==FVM_VALUE_NUMBER && out.number==77);}
#ifndef FVM_DISPATCH_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK && fvm_avm1_set_url_rooted(c,dispatch_sink,0,0)==FVM_OK);FVM_Value value;
        assert(fvm_value_string_bytes(&value,"u\0x",3)==FVM_OK && fvm_set_global(c,"url",&value)==FVM_OK);
        assert(fvm_value_string_bytes(&value,"t\0y",3)==FVM_OK && fvm_set_global(c,"target",&value)==FVM_OK);
        for(unsigned flags=0;flags<256;++flags)if(!(flags&~195U) && (flags&3)!=3)
        {
            dispatch_deliveries=0;Program p;p.number(77);p.variable("url");p.variable("target");dispatch_dynamic(p,flags);dispatch_run(c,p);assert(dispatch_deliveries==1);
            assert(fvm_gc_collect(c,0)==FVM_OK && fvm_get_global(c,"deliveredUrl",&value)==FVM_OK && fvm_value_string_size(&value)==3 && !std::memcmp(value.string,"u\0x",3));
            assert(fvm_get_global(c,"deliveredTarget",&value)==FVM_OK && fvm_value_string_size(&value)==3 && !std::memcmp(value.string,"t\0y",3));
            assert(fvm_get_global(c,"deliveredFlags",&value)==FVM_OK && value.number==flags);
        }
        Program literal;literal.number(77);dispatch_literal(literal,"FSCommand:quit","argument");dispatch_run(c,literal);
        assert(fvm_gc_collect(c,0)==FVM_OK && fvm_get_global(c,"deliveredUrl",&value)==FVM_OK && !std::strcmp(value.string,"FSCommand:quit"));
        for(unsigned side=0;side<2;++side)
        {
            dispatch_deliveries=0;Program p;p.number(77);dispatch_literal(p,side?"url":"\xc3\xa9",side?"\xc3\xa9":"target");
            bool legacy=version && version<6;dispatch_run(c,p,legacy?FVM_UNSUPPORTED:FVM_OK);assert(dispatch_deliveries==(legacy?0U:1U));
        }
        const FVM_Result failures[]={FVM_ERROR,FVM_EXCEPTION};
        for(unsigned i=0;i<2;++i)
        {dispatch_status=failures[i];dispatch_deliveries=0;Program p;p.number(77);p.text("url");p.text("target");dispatch_dynamic(p,0);dispatch_run(c,p,failures[i]);assert(dispatch_deliveries==1);}
        dispatch_status=FVM_OK;
        Program missing;dispatch_dynamic(missing,0);missing.number(77);dispatch_run(c,missing,version?FVM_OK:FVM_ERROR);
        assert(fvm_avm1_set_url_rooted(c,0,0,0)==FVM_OK);Program disabled;disabled.number(77);dispatch_literal(disabled,"url","target");dispatch_run(c,disabled,FVM_UNSUPPORTED);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
