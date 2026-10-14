/* Delete2 slash paths resolve full prefixes across scopes and preserve dotted clip names. */
#define main paths_fixture_main
#include "movieclip_paths.cpp"
#undef main
#include "flashvm/execution.h"
static bool remove_name(FVM_Context *c,FVM_Object *receiver,const char *key)
{
 Bytes bytes;text(bytes,key);bytes.push_back(0x3b);bytes.push_back(0x3e);bytes.push_back(0);
 FVM_Object *fn=0;FVM_Value out;assert(fvm_function_create_avm1(c,bytes.begin(),bytes.size(),0,0,0,&fn)==FVM_OK);
 assert(fvm_function_invoke(c,fn,receiver,0,0,&out)==FVM_OK && out.type==FVM_VALUE_BOOLEAN);return !!out.boolean;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
 assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);
 FVM_Object *a=child(c,root,"a",1),*b=child(c,a,"x.y",2);FVM_Value value;fvm_value_number(&value,17);
 assert(fvm_object_set(c,b,"victim",&value)==FVM_OK && remove_name(c,root,"a/x.y:victim"));
 assert(!remove_name(c,root,"a/x.y:victim"));
 assert(fvm_object_set(c,b,"victim",&value)==FVM_OK && remove_name(c,b,"/a/x.y:victim"));
 assert(fvm_object_set(c,a,"victim",&value)==FVM_OK && remove_name(c,b,"../:victim"));
 assert(fvm_object_set(c,b,"slash/name",&value)==FVM_OK && remove_name(c,root,"a/x.y:slash/name"));
 if(version>=5){assert(fvm_object_set(c,b,"victim",&value)==FVM_OK && remove_name(c,root,"this/a/x.y:victim"));assert(fvm_object_set(c,b,"victim",&value)==FVM_OK && remove_name(c,b,"_root/a/x.y:victim"));}
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
