/* Native register banks match legacy/Function2 interpretation, root entry and loaded bounds. */
#define main jit_reference_fixture_main
#include "jit_references.cpp"
#undef main
#include "../src/standard/function_state.hpp"
#include "flashvm/execution_mode.h"

static void load(Program &p,unsigned index){p.op(0x96);p.op(2);p.op(0);p.op(4);p.op((unsigned char)index);}
static void store(Program &p,unsigned index){p.op(0x87);p.op(1);p.op(0);p.op((unsigned char)index);}
static void check(FVM_Context *c,const Program &p,unsigned count,unsigned flags,FVM_Object *receiver,
                  FVM_Result expected,FVM_ValueType type,double number,FVM_Object *object=0,
                  bool force_function2=false)
{
    FVM_ScriptFunction definition;definition.body=p.bytes;definition.function2=count!=0 || force_function2;
    definition.register_count=count;definition.flags=flags;
    definition.parameters.push_back(fvm_detail::string("x"));definition.parameter_registers.push_back(count>7?7:0);
    FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value arg,out;
    assert(fvm_function_make_script(c,definition,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
    assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);fvm_value_number(&arg,42);
    for(unsigned mode=0;mode<3;++mode)
    {
        assert(fvm_execution_set_mode(c,mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_function_set_body(c,fn,mode==1?fvm_jit_body:0,mode==1?code:0,0)==FVM_OK);
        unsigned long before=c->execution_mode.stats.native_entries;
        assert(fvm_function_invoke(c,fn,receiver,1,&arg,&out)==expected);
        assert(c->execution_mode.stats.native_entries==before+(mode!=0));
        if(expected==FVM_OK)
        {
            assert(out.type==type);
            if(type==FVM_VALUE_NUMBER)assert(out.number==number);
            if(type==FVM_VALUE_OBJECT)assert(out.object==object);
            if(type==FVM_VALUE_STRING)assert(!std::strcmp(out.string,"register-text"));
        }
    }
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;FVM_Object *receiver=0;FVM_Value value,out;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(rt,&c)==FVM_OK);
        if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
        assert(fvm_standard_install(c)==FVM_OK && fvm_construct(c,"Object",0,0,&receiver)==FVM_OK);
        assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        assert(fvm_gc_pin(c,receiver)==FVM_OK);
        Program simple;simple.number(42);store(simple,1);simple.op(0x17);load(simple,1);simple.finish();
        check(c,simple,0,0,receiver,FVM_OK,FVM_VALUE_NUMBER,42);
        check(c,simple,0,0,receiver,version?FVM_OK:FVM_INVALID_ARGUMENT,FVM_VALUE_NUMBER,42,0,true);
        Program keep;keep.number(42);store(keep,1);keep.finish();
        check(c,keep,0,0,receiver,FVM_OK,FVM_VALUE_NUMBER,42);
        Program mixed;mixed.number(42);store(mixed,1);mixed.op(0x17);
        const unsigned char operands[]={0x96,7,0,4,1,7,2,0,0,0,0x0b};
        for(size_t i=0;i<sizeof(operands);++i)mixed.op(operands[i]);
        mixed.finish();check(c,mixed,0,0,receiver,FVM_OK,FVM_VALUE_NUMBER,40);
        Program text;text.text("register-text");store(text,1);text.op(0x17);load(text,1);text.finish();
        check(c,text,0,0,receiver,FVM_OK,FVM_VALUE_STRING,0);
        assert(fvm_gc_collect(c,0)==FVM_OK);
        if(version)assert(fvm_exec_global_register(c,0,1,0,&out)==FVM_OK &&
                          out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"register-text"));
        const unsigned invalid[]={4,255};
        for(size_t i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i)
        {
            Program p;p.number(42);store(p,invalid[i]);p.op(0x17);load(p,invalid[i]);p.finish();
            check(c,p,0,0,receiver,version?FVM_OK:FVM_INVALID_ARGUMENT,FVM_VALUE_UNDEFINED,0);
            Program read;load(read,invalid[i]);read.finish();
            check(c,read,0,0,receiver,version?FVM_OK:FVM_INVALID_ARGUMENT,FVM_VALUE_UNDEFINED,0);
        }
        Program missing;store(missing,1);load(missing,1);missing.finish();
        check(c,missing,0,0,receiver,version?FVM_OK:FVM_ERROR,FVM_VALUE_UNDEFINED,0);
        Program parameter;load(parameter,7);store(parameter,2);parameter.op(0x17);load(parameter,2);parameter.finish();
        fvm_value_number(&value,99);assert(fvm_exec_global_register(c,1,2,&value,&out)==FVM_OK);
        check(c,parameter,8,0,receiver,FVM_OK,FVM_VALUE_NUMBER,42);
        assert(fvm_exec_global_register(c,0,2,0,&out)==FVM_OK && out.number==99);
        Program boundary;boundary.number(42);store(boundary,2);boundary.op(0x17);load(boundary,2);boundary.finish();
        check(c,boundary,2,0,receiver,FVM_OK,version?FVM_VALUE_UNDEFINED:FVM_VALUE_NUMBER,42);
        Program actual_this;load(actual_this,1);actual_this.finish();
        check(c,actual_this,8,1,receiver,FVM_OK,FVM_VALUE_OBJECT,0,receiver);
        check(c,actual_this,8,256,receiver,FVM_OK,FVM_VALUE_OBJECT,0,c->global);
        Program arguments;load(arguments,1);arguments.number(0);arguments.op(0x4e);arguments.finish();
        check(c,arguments,8,4,receiver,FVM_OK,FVM_VALUE_NUMBER,42);
        Program root;root.number(42);store(root,1);root.op(0x17);load(root,1);
        root.text("root_result");root.op(0x4d);root.op(0x1d);root.op(0);
        FVM_ExecutionOptions options;assert(fvm_execution_options_init(&options)==FVM_OK);
        for(unsigned mode=0;mode<2;++mode)
        {
            options.mode=mode?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER;
            unsigned long before=c->execution_mode.stats.native_entries;
            assert(fvm_avm1_execute_options(c,root.bytes.begin(),root.bytes.size(),&options)==FVM_OK);
            assert(c->execution_mode.stats.native_entries==before+mode);
            assert(fvm_get_global(c,"root_result",&out)==FVM_OK && out.number==42);
        }
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
        Program reader;load(reader,1);reader.finish();
        check(c,reader,0,0,receiver,FVM_OK,version?FVM_VALUE_NUMBER:FVM_VALUE_UNDEFINED,42);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
