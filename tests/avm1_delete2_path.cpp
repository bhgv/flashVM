/* Qualified Delete2 resolves a root binding then traverses object-valued dot/colon members. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
static FVM_Value remove(FVM_Context *c,const char *key)
{
    Bytes bytes;text(bytes,key);bytes.push_back(0x3b);bytes.push_back(0x3e);bytes.push_back(0);
    FVM_Object *fn=0;assert(fvm_function_create_avm1(c,bytes.begin(),bytes.size(),0,0,0,&fn)==FVM_OK);
    FVM_Value out;assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK);return out;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version){
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *o=0,*child=0;assert(fvm_object_create(c,&o)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
        FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=o;assert(fvm_set_global(c,"o",&value)==FVM_OK);
        value.object=child;assert(fvm_object_set(c,o,"child",&value)==FVM_OK);fvm_value_number(&value,17);
        const char *paths[]={"o.child.x","o:child:x","o.child:x"};
        for(unsigned i=0;i<3;++i){assert(fvm_object_set(c,child,"x",&value)==FVM_OK);assert(!!remove(c,paths[i]).boolean==(version>=5 || i==1));}
        if(version==4){value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_set_global(c,"o.child",&value)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,child,"x",&value)==FVM_OK && remove(c,"o.child.x").boolean);}
        assert(!remove(c,"missing.x").boolean);
        fvm_value_string(&value,"primitive");assert(fvm_object_set(c,o,"child",&value)==FVM_OK && !remove(c,"o.child.x").boolean);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
