/* Registered clip identities use the symbol movie version before constructors, across scene paths and transfer. */
#define FVM_SYMBOL_PROVENANCE_FIXTURE_ONLY
#include "movieclip_registered_provenance.cpp"
#include "../src/standard/movieclip_construction.hpp"
struct IdentityProbe { unsigned version, calls; bool initial, raising; FVM_Object *constructor; };
static void clip_identity(FVM_Context *c, FVM_Object *clip, const IdentityProbe &probe)
{
 FVM_Value value;int own=0;unsigned flags=0;
 assert(fvm_object_has_own(c,clip,"__constructor__",&own)==FVM_OK && own && fvm_object_get(c,clip,"__constructor__",&value)==FVM_OK && value.object==probe.constructor);
 assert(fvm_object_property_flags(c,clip,"__constructor__",&flags)==FVM_OK && flags==FVM_PROPERTY_DONT_ENUM);
 assert(fvm_object_has_own(c,clip,"constructor",&own)==FVM_OK && !!own==(probe.version<7 || probe.initial));
 if(own){assert(fvm_object_get(c,clip,"constructor",&value)==FVM_OK);if(probe.version<7)assert(value.type==FVM_VALUE_OBJECT && value.object==probe.constructor);else assert(value.type==FVM_VALUE_NUMBER && value.number==23);assert(fvm_object_property_flags(c,clip,"constructor",&flags)==FVM_OK && flags==(probe.version<7?FVM_PROPERTY_DONT_ENUM:0));}
}
static FVM_Result clip_identity_body(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *userdata)
{
 IdentityProbe &probe=*(IdentityProbe *)userdata;++probe.calls;assert(frame->function==probe.constructor && frame->receiver->resource_kind==FVM_RESOURCE_MOVIECLIP);clip_identity(c,frame->receiver,probe);
 *out=case_object(frame->receiver);return probe.raising?fvm_exception_raise(c,out):FVM_OK;
}
static FVM_Object *identity_register(FVM_Context *c,unsigned version,IdentityProbe *probe)
{
 case_loaded(c,version);FVM_Value klass,ctor=registered_constructor(c);assert(fvm_get_global(c,"Object",&klass)==FVM_OK);case_register(c,klass.object,"Hero",4,ctor.object);
 probe->constructor=ctor.object;assert(fvm_function_set_body(c,ctor.object,clip_identity_body,probe,0)==FVM_OK);return ctor.object;
}
int main()
{
 FVM_Config cfg;FVM_Runtime *runtime=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
 for(unsigned source=5;source<=8;++source)for(unsigned ambient=6;ambient<=8;ambient+=2)for(unsigned initial=0;initial<2;++initial){
  FVM_Context *c=0;assert(fvm_context_create(runtime,&c)==FVM_OK);case_loaded(c,8);assert(fvm_standard_install(c)==FVM_OK);
  IdentityProbe probes[2]={{source,0,initial!=0,false,0},{source,0,initial!=0,false,0}};identity_register(c,6,probes);identity_register(c,8,probes+1);IdentityProbe &probe=probes[source<7?0:1];case_loaded(c,ambient);
  FVM_Object *root=symbol_root(c,source),*init=0;assert(fvm_object_create(c,&init)==FVM_OK && fvm_gc_pin(c,init)==FVM_OK);FVM_Value value;fvm_value_number(&value,17);assert(fvm_object_set(c,init,"__constructor__",&value)==FVM_OK);fvm_value_number(&value,23);assert(fvm_object_set(c,init,"constructor",&value)==FVM_OK);
  FVM_Object *child=attach(c,root,"Hero","dynamic",3,initial?init:0);assert(child && probe.calls==1);clip_identity(c,child,probe);
  FVM_Value instance,depth,initializer,out;fvm_value_string(&instance,"copy");fvm_value_number(&depth,4);initializer=initial?case_object(init):FVM_Value();
  assert(fvm_exec_duplicate_clip(c,child,&instance,&depth,&initializer,&out)==FVM_OK && out.type==FVM_VALUE_OBJECT && probe.calls==2);clip_identity(c,out.object,probe);
  probe.initial=false;assert(fvm_movieclip_advance(c,root,1)==FVM_OK && probe.calls==3);value=get(c,root,"actor");clip_identity(c,value.object,probe);assert(fvm_gc_collect(c,0)==FVM_OK && c->swf_info.version==ambient);
  probe.raising=true;FVM_Value args[3];fvm_value_string(args,"Hero");fvm_value_string(args+1,"broken");fvm_value_number(args+2,7);
  assert(fvm_object_call(c,root,"attachMovie",3,args,&out)==FVM_EXCEPTION && fvm_exception_get(c,&value)==FVM_OK);clip_identity(c,value.object,probe);assert(fvm_gc_collect(c,0)==FVM_OK && fvm_exception_clear(c)==FVM_OK);probe.raising=false;
  FVM_Context *other=0;FVM_Channel *channel=0;assert(fvm_context_create(runtime,&other)==FVM_OK && fvm_channel_create(&channel)==FVM_OK);case_loaded(other,source<7?8:6);value=case_object(root);
  assert(fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&value,0)==FVM_OK && fvm_gc_pin(other,value.object)==FVM_OK && fvm_gc_collect(other,0)==FVM_OK);
  FVM_Object *copy=value.object,*library=copy->resource_result.object->properties["library"].object,*proto=0,*constructor=0;assert(fvm_movieclip_sprite_class(other,library,"Hero",proto,constructor)==FVM_OK && constructor);
  IdentityProbe imported={source,0,false,false,constructor};assert(fvm_function_set_body(other,constructor,clip_identity_body,&imported,0)==FVM_OK);FVM_Object *transported=attach(other,copy,"Hero","transported",8);assert(transported && imported.calls==1);clip_identity(other,transported,imported);
  assert(!c->gc_frames && !other->gc_frames && !c->gc_native_depth && !other->gc_native_depth);fvm_channel_destroy(channel);fvm_context_destroy(other);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(runtime);return 0;
}
