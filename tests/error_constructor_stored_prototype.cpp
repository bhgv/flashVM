/* Error construction uses stored prototype data and preserves the original message value. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/property.h"
static FVM_Object *error_class, *error_replacement;
static unsigned error_coercions, error_getters;
static FVM_Result error_getter(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{++error_getters;*out=class_value(error_replacement);return fvm_exception_raise(c,out);}
/* Script accessors retain data separately from the throwing getter. */
static FVM_Result error_script_getter(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{return error_getter(c,0,0,0,out,0);}
static FVM_Result error_input(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
 ++error_coercions;
 *out=class_value(error_replacement);
 return fvm_exception_raise(c,out);
}
int main()
{
 FVM_Config cfg;FVM_Runtime *runtime=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
 for(unsigned version=4;version<=8;++version)for(unsigned kind=0;kind<10;++kind)for(unsigned route=0;route<8;++route){
  if(route>=2 && !fvm_jit_available())continue;FVM_Context *c=0;assert(fvm_context_create(runtime,&c)==FVM_OK);
  unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Value klass,stored,input,out;assert(fvm_get_global(c,"Error",&klass)==FVM_OK);error_class=klass.object;
  assert(fvm_object_get(c,error_class,"prototype",&stored)==FVM_OK && fvm_gc_pin(c,stored.object)==FVM_OK && fvm_object_create(c,&error_replacement)==FVM_OK && fvm_gc_pin(c,error_replacement)==FVM_OK);
  FVM_Object *constructors=0;assert(fvm_object_create(c,&constructors)==FVM_OK);
  assert(fvm_object_set(c,constructors,"Error",&klass)==FVM_OK);out=class_value(constructors);assert(fvm_set_global(c,"constructors",&out)==FVM_OK);
  FVM_Object *expected=stored.object;
  if(kind){int removed=0;assert(fvm_object_set_property_flags(c,error_class,"prototype",0,FVM_PROPERTY_VALID_FLAGS)==FVM_OK);
   assert(fvm_object_delete(c,error_class,"prototype",&removed)==FVM_OK && removed);
   if(kind==1)assert(fvm_object_define_accessor_rooted(c,error_class,"prototype",error_getter,0,0,error_replacement)==FVM_OK);
   if(kind==2){fvm_value_number(&out,17);assert(fvm_object_set(c,error_class,"prototype",&out)==FVM_OK);}
   if(kind==4){assert(fvm_object_define_method_rooted(c,error_class,"prototype",error_getter,0,error_replacement)==FVM_OK && fvm_object_get(c,error_class,"prototype",&out)==FVM_OK);expected=out.object;}
   else if(kind==5){assert(fvm_object_set(c,error_class,"PrOtOtYpE",&stored)==FVM_OK);expected=version<7?stored.object:0;}
   else expected=0;
  }
  if(kind>=6){
   if(kind==6 || kind==7){if(kind==6)assert(fvm_object_set(c,error_class,"prototype",&stored)==FVM_OK);else{fvm_value_number(&out,17);assert(fvm_object_set(c,error_class,"prototype",&out)==FVM_OK);}}
   const unsigned char end[]={0};FVM_Object *getter=0;
   assert(fvm_function_create_avm1(c,end,sizeof(end),0,0,0,&getter)==FVM_OK && fvm_function_set_body(c,getter,error_script_getter,0,error_replacement)==FVM_OK);
   if(kind==6 || kind==7){FVM_Value args[3];fvm_value_string(args,"prototype");args[1]=class_value(getter);fvm_value_undefined(args+2);args[2].type=FVM_VALUE_NULL;
    assert(fvm_object_call(c,error_class,"addProperty",3,args,&out)==FVM_OK && out.boolean);expected=kind==6?stored.object:0;}
   if(kind==8){FVM_Object *carrier=0;assert(fvm_object_create(c,&carrier)==FVM_OK && fvm_object_set(c,carrier,"prototype",&stored)==FVM_OK && fvm_object_set_prototype(c,error_class,carrier)==FVM_OK);}
   if(kind==9){out=class_value(getter);assert(fvm_object_set(c,error_class,"__resolve",&out)==FVM_OK);}
  }
  FVM_Object *argument=0;assert(fvm_object_create(c,&argument)==FVM_OK && fvm_object_define_method_rooted(c,argument,"valueOf",error_input,0,0)==FVM_OK && fvm_object_define_method_rooted(c,argument,"toString",error_input,0,0)==FVM_OK);
  input=class_value(argument);assert(fvm_set_global(c,"errorInput",&input)==FVM_OK);error_coercions=error_getters=0;
  FVM_Object *instance=0;
  if(!route)assert(fvm_function_construct(c,error_class,1,&input,&instance)==FVM_OK);
  else if(route==1)assert(fvm_construct(c,"Error",1,&input,&instance)==FVM_OK);
  else{class_mode=route>=5?route-5:route-2;Program code;code.variable("errorInput");code.number(1);if(route>=5){code.variable("constructors");code.text("Error");code.op(0x53);}else{code.text("Error");code.op(0x40);}code.finish();out=class_run(c,code);assert(out.type==FVM_VALUE_OBJECT);instance=out.object;}
  assert(instance && instance->prototype==expected && instance->resource_kind==FVM_RESOURCE_ERROR && !error_coercions && !error_getters);
  assert(fvm_object_get(c,instance,"message",&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && out.object==argument);
  assert(fvm_gc_pin(c,instance)==FVM_OK);assert(fvm_object_get(c,instance,"__constructor__",&out)==FVM_OK && out.object==error_class);
  int own=0;assert(fvm_object_has_own(c,instance,"constructor",&own)==FVM_OK && !!own==(version<7));
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->exception_pending);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(runtime);return 0;
}
