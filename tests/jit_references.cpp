/* Real native reference blocks match interpreter results in host and SWF4--8 profiles. */
#include "../src/jit/internal.hpp"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>

struct Program
{
    fvm_detail::vector<unsigned char> bytes;
    void op(unsigned char value) { bytes.push_back(value); }
    void text(const char *value)
    {
        size_t n=std::strlen(value)+2; op(0x96);op((unsigned char)n);op((unsigned char)(n>>8));op(0);
        for(size_t i=0;i+1<n;++i)op((unsigned char)value[i]);
    }
    void number(unsigned value)
    {
        op(0x96);op(5);op(0);op(7);
        for(unsigned i=0;i<4;++i)op((unsigned char)(value>>(i*8)));
    }
    void variable(const char *value) { text(value);op(0x1c); }
    void finish() { op(0x3e);op(0); }
};

static void execute(FVM_Context *c,const Program &p,FVM_Object *object,FVM_ValueType type,double number)
{
    const char *parameters[]={"input"};FVM_Object *function=0;FVM_JitCode *native=0;
    FVM_Value arg,out,value;fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=object;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),1,parameters,0,&function)==FVM_OK);
    assert(fvm_gc_pin(c,function)==FVM_OK);
    assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&native)==FVM_OK);
    for(unsigned mode=0;mode<2;++mode)
    {
        fvm_value_number(&value,42);assert(fvm_object_set(c,object,"value",&value)==FVM_OK);
        if(mode)assert(fvm_function_set_body(c,function,fvm_jit_body,native,0)==FVM_OK);
        unsigned long entries=c->execution_mode.stats.native_entries;
        assert(fvm_function_invoke(c,function,0,1,&arg,&out)==FVM_OK && out.type==type);
        assert(c->execution_mode.stats.native_entries==entries+mode);
        if(type==FVM_VALUE_NUMBER)assert(out.number==number);
        if(type==FVM_VALUE_BOOLEAN)assert(out.boolean==(number!=0));
    }
    assert(fvm_function_set_body(c,function,0,0,0)==FVM_OK && fvm_jit_destroy(native)==FVM_OK);
}

int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *runtime=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    for(unsigned version=0;version<=8;++version)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;FVM_Object *object=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_context_create(runtime,&c)==FVM_OK);
        if(version){swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        assert(fvm_construct(c,"Object",0,0,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK);
        Program local;local.text("kept");local.number(42);local.op(0x3c);
        local.text("kept");local.op(0x41);local.text("copy");local.variable("kept");local.op(0x1d);
        local.variable("copy");local.finish();execute(c,local,object,FVM_VALUE_NUMBER,42);
        Program fresh;fresh.text("fresh");fresh.op(0x41);fresh.variable("fresh");fresh.finish();
        execute(c,fresh,object,FVM_VALUE_UNDEFINED,0);
        Program member;member.variable("input");member.text("value");member.number(42);member.op(0x4f);
        member.variable("input");member.text("value");member.op(0x4e);member.finish();
        execute(c,member,object,FVM_VALUE_NUMBER,42);
        Program remove;remove.variable("input");remove.text("value");remove.op(0x3a);remove.finish();
        execute(c,remove,object,FVM_VALUE_BOOLEAN,1);
        Program binding;binding.text("jit_delete_binding");binding.number(42);binding.op(0x1d);
        binding.text("jit_delete_binding");binding.op(0x3b);binding.finish();
        execute(c,binding,object,FVM_VALUE_BOOLEAN,1);
        if(version)
        {
            Program qualified;qualified.text("input.value");qualified.op(0x3b);qualified.finish();
            execute(c,qualified,object,FVM_VALUE_BOOLEAN,1);
            Program primitive;primitive.text("abc");primitive.text("length");primitive.op(0x4e);primitive.finish();
            execute(c,primitive,object,FVM_VALUE_NUMBER,3);
        }
        const unsigned char missing[]={0x1d,0x3c,0x41,0x4e,0x4f,0x3a,0x3b};
        for(size_t i=0;i<sizeof(missing);++i)
        {
            const unsigned char bytes[]={missing[i],0x3e,0};FVM_JitCode *code=0;
            FVM_Context *contexts[2]={0,0};FVM_Object *functions[2]={0,0};
            FVM_Value results[2];FVM_Result statuses[2];
            assert(fvm_jit_compile(bytes,sizeof(bytes),&code)==FVM_OK);
            /* Delete2 with an absent operand can delete the versioned global
             * undefined binding. Compare fresh environments, not consecutive mutations. */
            for(unsigned mode=0;mode<2;++mode)
            {
                assert(fvm_context_create(runtime,&contexts[mode])==FVM_OK);
                if(version)assert(fvm_load_memory(contexts[mode],swf,sizeof(swf))==FVM_OK);
                assert(fvm_standard_install(contexts[mode])==FVM_OK);
                assert(fvm_function_create_avm1(contexts[mode],bytes,sizeof(bytes),0,0,0,&functions[mode])==FVM_OK);
                if(mode)assert(fvm_function_set_body(contexts[mode],functions[mode],fvm_jit_body,code,0)==FVM_OK);
                statuses[mode]=fvm_function_invoke(contexts[mode],functions[mode],0,0,0,&results[mode]);
            }
            assert(statuses[0]==statuses[1]);
            if(statuses[0]==FVM_OK)
            {
                assert(results[0].type==results[1].type);
                if(results[0].type==FVM_VALUE_BOOLEAN)assert(results[0].boolean==results[1].boolean);
            }
            assert(fvm_function_set_body(contexts[1],functions[1],0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
            fvm_context_destroy(contexts[0]);fvm_context_destroy(contexts[1]);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(runtime);return 0;
}
