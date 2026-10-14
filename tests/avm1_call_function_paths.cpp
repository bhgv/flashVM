/* CallFunction uses versioned variable paths and never resurrects deleted script records. */
#define main function2_fixture_main
#include "avm1_function2.cpp"
#undef main
#include "../src/core/runtime.hpp"
static FVM_Value call_name(FVM_Context *c,const char *key)
{ Bytes code;integer(code,0);text(code,key);code.push_back(0x3d);code.push_back(0x3e);code.push_back(0);FVM_Object *fn=0;FVM_Value out;assert(fvm_function_create_avm1(c,code.begin(),code.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK);return out; }
static void definition(Bytes &b,const char *key)
{ Bytes body;integer(body,99);body.push_back(0x3e);body.push_back(0);Bytes header;name(header,key);u16(header,0);u16(header,(unsigned)body.size());b.push_back(0x9b);u16(b,(unsigned)header.size());for(size_t i=0;i<header.size();++i)b.push_back(header[i]);for(size_t i=0;i<body.size();++i)b.push_back(body[i]); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned version=4;version<=8;++version){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
 FVM_Object *owner=0,*method=0;assert(fvm_object_create(c,&owner)==FVM_OK);Bytes body;variable(body,"this");body.push_back(0x3e);body.push_back(0);assert(fvm_function_create_avm1(c,body.begin(),body.size(),0,0,0,&method)==FVM_OK);FVM_Value value,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=owner;assert(fvm_set_global(c,"owner",&value)==FVM_OK);value.object=method;assert(fvm_object_set(c,owner,"method",&value)==FVM_OK && fvm_set_global(c,"owner.method",&value)==FVM_OK);
 out=call_name(c,"owner:method");assert(out.type==FVM_VALUE_OBJECT && out.object==owner);out=call_name(c,"owner.method");assert(out.type==FVM_VALUE_OBJECT && out.object==(version>=5?owner:c->global));
 if(version>=5){out=call_name(c,"owner/method");assert(out.type==FVM_VALUE_OBJECT && out.object==c->global);}
 Bytes cached;definition(cached,"cached");text(cached,"cached");cached.push_back(0x3b);cached.push_back(0x17);integer(cached,0);text(cached,"cached");cached.push_back(0x3d);cached.push_back(0x3e);cached.push_back(0);FVM_Object *fn=0;assert(fvm_function_create_avm1(c,cached.begin(),cached.size(),0,0,0,&fn)==FVM_OK && fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED && c->script_functions.find("cached")!=c->script_functions.end());
 fvm_value_number(&value,7);assert(fvm_set_global(c,"notCallable",&value)==FVM_OK && call_name(c,"notCallable").type==FVM_VALUE_UNDEFINED && call_name(c,"missing").type==FVM_VALUE_UNDEFINED);
 assert(c->lexical_scopes.empty() && !c->gc_frames);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
