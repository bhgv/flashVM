/* Actual GetVariable respects namespace version gates and target/global shadowing. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "../src/core/runtime.hpp"
static FVM_Value read_name(FVM_Context *c,FVM_Object *clip,const char *key)
{ Bytes b;text(b,"result");global(b,key);b.push_back(0x1d);b.push_back(0);assert(fvm_avm1_execute_receiver(c,b.begin(),b.size(),clip)==FVM_OK);FVM_Value out;assert(fvm_object_get(c,clip,"result",&out)==FVM_OK);return out; }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK);FVM_Value out=read_name(c,clip,"_global"),value;assert(version>=6?(out.type==FVM_VALUE_OBJECT && out.object==c->global):out.type==FVM_VALUE_UNDEFINED);out=read_name(c,clip,"_GLOBAL");assert(version==6?(out.type==FVM_VALUE_OBJECT && out.object==c->global):out.type==FVM_VALUE_UNDEFINED);
 fvm_value_number(&value,31);assert(fvm_set_global(c,"_global",&value)==FVM_OK && read_name(c,clip,"_global").number==31);fvm_value_number(&value,41);assert(fvm_object_set(c,clip,"_global",&value)==FVM_OK && read_name(c,clip,"_global").number==41);fvm_value_undefined(&value);assert(fvm_object_set(c,clip,"_global",&value)==FVM_OK && read_name(c,clip,"_global").type==FVM_VALUE_UNDEFINED);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
