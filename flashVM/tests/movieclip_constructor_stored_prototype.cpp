/* Registered clip construction uses raw own prototype data and the source movie's case policy. */
#define FVM_SYMBOL_PROVENANCE_FIXTURE_ONLY
#include "movieclip_registered_provenance.cpp"
static FVM_Object *clip_proto_expected,*clip_proto_replacement;
static unsigned clip_proto_gets,clip_proto_calls,clip_proto_fault;
static FVM_Result clip_proto_getter(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{++clip_proto_gets;*out=case_object(clip_proto_replacement);return clip_proto_fault?fvm_exception_raise(c,out):FVM_OK;}
static FVM_Result clip_proto_native(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++clip_proto_gets;*out=case_object(clip_proto_replacement);return clip_proto_fault?fvm_exception_raise(c,out):FVM_OK;}
static FVM_Result clip_proto_body(FVM_Context *,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{++clip_proto_calls;assert(frame->receiver->prototype==clip_proto_expected);fvm_value_undefined(out);return FVM_OK;}
int main()
{
 FVM_Config cfg;FVM_Runtime *runtime=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
 for(unsigned source=6;source<=8;source+=2)for(unsigned ambient=6;ambient<=8;ambient+=2)for(unsigned kind=0;kind<8;++kind)for(clip_proto_fault=0;clip_proto_fault<2;++clip_proto_fault){
  FVM_Context *c=0;assert(fvm_context_create(runtime,&c)==FVM_OK);case_loaded(c,8);assert(fvm_standard_install(c)==FVM_OK);case_loaded(c,source);
  FVM_Value klass,ctor=registered_constructor(c),stored=get(c,ctor.object,"prototype"),value,args[3],out;assert(fvm_get_global(c,"Object",&klass)==FVM_OK && fvm_gc_pin(c,stored.object)==FVM_OK && fvm_object_create(c,&clip_proto_replacement)==FVM_OK);
  case_register(c,klass.object,"Hero",4,ctor.object);assert(fvm_function_set_body(c,ctor.object,clip_proto_body,0,0)==FVM_OK);
  FVM_Object *getter=0;const unsigned char end[]={0};assert(fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&getter)==FVM_OK && fvm_function_set_body(c,getter,clip_proto_getter,0,clip_proto_replacement)==FVM_OK);
  if(kind==1 || kind==2 || kind==4 || kind==6 || kind==7){int removed=0;assert(fvm_object_set_property_flags(c,ctor.object,"prototype",0,FVM_PROPERTY_VALID_FLAGS)==FVM_OK && fvm_object_delete(c,ctor.object,"prototype",&removed)==FVM_OK && removed);}
  if(kind==3 || kind==5){fvm_value_number(&value,17);assert(fvm_object_set(c,ctor.object,"prototype",&value)==FVM_OK);}
  if(kind==0 || kind==3){fvm_value_string(args,"prototype");args[1]=case_object(getter);fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;assert(fvm_object_call(c,ctor.object,"addProperty",3,args,&out)==FVM_OK && out.boolean);}
  if(kind==1){FVM_Object *carrier=0;assert(fvm_object_create(c,&carrier)==FVM_OK && fvm_object_set(c,carrier,"prototype",&stored)==FVM_OK && fvm_object_set_prototype(c,ctor.object,carrier)==FVM_OK);}
  if(kind==2)assert(fvm_object_define_accessor_rooted(c,ctor.object,"prototype",clip_proto_native,0,0,clip_proto_replacement)==FVM_OK);
  if(kind==4)assert(fvm_object_set(c,ctor.object,"PrOtOtYpE",&stored)==FVM_OK);
  if(kind==6){value=case_object(getter);assert(fvm_object_set(c,ctor.object,"__resolve",&value)==FVM_OK);}
  if(kind==7){assert(fvm_object_define_method_rooted(c,ctor.object,"prototype",clip_proto_native,0,clip_proto_replacement)==FVM_OK);stored=get(c,ctor.object,"prototype");}
  clip_proto_expected=kind==0 || kind==7 || (kind==4 && source<7)?stored.object:0;case_loaded(c,ambient);FVM_Object *root=symbol_root(c,source);clip_proto_gets=clip_proto_calls=0;
  FVM_Object *child=attach(c,root,"Hero","dynamic",3);assert(child && child->prototype==clip_proto_expected && clip_proto_calls==1 && !clip_proto_gets);
  FVM_Value instance,depth,initial;fvm_value_string(&instance,"copy");fvm_value_number(&depth,4);fvm_value_undefined(&initial);
  assert(fvm_exec_duplicate_clip(c,child,&instance,&depth,&initial,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object->prototype==clip_proto_expected && clip_proto_calls==2 && !clip_proto_gets);
  assert(fvm_movieclip_advance(c,root,1)==FVM_OK && clip_proto_calls==3 && !clip_proto_gets);value=get(c,root,"actor");assert(value.type==FVM_VALUE_OBJECT && value.object->prototype==clip_proto_expected);
  if(kind!=2 && kind!=7){
   FVM_Context *other=0;FVM_Channel *channel=0;assert(fvm_context_create(runtime,&other)==FVM_OK && fvm_channel_create(&channel)==FVM_OK);case_loaded(other,source<7?8:6);value=case_object(root);
   assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&value,0)==FVM_OK && fvm_gc_pin(other,value.object)==FVM_OK);
   FVM_Object *copy=value.object;FVM_Value existing=get(other,copy,"dynamic");FVM_Object *copied_proto=existing.object->prototype;
   FVM_Object *transported=attach(other,copy,"Hero","transported",8);assert(transported && transported->prototype==copied_proto && fvm_gc_collect(other,0)==FVM_OK && !other->gc_frames && !other->gc_native_depth);
   fvm_channel_destroy(channel);fvm_context_destroy(other);
  }
  assert(c->swf_info.version==ambient && !c->exception_pending && !c->gc_frames && !c->gc_native_depth && fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(runtime);return 0;
}
