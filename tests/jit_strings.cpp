/* Native string Actions match interpretation in host/SWF4--8 with exact UTF-16 snapshots. */
#define main string_reference_fixture_main
#include "jit_references.cpp"
#undef main
#include "flashvm/gc_roots.h"
#include "flashvm/exception.h"
static void units(const FVM_Value &value,fvm_detail::vector<unsigned short> &out)
{
    size_t count=0;assert(fvm_value_get_utf16(&value,0,0,&count)==FVM_OK);out.resize(count);
    assert(fvm_value_get_utf16(&value,out.begin(),count,&count)==FVM_OK);
}
static bool equal_units(const fvm_detail::vector<unsigned short> &a,const fvm_detail::vector<unsigned short> &b)
{
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i)if(a[i]!=b[i])return false;
    return true;
}
static void string_check(FVM_Context *c,const Program &p,unsigned argc,const FVM_Value *args,
                         const FVM_Value *expected=0,FVM_Result expected_status=FVM_OK)
{
    const char *params[]={"a","b","d"};FVM_Object *fn=0;FVM_JitCode *code=0;
    FVM_GCRootScope *roots=0;assert(fvm_gc_roots_begin(c,&roots)==FVM_OK);
    for(unsigned i=0;i<argc;++i)assert(fvm_gc_roots_add(roots,args+i)==FVM_OK);
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),3,params,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    FVM_Value baseline,out;FVM_Result first=FVM_ERROR;fvm_detail::vector<unsigned short> saved,actual;
    for(unsigned mode=0;mode<3;++mode)
    {
        assert(fvm_execution_set_mode(c,mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_set_body(c,fn,mode==1?fvm_jit_body:0,mode==1?code:0,0)==FVM_OK);
        unsigned long before=c->execution_mode.stats.native_entries;
        fvm_value_undefined(&out);
        FVM_Result status=fvm_function_invoke(c,fn,0,argc,args,&out);
        assert(c->execution_mode.stats.native_entries==before+(mode!=0));
        if(!mode){first=status;baseline=out;if(status==FVM_OK && out.type==FVM_VALUE_STRING)units(out,saved);}
        else
        {
            assert(status==first);
            if(status==FVM_OK)
            {
                assert(out.type==baseline.type);
                if(out.type==FVM_VALUE_NUMBER)assert(out.number==baseline.number || (out.number!=out.number && baseline.number!=baseline.number));
                if(out.type==FVM_VALUE_BOOLEAN)assert(out.boolean==baseline.boolean);
                if(out.type==FVM_VALUE_STRING){units(out,actual);assert(equal_units(actual,saved));}
            }
        }
        if(expected)
        {
            assert(status==expected_status);
            if(status==FVM_OK)
            {
                assert(out.type==expected->type);
                if(out.type==FVM_VALUE_NUMBER)assert(out.number==expected->number);
                if(out.type==FVM_VALUE_BOOLEAN)assert(out.boolean==expected->boolean);
                if(out.type==FVM_VALUE_STRING){units(*expected,actual);assert(equal_units(actual,saved));}
            }
        }
        if(status==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);
        assert(!c->script_depth && !c->gc_native_depth && !code->active);
    }
    if(first==FVM_OK)assert(fvm_gc_roots_add(roots,&out)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    if(first==FVM_OK && out.type==FVM_VALUE_STRING){units(out,actual);assert(equal_units(actual,saved));}
    assert(fvm_gc_roots_end(roots)==FVM_OK);
}
static Program string_program(unsigned char op,unsigned operands)
{
    Program p;const char *keys[]={"a","b","d"};
    for(unsigned i=0;i<operands;++i)p.variable(keys[i]);
    p.op(op);p.finish();return p;
}
#ifndef FVM_STRING_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    const unsigned char operations[]={0x14,0x31,0x32,0x36,0x33,0x37,0x15,0x35,0x21,0x13,0x29,0x68};
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        for(unsigned i=0;i<sizeof(operations);++i)
        {
            unsigned operands=i<6?1:i<8?3:2;FVM_Value args[3],expected;
            fvm_value_string(args,"abcd");fvm_value_string(args+1,"b");fvm_value_number(args+2,2);
            if(i<2)fvm_value_number(&expected,4);
            else if(i<4)fvm_value_number(&expected,97);
            else if(i<6){fvm_value_number(args,65);fvm_value_string(&expected,"A");}
            else if(i<8){fvm_value_number(args+1,2);fvm_value_string(&expected,"bc");}
            else if(i==8)fvm_value_string(&expected,"abcdb");
            else fvm_value_boolean(&expected,i==10);
            Program p=string_program(operations[i],operands);string_check(c,p,operands,args,&expected);
            Program missing;missing.op(operations[i]);missing.finish();string_check(c,missing,0,0);
        }
        const unsigned short data[]={'A',0,0xd83d,0xde00,0xd800,'Z'};
        FVM_Value args[3],expected;assert(fvm_value_string_utf16(c,data,6,args)==FVM_OK);
        FVM_GCRootScope *hold=0;assert(fvm_gc_roots_begin(c,&hold)==FVM_OK && fvm_gc_roots_add(hold,args)==FVM_OK);
        fvm_value_number(&expected,6);string_check(c,string_program(0x14,1),1,args,&expected,
                                                 version && version<6?FVM_UNSUPPORTED:FVM_OK);
        fvm_value_number(args+1,2);fvm_value_number(args+2,3);
        assert(fvm_value_string_utf16(c,data+1,3,&expected)==FVM_OK && fvm_gc_roots_add(hold,&expected)==FVM_OK);
        string_check(c,string_program(0x35,3),3,args,&expected,version && version<6?FVM_UNSUPPORTED:FVM_OK);
        unsigned short joined[7];for(unsigned i=0;i<6;++i)joined[i]=data[i];joined[6]='!';
        fvm_value_string(args+1,"!");assert(fvm_value_string_utf16(c,joined,7,&expected)==FVM_OK);
        assert(fvm_gc_roots_add(hold,&expected)==FVM_OK);string_check(c,string_program(0x21,2),2,args,&expected);
        const unsigned short supplementary[]={0xd83d,0xde00},bmp[]={0xe000};
        assert(fvm_value_string_utf16(c,supplementary,2,args)==FVM_OK && fvm_gc_roots_add(hold,args)==FVM_OK);
        assert(fvm_value_string_utf16(c,bmp,1,args+1)==FVM_OK && fvm_gc_roots_add(hold,args+1)==FVM_OK);
        fvm_value_boolean(&expected,1);string_check(c,string_program(0x29,2),2,args,&expected);
        fvm_value_number(args,0);fvm_value_string(&expected,"");string_check(c,string_program(0x33,1),1,args,&expected);
        const unsigned short replacement[]={0xfffd};fvm_value_number(args,0xd800);
        assert(fvm_value_string_utf16(c,replacement,1,&expected)==FVM_OK && fvm_gc_roots_add(hold,&expected)==FVM_OK);
        string_check(c,string_program(0x37,1),1,args,&expected,version && version<6?FVM_UNSUPPORTED:FVM_OK);
        assert(fvm_gc_roots_end(hold)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
