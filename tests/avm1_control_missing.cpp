/* Missing stack/control operands retain loaded undefined semantics and host strictness. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned traced;
static FVM_Result trace_sink(FVM_Context *,FVM_Object *,unsigned argc,const FVM_Value *args,FVM_Value *out,void *)
{assert(argc==1 && args[0].type==FVM_VALUE_STRING && std::strcmp(args[0].string,"undefined")==0);++traced;fvm_value_undefined(out);return FVM_OK;}
static void store(Bytes &code,const char *key)
{text(code,key);code.push_back(0x4d);code.push_back(0x1d);}
static void run(FVM_Context *c,Bytes code)
{code.push_back(0);assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(mode+3);
        assert(fvm_context_create(rt,&c)==FVM_OK);if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);const unsigned ops[]={0x17,0x26,0x2a,0x3e,0x4c,0x4d};
        if(!mode)
        {
            for(unsigned i=0;i<sizeof(ops)/sizeof(ops[0]);++i){const unsigned char code[]={(unsigned char)ops[i],0};assert(fvm_avm1_execute(c,code,sizeof(code))==FVM_ERROR);}
            const unsigned char branch[]={0x9d,2,0,0,0,0};assert(fvm_avm1_execute(c,branch,sizeof(branch))==FVM_ERROR);
        }
        else
        {
            for(unsigned op=0;op<2;++op)for(unsigned one=0;one<(op?2U:1U);++one)
            {
                Bytes code;if(one)integer(code,7);code.push_back(op?0x4d:0x4c);store(code,"top");store(code,"bottom");run(c,code);
                FVM_Value out;assert(fvm_get_global(c,"top",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
                assert(fvm_get_global(c,"bottom",&out)==FVM_OK);assert(one?out.type==FVM_VALUE_NUMBER && out.number==7:out.type==FVM_VALUE_UNDEFINED);
            }
            Bytes code;code.push_back(0x17);run(c,code);
            assert(fvm_avm1_set_trace_rooted(c,trace_sink,0,0)==FVM_OK);traced=0;code.clear();code.push_back(0x26);run(c,code);assert(traced==1);
            const unsigned char thrown[]={0x2a,0};assert(fvm_avm1_execute(c,thrown,sizeof(thrown))==FVM_EXCEPTION);
            FVM_Value out;assert(fvm_exception_get(c,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);assert(fvm_exception_clear(c)==FVM_OK);
            const unsigned char returning[]={0x3e,0};FVM_Object *fn=0;
            assert(fvm_function_create_avm1(c,returning,sizeof(returning),0,0,0,&fn)==FVM_OK);
            fvm_value_number(&out,99);assert(fvm_function_invoke(c,fn,c->global,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            Bytes assignment;text(assignment,"branch");integer(assignment,7);assignment.push_back(0x1d);
            code.clear();code.push_back(0x9d);u16(code,2);u16(code,(unsigned)assignment.size());for(size_t i=0;i<assignment.size();++i)code.push_back(assignment[i]);run(c,code);
            assert(fvm_get_global(c,"branch",&out)==FVM_OK && out.number==7);
        }
        const unsigned char malformed[]={0x9d,1,0,0,0};assert(fvm_avm1_execute(c,malformed,sizeof(malformed))==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
