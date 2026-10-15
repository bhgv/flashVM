/* Missing display/URL operands retain top-relative roles; empty With skips its body. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned version,one,delivered;
static FVM_Result sink(FVM_Context *,FVM_Object *,unsigned argc,const FVM_Value *args,FVM_Value *out,void *)
{const char *missing=version<7?"":"undefined";assert(argc==3 && args[0].type==FVM_VALUE_STRING && args[1].type==FVM_VALUE_STRING && args[2].type==FVM_VALUE_NUMBER && args[2].number==0);assert(std::strcmp(args[0].string,missing)==0 && std::strcmp(args[1].string,one?"dest":missing)==0);++delivered;fvm_value_undefined(out);return FVM_OK;}
static void run(FVM_Context *c,FVM_Object *root,Bytes code)
{code.push_back(0);assert(fvm_avm1_execute_receiver(c,code.begin(),code.size(),root)==FVM_OK);}
static void set(FVM_Context *c,FVM_Object *o,double n)
{FVM_Value value;fvm_value_number(&value,n);assert(fvm_object_set(c,o,"_x",&value)==FVM_OK);}
static double get(FVM_Context *c,FVM_Object *o)
{FVM_Value out;assert(fvm_object_get(c,o,"_x",&out)==FVM_OK && out.type==FVM_VALUE_NUMBER);return out.number;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned mode=0;mode<6;++mode)
    {
        version=mode+3;FVM_Context *c=0;FVM_Object *root=0,*clip=0;FVM_Value args[2],out;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(mode)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
        assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);
        assert(fvm_value_string(args,"undefined")==FVM_OK);fvm_value_number(args+1,2);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);clip=out.object;
        for(unsigned write=0;write<2;++write)for(unsigned count=0;count<(write?3U:2U);++count)
        {
            set(c,root,25);set(c,clip,17);Bytes code;if(count==2)integer(code,0);if(count)integer(code,write?(count==1?7:9):0);code.push_back(write?0x23:0x22);
            if(!write){text(code,"result");code.push_back(0x4d);code.push_back(0x1d);}code.push_back(0);
            FVM_Result r=fvm_avm1_execute_receiver(c,code.begin(),code.size(),root);
            if(!mode)assert(r==FVM_ERROR);
            else
            {
                assert(r==FVM_OK);
                if(!write){assert(fvm_object_get(c,root,"result",&out)==FVM_OK);if(!count && version>=7)assert(out.type==FVM_VALUE_UNDEFINED);else assert(out.type==FVM_VALUE_NUMBER && out.number==(version<7?25:17));}
                else{double expected_root=25,expected_clip=17;if(count==1 && version<7)expected_root=7;if(count==2){if(version<7)expected_root=9;else expected_clip=9;}assert(get(c,root)==expected_root && get(c,clip)==expected_clip);}
            }
        }
        assert(fvm_avm1_set_url_rooted(c,sink,0,0)==FVM_OK);
        for(one=0;one<2;++one)
        {Bytes code;if(one)text(code,"dest");code.push_back(0x9a);u16(code,1);code.push_back(0);code.push_back(0);delivered=0;FVM_Result r=fvm_avm1_execute_receiver(c,code.begin(),code.size(),root);assert(mode?r==FVM_OK && delivered==1:r==FVM_ERROR && !delivered);}
        Bytes body;text(body,"inside");integer(body,99);body.push_back(0x1d);body.push_back(0);
        Bytes code;code.push_back(0x94);u16(code,2);u16(code,(unsigned)body.size());for(size_t i=0;i<body.size();++i)code.push_back(body[i]);text(code,"after");integer(code,7);code.push_back(0x1d);code.push_back(0);
        FVM_Result r=fvm_avm1_execute_receiver(c,code.begin(),code.size(),root);if(!mode)assert(r==FVM_ERROR);else{assert(r==FVM_OK && c->lexical_scopes.empty());assert(fvm_object_get(c,root,"inside",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);assert(fvm_object_get(c,root,"after",&out)==FVM_OK && out.number==7);}
        const unsigned char bad_url[]={0x9a,1,0,3,0},bad_with[]={0x94,1,0,0,0};
        assert(fvm_avm1_execute_receiver(c,bad_url,sizeof(bad_url),root)==FVM_INVALID_ARGUMENT && fvm_avm1_execute_receiver(c,bad_with,sizeof(bad_with),root)==FVM_INVALID_ARGUMENT);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
