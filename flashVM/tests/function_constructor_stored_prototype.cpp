/* Loaded construction reads own stored prototype data without invoking accessors or __resolve. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/execution.h"
static bool prototype_cold;
static unsigned prototype_gets, prototype_calls, prototype_fault;
static FVM_Object *prototype_expected, *prototype_replacement;
static FVM_Result prototype_getter(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{ ++prototype_gets;*out=class_value(prototype_replacement);return prototype_fault?fvm_exception_raise(c,out):FVM_OK; }
static FVM_Result prototype_native_getter(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{ ++prototype_gets;*out=class_value(prototype_replacement);return prototype_fault?fvm_exception_raise(c,out):FVM_OK; }
static FVM_Result prototype_body(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{ ++prototype_calls;if(prototype_cold){FVM_Value value;assert(fvm_object_get(c,frame->function,"prototype",&value)==FVM_OK && value.type==FVM_VALUE_OBJECT);prototype_expected=value.object;prototype_cold=false;}assert(frame->receiver->prototype==prototype_expected && fvm_gc_collect(c,0)==FVM_OK);fvm_value_undefined(out);return FVM_OK; }
int main()
{
 FVM_Config cfg;FVM_Runtime *runtime=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
 for(unsigned version=0;version<=8;++version)for(unsigned kind=0;kind<8;++kind)for(prototype_fault=0;prototype_fault<2;++prototype_fault){
  if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(runtime,&c)==FVM_OK);
  if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}assert(fvm_standard_install(c)==FVM_OK);
  FVM_Object *function=0,*getter=0;const unsigned char end[]={0};FVM_Value value,stored,args[3],out;assert(fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&function)==FVM_OK && fvm_function_set_body(c,function,prototype_body,0,0)==FVM_OK);
  value=class_value(function);assert(fvm_set_global(c,"StoredCtor",&value)==FVM_OK && fvm_object_get(c,function,"prototype",&stored)==FVM_OK && fvm_gc_pin(c,stored.object)==FVM_OK && fvm_object_create(c,&prototype_replacement)==FVM_OK);
  assert(fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&getter)==FVM_OK && fvm_function_set_body(c,getter,prototype_getter,0,prototype_replacement)==FVM_OK);
  if(kind==1 || kind==2 || kind==4 || kind==5 || kind==6 || kind==7){int removed=0;assert(fvm_object_set_property_flags(c,function,"prototype",0,FVM_PROPERTY_VALID_FLAGS)==FVM_OK && fvm_object_delete(c,function,"prototype",&removed)==FVM_OK && removed);}
  if(kind==3){fvm_value_number(&value,17);assert(fvm_object_set(c,function,"prototype",&value)==FVM_OK);}
  if(kind==0 || kind==3){fvm_value_string(args,"prototype");args[1]=class_value(getter);fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;assert(fvm_object_call(c,function,"addProperty",3,args,&out)==FVM_OK && out.boolean);}
  if(kind==1){FVM_Object *carrier=0;assert(fvm_object_create(c,&carrier)==FVM_OK && fvm_object_set(c,carrier,"prototype",&stored)==FVM_OK && fvm_object_set_prototype(c,function,carrier)==FVM_OK);}
  if(kind==2)assert(fvm_object_define_accessor_rooted(c,function,"prototype",prototype_native_getter,0,0,prototype_replacement)==FVM_OK);
  if(kind==4)assert(fvm_object_set(c,function,"PrOtOtYpE",&stored)==FVM_OK);
  if(kind==5){value=class_value(getter);assert(fvm_object_set(c,function,"__resolve",&value)==FVM_OK);}
  FVM_Value native_prototype;fvm_value_undefined(&native_prototype);
  if(kind>=6){assert(fvm_object_define_method_rooted(c,function,"prototype",prototype_native_getter,0,prototype_replacement)==FVM_OK);if(kind==6){assert(fvm_object_get(c,function,"prototype",&native_prototype)==FVM_OK && native_prototype.type==FVM_VALUE_OBJECT);assert(fvm_gc_pin(c,native_prototype.object)==FVM_OK);}}
  prototype_expected=version?(kind==0 || (kind==4 && version<=6)?stored.object:0):(kind==0 || kind==2 || kind==3?prototype_replacement:kind==1?stored.object:0);
  if(kind==6)prototype_expected=native_prototype.object;
  bool callback=kind==0 || kind==2 || kind==3 || (kind==5 && version);prototype_gets=prototype_calls=0;
  assert(fvm_object_get(c,function,"prototype",&out)==(callback && prototype_fault?FVM_EXCEPTION:FVM_OK) && prototype_gets==(callback?1U:0U));if(c->exception_pending)assert(fvm_exception_clear(c)==FVM_OK);
  if(kind==7){function->method_values.clear();prototype_cold=true;}else prototype_cold=false;
  prototype_gets=0;FVM_Object *instance=0;FVM_Result expected=!version && callback && prototype_fault?FVM_EXCEPTION:FVM_OK;
  assert(fvm_function_construct(c,function,0,0,&instance)==expected && prototype_gets==(!version && callback?1U:0U) && prototype_calls==(expected==FVM_OK?1U:0U));
  if(expected==FVM_EXCEPTION){assert(!instance && fvm_exception_clear(c)==FVM_OK);}else{
   assert(instance && fvm_gc_pin(c,instance)==FVM_OK);if(version){FVM_Value type=class_value(function),input=class_value(instance);assert(fvm_exec_instance_of(c,&input,&type,&out)==FVM_OK && !!out.boolean==(prototype_expected!=0));}
  }
  if(version && fvm_jit_available())for(class_mode=0;class_mode<3;++class_mode){Program code;code.number(0);code.text("StoredCtor");code.op(0x40);code.finish();out=class_run(c,code);assert(out.object->prototype==prototype_expected && !prototype_gets);}
  assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(runtime);return 0;
}
