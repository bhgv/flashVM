/* MovieClip dot/slash paths preserve counted names and legacy _target callback restoration. */
#define FVM_AVM1_STANDARD_FIXTURE_ONLY
#include "avm1_standard_types.cpp"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "../src/core/runtime.hpp"
#include "../src/standard/movieclip_state.hpp"
static unsigned overrides;
static FVM_Result custom(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++overrides;return fvm_value_string(out,"custom"); }
static FVM_Object *child(FVM_Context *c,FVM_Object *root,const char *name,unsigned depth)
{ FVM_Value args[2],out;fvm_value_string(args,name);fvm_value_number(args+1,depth);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);return out.object; }
static void path(FVM_Context *c,FVM_Object *clip,const char *dot,size_t dot_size,const char *slash,size_t slash_size)
{
 FVM_Value out;assert(fvm_object_call(c,clip,"toString",0,0,&out)==FVM_OK && fvm_value_string_size(&out)==dot_size && !std::memcmp(out.string,dot,dot_size));assert(fvm_object_get(c,clip,"_target",&out)==FVM_OK && fvm_value_string_size(&out)==slash_size && !std::memcmp(out.string,slash,slash_size));
}
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
 FVM_Object *root=0;assert(fvm_movieclip_create(c,"host-root-name",1,&root)==FVM_OK);FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=root;assert(fvm_set_global(c,"root",&value)==FVM_OK);FVM_Object *a=child(c,root,"a",1),*b=child(c,a,"b",2);path(c,root,"_level0",7,"/",1);path(c,b,"_level0.a.b",11,"/a/b",4);
 fvm_value_string_bytes(&value,"x\0y",3);assert(fvm_object_set(c,a,"_name",&value)==FVM_OK);path(c,b,"_level0.x\0y.b",13,"/x\0y/b",6);
 fvm_value_string_bytes(&value,"\xed\xa0\x80",3);assert(fvm_object_set(c,b,"_name",&value)==FVM_OK);path(c,b,"_level0.x\0y.\xed\xa0\x80",15,"/x\0y/\xed\xa0\x80",8);
 /* Emulate the old graph's shared native callback ID 1010 for its _target getter. */
 FVM_Object *proto=root->prototype;FVM_NativeFunction getter=proto->accessors["_target"].getter;proto->accessors["_target"].getter=fvm_movieclip_callback_from_id(1010);
 FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=b;assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK);proto->accessors["_target"].getter=getter;assert(fvm_channel_receive_value(other,channel,&value,0)==FVM_OK);FVM_Object *copy=value.object;assert(fvm_gc_pin(other,copy)==FVM_OK);assert(fvm_gc_collect(other,0)==FVM_OK);path(other,copy,"_level0.x\0y.\xed\xa0\x80",15,"/x\0y/\xed\xa0\x80",8);
 unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_object_define_method_rooted(c,b,"toString",custom,0,0)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=b;assert(fvm_set_global(c,"input",&value)==FVM_OK);Bytes actions;global(actions,"input");actions.push_back(0x4b);FVM_Value out=run(c,actions);assert(overrides==0 && fvm_value_string_size(&out)==15 && !std::memcmp(out.string,"_level0.x\0y.\xed\xa0\x80",15));assert(fvm_value_to_string(c,&value,&out)==FVM_OK && overrides==1 && !std::strcmp(out.string,"custom"));
 /* Parent cycles fail within the bounded traversal, without entering callbacks. */
 FVM_Object *state=root->resource_result.object;FVM_Value old=state->properties["parent"];state->properties["parent"]=value;assert(fvm_movieclip_reference_string(c,b,&out)==FVM_ERROR);state->properties["parent"]=old;assert(fvm_movieclip_reference_string(c,b,&out)==FVM_OK);
 fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
