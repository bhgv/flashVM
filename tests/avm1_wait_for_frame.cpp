/* Frame checks skip complete records, including variable payloads and nested bodies. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static void direct(Bytes &code,unsigned frame,unsigned skip)
{code.push_back(0x8a);u16(code,3);u16(code,frame);code.push_back((unsigned char)skip);}
static void value_check(Bytes &code,unsigned skip)
{code.push_back(0x8d);u16(code,1);code.push_back((unsigned char)skip);}
static void set(Bytes &code,const char *name,unsigned value)
{text(code,name);integer(code,value);code.push_back(0x1d);}
static void run(FVM_Context *c,Bytes code)
{code.push_back(0);assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;FVM_Value out;int removed=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        for(unsigned mode=0;mode<2;++mode)
        {
            for(unsigned loaded=0;loaded<2;++loaded)
            {
                Bytes code;
                if(mode){integer(code,loaded?16001:16002);value_check(code,3);}
                else direct(code,loaded?16000:16001,3);
                set(code,"branch",loaded?1:99);set(code,"after",7);run(c,code);
                assert(fvm_get_global(c,"after",&out)==FVM_OK && out.number==7);
                assert(fvm_get_global(c,"branch",&out)==FVM_OK);
                assert(loaded?out.number==1:out.type==FVM_VALUE_UNDEFINED);
                assert(fvm_object_delete(c,c->global,"branch",&removed)==FVM_OK);
            }
            Bytes code;
            if(mode){integer(code,16002);value_check(code,1);}else direct(code,16001,1);
            code.push_back(0xf0);u16(code,5);for(unsigned i=0;i<5;++i)code.push_back(0x7f);
            set(code,"long_record",8);run(c,code);
            assert(fvm_get_global(c,"long_record",&out)==FVM_OK && out.number==8);
            Bytes nested;set(nested,"nested",99);nested.push_back(0);
            code.clear();if(mode){integer(code,16002);value_check(code,1);}else direct(code,16001,1);
            code.push_back(0x94);u16(code,2);u16(code,(unsigned)nested.size());
            for(size_t i=0;i<nested.size();++i)code.push_back(nested[i]);
            set(code,"after_nested",9);run(c,code);
            assert(fvm_get_global(c,"nested",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            assert(fvm_get_global(c,"after_nested",&out)==FVM_OK && out.number==9);
            code.clear();if(mode){integer(code,16002);value_check(code,255);}else direct(code,16001,255);
            set(code,"too_far",99);run(c,code);
            assert(fvm_get_global(c,"too_far",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
        }
        Bytes empty;value_check(empty,255);set(empty,"empty",10);run(c,empty);
        assert(fvm_get_global(c,"empty",&out)==FVM_OK && out.number==10);
        const unsigned char bad_direct[]={0x8a,2,0,0,0,0};
        const unsigned char bad_value[]={0x8d,0,0,0};
        assert(fvm_avm1_execute(c,bad_direct,sizeof(bad_direct))==FVM_INVALID_ARGUMENT);
        assert(fvm_avm1_execute(c,bad_value,sizeof(bad_value))==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
