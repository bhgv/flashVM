/* URL actions delegate counted values/flags to a rooted host sink, preserving coercion order. */
#define main string_fixture_main
#include "avm1_legacy_string_actions.cpp"
#undef main
#include "../src/core/runtime.hpp"
static void word(Bytes &b,unsigned value){b.push_back((unsigned char)value);b.push_back((unsigned char)(value>>8));}
static unsigned delivered;
static FVM_Value last_url,last_target;
static unsigned last_flags;
static FVM_Object *capture;
static FVM_Result url_sink(FVM_Context *c,FVM_Object *receiver,unsigned n,const FVM_Value *a,FVM_Value *out,void *raw)
{
 assert(receiver==c->global && n==3 && a[0].type==FVM_VALUE_STRING && a[1].type==FVM_VALUE_STRING && a[2].type==FVM_VALUE_NUMBER);assert(raw==capture);FVM_Value alive;assert(fvm_object_get(c,capture,"alive",&alive)==FVM_OK && alive.number==42);assert(fvm_gc_collect(c,0)==FVM_INVALID_ARGUMENT);last_url=a[0];last_target=a[1];last_flags=(unsigned)a[2].number;++delivered;fvm_value_number(out,123);return FVM_OK;
}
static FVM_Result url_error(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ return FVM_ERROR; }
static FVM_Result url_throw(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ throw std::bad_alloc(); }
static FVM_Result url_replace(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ ++delivered;return fvm_avm1_set_url_rooted(c,0,0,0); }
static FVM_Result request(FVM_Context *c,const FVM_Value &url,const FVM_Value &target,unsigned flags)
{ assert(fvm_set_global(c,"url",&url)==FVM_OK);assert(fvm_set_global(c,"target",&target)==FVM_OK);Bytes b;global(b,"url");global(b,"target");b.push_back(0x9a);word(b,1);b.push_back((unsigned char)flags);b.push_back(0);return fvm_avm1_execute(c,b.begin(),b.size()); }
static void literal(Bytes &b,const char *url,const char *target)
{ b.push_back(0x83);word(b,(unsigned)(std::strlen(url)+std::strlen(target)+2));do{b.push_back((unsigned char)*url);}while(*url++);do{b.push_back((unsigned char)*target);}while(*target++);b.push_back(0); }
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);assert(fvm_extensions_install(c)==FVM_OK);
 assert(fvm_object_create(c,&capture)==FVM_OK);FVM_Value value,a,b;fvm_value_number(&value,42);assert(fvm_object_set(c,capture,"alive",&value)==FVM_OK);assert(fvm_avm1_set_url_rooted(c,url_sink,capture,capture)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);assert(fvm_avm1_set_url_rooted(other,url_sink,0,capture)==FVM_INVALID_ARGUMENT);assert(fvm_avm1_set_url_rooted(0,url_sink,0,0)==FVM_INVALID_ARGUMENT);assert(fvm_avm1_set_url_rooted(c,0,capture,0)==FVM_INVALID_ARGUMENT);
 fvm_value_string_bytes(&a,"a\0b",3);fvm_value_string_bytes(&b,"t\0g",3);const unsigned flags[]={0,1,2,64,128,192,194};for(unsigned i=0;i<sizeof(flags)/sizeof(*flags);++i){assert(request(c,a,b,flags[i])==FVM_OK);assert(last_flags==flags[i] && fvm_value_string_size(&last_url)==3 && !std::memcmp(last_url.string,"a\0b",3) && fvm_value_string_size(&last_target)==3 && !std::memcmp(last_target.string,"t\0g",3));}
 unsigned before=delivered;const unsigned bad[]={3,4,8,16,32,195,255};for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);++i)assert(request(c,a,b,bad[i])==FVM_INVALID_ARGUMENT);assert(delivered==before);
 Bytes code;literal(code,"FSCommand:quit","argument");assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK && last_flags==0 && !std::strcmp(last_url.string,"FSCommand:quit") && !std::strcmp(last_target.string,"argument"));
 const unsigned char malformed[][8]={{0x83,2,0,'a',0,0},{0x83,3,0,0,0,0,0},{0x9a,0,0,0}};for(unsigned i=0;i<3;++i)assert(fvm_avm1_execute(c,malformed[i],i==0?6:i==1?7:4)==FVM_INVALID_ARGUMENT);unsigned char empty[]={0x9a,1,0,0,0};assert(fvm_avm1_execute(c,empty,sizeof(empty))==FVM_ERROR);
 FVM_Object *left=0,*right=0;assert(fvm_construct(c,"Object",0,0,&left)==FVM_OK);assert(fvm_construct(c,"Object",0,0,&right)==FVM_OK);unsigned ids[]={1,2};assert(fvm_object_define_method_rooted(c,left,"toString",ordered,ids,0)==FVM_OK);assert(fvm_object_define_method_rooted(c,right,"toString",ordered,ids+1,0)==FVM_OK);fvm_value_undefined(&a);a.type=FVM_VALUE_OBJECT;a.object=left;b=a;b.object=right;calls=0;assert(request(c,a,b,0)==FVM_OK && calls==2 && order[0]==2 && order[1]==1);
 /* URL conversion collects a target string created by the preceding script conversion. */
 Bytes body;text(body,"collected");integer(body,0);global(body,"gc");text(body,"collect");body.push_back(0x52);body.push_back(0x1d);text(body,"URL");script_method(c,left,body);body.clear();text(body,"target");text(body,"suffix");body.push_back(0x21);script_method(c,right,body);assert(fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
 for(unsigned i=0;i<20;++i){assert(request(c,a,b,0)==FVM_OK && !std::strcmp(last_url.string,"URL") && !std::strcmp(last_target.string,"targetsuffix"));FVM_Value collected;assert(fvm_get_global(c,"collected",&collected)==FVM_OK && collected.number>=1);}
 assert(fvm_avm1_set_url_rooted(c,url_error,0,capture)==FVM_OK);assert(request(c,a,b,0)==FVM_ERROR);assert(fvm_avm1_set_url_rooted(c,url_throw,0,capture)==FVM_OK);assert(request(c,a,b,0)==FVM_OUT_OF_MEMORY);assert(c->gc_native_depth==0 && c->avm1_execution_depth==0);
 assert(fvm_avm1_set_url_rooted(c,url_replace,0,capture)==FVM_OK);assert(request(c,a,b,0)==FVM_OK);unsigned long freed=0;assert(fvm_gc_collect(c,&freed)==FVM_OK && freed>=1);assert(request(c,a,b,0)==FVM_UNSUPPORTED);
 fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
