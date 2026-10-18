/* Missing names preserve versioned scope policy; SetTarget2 undefined resets or invalidates. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static FVM_Object *expected_base,*expected_target;static unsigned seen;
static FVM_Result probe(FVM_Context *c,FVM_Object *,unsigned argc,const FVM_Value *args,FVM_Value *out,void *)
{assert(argc==1 && args[0].type==FVM_VALUE_STRING && std::strcmp(args[0].string,"probe")==0);assert(c->avm1_target_active && c->avm1_base_clip==expected_base && c->avm1_target_clip==expected_target);++seen;fvm_value_undefined(out);return FVM_OK;}
static void run(FVM_Context *c,FVM_Object *root,Bytes code)
{code.push_back(0);assert(fvm_avm1_execute_receiver(c,code.begin(),code.size(),root)==FVM_OK);}
static void finish(Bytes &code)
{text(code,"result");code.push_back(0x4d);code.push_back(0x1d);}
static void bind(FVM_Context *c,FVM_Object *root,const char *key)
{FVM_Value value;fvm_value_number(&value,123);assert(fvm_object_set(c,root,key,&value)==FVM_OK);}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        FVM_Context *c=0;FVM_Object *root=0;unsigned version=mode+3;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        const unsigned ops[]={0x1c,0x1d,0x20,0x3b,0x3c,0x41};
        if(!mode)
        {
            for(unsigned i=0;i<6;++i){const unsigned char code[]={(unsigned char)ops[i],0};assert(fvm_avm1_execute_receiver(c,code,sizeof(code),root)==FVM_ERROR);}
            const unsigned char bad_name[]={0x96,5,0,7,7,0,0,0,0x1c,0};assert(fvm_avm1_execute_receiver(c,bad_name,sizeof(bad_name),root)==FVM_ERROR);
        }
        else
        {
            const char *name=version<7?"":"undefined";FVM_Value out;
            bind(c,root,name);Bytes code;code.push_back(0x1c);finish(code);run(c,root,code);
            assert(fvm_object_get(c,root,"result",&out)==FVM_OK);
            assert(version<7?out.type==FVM_VALUE_UNDEFINED:out.type==FVM_VALUE_NUMBER && out.number==123);
            for(unsigned one=0;one<2;++one)
            {
                bind(c,root,name);code.clear();if(one)integer(code,7);code.push_back(0x1d);run(c,root,code);
                assert(fvm_object_get(c,root,name,&out)==FVM_OK);
                if(version<7)assert(out.type==FVM_VALUE_UNDEFINED);else if(one)assert(out.type==FVM_VALUE_NUMBER && out.number==7);else assert(out.type==FVM_VALUE_UNDEFINED);
                bind(c,root,name);code.clear();if(one)integer(code,7);code.push_back(0x3c);run(c,root,code);
                assert(fvm_object_get(c,root,name,&out)==FVM_OK);
                if(version<7)assert(out.type==FVM_VALUE_UNDEFINED);else if(one)assert(out.type==FVM_VALUE_NUMBER && out.number==7);else assert(out.type==FVM_VALUE_UNDEFINED);
            }
            bind(c,root,name);code.clear();code.push_back(0x41);run(c,root,code);assert(fvm_object_get(c,root,name,&out)==FVM_OK);assert(version<7?out.type==FVM_VALUE_UNDEFINED:out.type==FVM_VALUE_NUMBER && out.number==123);
            bind(c,root,name);code.clear();code.push_back(0x3b);finish(code);run(c,root,code);
            assert(fvm_object_get(c,root,"result",&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN && !!out.boolean==(version>=7));
            assert(fvm_object_get(c,root,name,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);
            FVM_Value args[2];fvm_value_string(args,"child");fvm_value_number(args+1,2);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);
            expected_base=root;expected_target=version<7?root:0;seen=0;assert(fvm_avm1_set_trace_rooted(c,probe,0,0)==FVM_OK);
            code.clear();code.push_back(0x8b);u16(code,6);const char path[]="child";for(unsigned i=0;i<sizeof(path);++i)code.push_back((unsigned char)path[i]);
            code.push_back(0x20);text(code,"probe");code.push_back(0x26);run(c,root,code);assert(seen==1 && !c->avm1_target_active && !c->avm1_base_clip && !c->avm1_target_clip);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
