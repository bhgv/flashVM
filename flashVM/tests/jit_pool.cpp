/* Native pool selection follows control flow and captured activations, including shared code caches. */
#define main jit_reference_fixture_main
#include "jit_references.cpp"
#undef main
#include "../src/standard/function_state.hpp"

static void ui16(Program &p,size_t n){p.op((unsigned char)n);p.op((unsigned char)(n>>8));}
static void pool(Program &p,const char *text)
{
    p.op(0x88);ui16(p,std::strlen(text)+3);ui16(p,1);
    for(size_t i=0;i<=std::strlen(text);++i)p.op((unsigned char)text[i]);
}
static void reference(Program &p,unsigned index,bool wide=false)
{
    p.op(0x96);ui16(p,wide?3:2);p.op(wide?9:8);p.op((unsigned char)index);
    if(wide)p.op((unsigned char)(index>>8));
}
static size_t branch(Program &p,unsigned char op)
{
    p.op(op);ui16(p,2);size_t at=p.bytes.size();ui16(p,0);return at;
}
static void patch(Program &p,size_t at,size_t target)
{
    size_t delta=target-at-2;p.bytes[at]=(unsigned char)delta;p.bytes[at+1]=(unsigned char)(delta>>8);
}
static FVM_Object *function(FVM_Context *c,const Program &p,const char *captured)
{
    FVM_ScriptFunction definition;definition.body=p.bytes;
    if(captured)definition.constants.push_back(fvm_detail::string(captured));
    FVM_Object *fn=0;assert(fvm_function_make_script(c,definition,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK);return fn;
}
static void compare(FVM_Context *c,const Program &p,const char *captured,const char *expected,
                    FVM_Result status=FVM_OK)
{
    FVM_Object *fn=function(c,p,captured);FVM_JitCode *native=0;FVM_Value out;
    assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&native)==FVM_OK);
    for(unsigned mode=0;mode<3;++mode)
    {
        assert(fvm_execution_set_mode(c,mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_set_body(c,fn,mode==1?fvm_jit_body:0,mode==1?native:0,0)==FVM_OK);
        unsigned long before=c->execution_mode.stats.native_entries;
        assert(fvm_function_invoke(c,fn,0,0,0,&out)==status);
        assert(c->execution_mode.stats.native_entries==before+(mode!=0));
        assert(!c->execution_mode.native_pool);
        if(status==FVM_OK)
        {
            if(expected)assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,expected));
            else assert(out.type==FVM_VALUE_UNDEFINED);
        }
    }
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(native)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        Program inherited;reference(inherited,0);inherited.finish();
        compare(c,inherited,"captured","captured");
        compare(c,inherited,"","");
        compare(c,inherited,0,0,version?FVM_OK:FVM_INVALID_ARGUMENT);
        Program replacement;pool(replacement,"selected");reference(replacement,0,true);replacement.finish();
        compare(c,replacement,"captured","selected");
        Program empty_text;pool(empty_text,"");reference(empty_text,0);empty_text.finish();
        compare(c,empty_text,0,"");
        Program skipped;pool(skipped,"first");size_t jump=branch(skipped,0x99);
        pool(skipped,"wrong");patch(skipped,jump,skipped.bytes.size());reference(skipped,0);skipped.finish();
        compare(c,skipped,0,"first");
        Program backward;pool(backward,"first");backward.number(0);
        backward.op(0x87);ui16(backward,1);backward.op(0);backward.op(0x17);
        size_t again=backward.bytes.size();reference(backward,0);
        backward.op(0x96);ui16(backward,2);backward.op(4);backward.op(0);
        size_t done=branch(backward,0x9d);backward.op(0x17);pool(backward,"second");
        backward.number(1);backward.op(0x87);ui16(backward,1);backward.op(0);backward.op(0x17);
        size_t back=branch(backward,0x99);patch(backward,back,again);patch(backward,done,backward.bytes.size());
        backward.finish();compare(c,backward,0,"second");
        Program empty;empty.op(0x88);ui16(empty,2);ui16(empty,0);reference(empty,0);empty.finish();
        compare(c,empty,"captured",0,version?FVM_OK:FVM_INVALID_ARGUMENT);
        Program missing;pool(missing,"first");reference(missing,65535,true);missing.finish();
        compare(c,missing,0,0,version?FVM_OK:FVM_INVALID_ARGUMENT);
        Program mixed;pool(mixed,"A");mixed.op(0x96);ui16(mixed,5);mixed.op(8);mixed.op(0);
        mixed.op(0);mixed.op('!');mixed.op(0);mixed.op(0x47);mixed.finish();compare(c,mixed,0,"A!");
        Program wide;wide.op(0x88);ui16(wide,519);ui16(wide,257);
        for(unsigned i=0;i<256;++i){wide.op('x');wide.op(0);}
        wide.op('l');wide.op('a');wide.op('s');wide.op('t');wide.op(0);
        /* 2 count bytes + 512 prefix bytes + 5 final bytes. */
        reference(wide,256,true);wide.finish();compare(c,wide,0,"last");
        FVM_Object *a=function(c,inherited,"A"),*b=function(c,inherited,"B");FVM_Value out;
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK);
        for(unsigned i=0;i<4;++i)
            assert(fvm_function_invoke(c,i%2?b:a,0,0,0,&out)==FVM_OK &&
                   out.type==FVM_VALUE_STRING && !std::strcmp(out.string,i%2?"B":"A"));
        assert(!c->execution_mode.native_pool);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
