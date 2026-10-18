/* GetVariable delegates qualified/slash paths while preserving versioned literal fallback. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
static FVM_Value read_name(FVM_Context *c,const char *key)
{
 Bytes bytes;text(bytes,key);bytes.push_back(0x1c);bytes.push_back(0x3e);bytes.push_back(0);FVM_Object *fn=0;FVM_Value out;
 assert(fvm_function_create_avm1(c,bytes.begin(),bytes.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK);return out;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 FVM_Object *o=0,*child=0;assert(fvm_object_create(c,&o)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=o;assert(fvm_set_global(c,"o",&value)==FVM_OK);value.object=child;assert(fvm_object_set(c,o,"child",&value)==FVM_OK);
 fvm_value_number(&value,17);assert(fvm_object_set(c,o,"member",&value)==FVM_OK);fvm_value_number(&value,23);assert(fvm_set_global(c,"o.member",&value)==FVM_OK);
 assert(read_name(c,"o.member").number==(version>=5?17:23));assert(read_name(c,"o:member").number==17);
 fvm_value_number(&value,31);assert(fvm_set_global(c,"o/missing",&value)==FVM_OK && read_name(c,"o/missing").number==31);
 fvm_value_number(&value,99);assert(fvm_set_global(c,"o/child",&value)==FVM_OK);FVM_Value out=read_name(c,"o/child");assert(version>=5?(out.type==FVM_VALUE_OBJECT && out.object==child):(out.type==FVM_VALUE_NUMBER && out.number==99));
 assert(fvm_set_global(c,":member",&value)==FVM_OK && read_name(c,":member").type==FVM_VALUE_UNDEFINED);
 if(version>=5){out=read_name(c,"_global.o");assert(version>=6?(out.type==FVM_VALUE_OBJECT && out.object==o):out.type==FVM_VALUE_UNDEFINED);}
 fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
