/* Actual member, qualified-variable and Delete2 actions share namespace version policy. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);FVM_Object *clip=0;assert(fvm_movieclip_create(c,"root",1,&clip)==FVM_OK);object(c,"clip",clip);FVM_Value value,out;fvm_value_number(&value,17);assert(fvm_set_global(c,"marker",&value)==FVM_OK);
 Bytes b;text(b,"clip:_global");b.push_back(0x1c);out=run(c,b);assert(version>=6?(out.type==FVM_VALUE_OBJECT && out.object==c->global):out.type==FVM_VALUE_UNDEFINED);
 if(version>=5){b.clear();global(b,"clip");text(b,"_global");b.push_back(0x4e);out=run(c,b);assert(version>=6?(out.type==FVM_VALUE_OBJECT && out.object==c->global):out.type==FVM_VALUE_UNDEFINED);}
 b.clear();text(b,"_global:marker");b.push_back(0x1c);out=run(c,b);assert(version>=6?(out.type==FVM_VALUE_NUMBER && out.number==17):out.type==FVM_VALUE_UNDEFINED);b.clear();text(b,"_global:marker");b.push_back(0x3b);out=run(c,b);assert(out.type==FVM_VALUE_BOOLEAN && out.boolean==(version>=6));fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
