/* String/member operands are filled below existing values; name callbacks retain roots. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static void finish(Bytes &code)
{text(code,"result");code.push_back(0x4d);code.push_back(0x1d);code.push_back(0);}
static FVM_Value run(FVM_Context *c,Bytes code)
{assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);FVM_Value out;assert(fvm_get_global(c,"result",&out)==FVM_OK);return out;}
static unsigned converted;
static FVM_Result key(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{++converted;int deleted=0;assert(fvm_object_delete(c,c->global,"name",&deleted)==FVM_OK);assert(fvm_object_delete(c,c->global,"assigned",&deleted)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);return fvm_value_string(out,"victim");}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        FVM_Context *c=0;unsigned version=mode+3;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        for(unsigned mb=0;mb<2;++mb)for(unsigned count=0;count<3;++count)
        {
            Bytes code;if(count==2){integer(code,2);integer(code,3);}else if(count==1)integer(code,7);
            code.push_back(mb?0x35:0x15);finish(code);
            if(!mode)assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_ERROR);
            else {FVM_Value out=run(c,code);assert(out.type==FVM_VALUE_STRING && std::strcmp(out.string,version<7 || !count?"":count==1?"undefin":"nde")==0);}
        }
        const unsigned ops[]={0x4e,0x4f,0x3a};
        for(unsigned i=0;i<3;++i)for(unsigned count=0;count<(i==1?3U:2U);++count)
        {
            Bytes code;if(count==2)text(code,"key");if(count)integer(code,7);code.push_back((unsigned char)ops[i]);
            if(i==1){text(code,"result");integer(code,123);code.push_back(0x1d);code.push_back(0);}else finish(code);
            if(!mode)assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_ERROR);
            else {FVM_Value out=run(c,code);if(i==0)assert(out.type==FVM_VALUE_UNDEFINED);else if(i==1)assert(out.type==FVM_VALUE_NUMBER && out.number==123);else assert(out.type==FVM_VALUE_BOOLEAN && !out.boolean);}
        }
        if(mode)
        {
            for(unsigned i=0;i<3;++i)
            {
                FVM_Object *name=0,*assigned=0;assert(fvm_object_create(c,&name)==FVM_OK);attach(c,name,"toString",key,0);
                FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=name;assert(fvm_set_global(c,"name",&value)==FVM_OK);
                Bytes code;text(code,"name");code.push_back(0x1c);
                if(i==1){assert(fvm_object_create(c,&assigned)==FVM_OK);value.object=assigned;assert(fvm_set_global(c,"assigned",&value)==FVM_OK);text(code,"assigned");code.push_back(0x1c);}
                code.push_back((unsigned char)ops[i]);
                if(i==1){text(code,"result");integer(code,123);code.push_back(0x1d);code.push_back(0);}else finish(code);
                converted=0;FVM_GCRootFrame *roots=c->gc_frames;run(c,code);assert(converted==1 && roots==c->gc_frames);
            }
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
