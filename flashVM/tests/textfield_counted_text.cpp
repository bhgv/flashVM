/* Counted TextField content survives layout, edits, traced GC and binary graph transfer. */
#include "flashvm/standard_text.h"
#include "flashvm/standard.h"
#include "flashvm/serialization.h"
#include "flashvm/transfer.h"
#include "flashvm/gc.h"
#include "fixtures/font_data.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <cstring>
#ifdef FVM_HAS_FONTS
static FVM_Object *counted_field(FVM_Context *c)
{
 assert(fvm_standard_text_install(c)==FVM_OK && fvm_standard_text_register_font(c,"Geometry",fvm_test_font,sizeof(fvm_test_font),0)==FVM_OK);
 FVM_Object *field=0;assert(fvm_textfield_create(c,0,100,100,&field)==FVM_OK && fvm_gc_pin(c,field)==FVM_OK);return field;
}
static void counted_text(FVM_Context *c,FVM_Object *field,const char *text,size_t size)
{FVM_Value value;assert(fvm_object_get(c,field,"text",&value)==FVM_OK && value.type==FVM_VALUE_STRING && fvm_value_string_size(&value)==size && !std::memcmp(value.string,text,size));}
static double counted_metric(FVM_Context *c,FVM_Object *field,const char *name)
{FVM_Value value;assert(fvm_object_get(c,field,name,&value)==FVM_OK && value.type==FVM_VALUE_NUMBER);return value.number;}
#endif
#ifndef FVM_COUNTED_FIELD_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 for(unsigned version=0;version<=8;++version){if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
  FVM_Object *field=counted_field(c),*prefix=0;assert(fvm_textfield_create(c,0,100,100,&prefix)==FVM_OK && fvm_gc_pin(c,prefix)==FVM_OK);FVM_Value value,args[3],out;
  assert(fvm_value_string_bytes(&value,"A\0B",3)==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);counted_text(c,field,"A\0B",3);assert(counted_metric(c,field,"length")==3);
  assert(fvm_value_string_bytes(&value,"A\0",2)==FVM_OK && fvm_object_set(c,prefix,"text",&value)==FVM_OK);assert(counted_metric(c,field,"textWidth")>counted_metric(c,prefix,"textWidth"));
  FVM_Object *format=0;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);assert(fvm_value_string_bytes(args,"A\0B",3)==FVM_OK && fvm_object_call(c,format,"getTextExtent",1,args,&out)==FVM_OK);
  assert(counted_metric(c,out.object,"width")==counted_metric(c,field,"textWidth"));
  fvm_value_number(args,1);fvm_value_number(args+1,2);assert(fvm_value_string_bytes(args+2,"X\0Y",3)==FVM_OK && fvm_object_call(c,field,"replaceText",3,args,&out)==FVM_OK);counted_text(c,field,"AX\0YB",5);assert(counted_metric(c,field,"length")==5);
  assert(fvm_object_call(c,field,"getTextFormat",2,args,&out)==FVM_OK);assert(fvm_object_get(c,field,"text",args+2)==FVM_OK);fvm_value_number(args,0);fvm_value_number(args+1,5);
  assert(fvm_object_call(c,field,"replaceText",3,args,&out)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);counted_text(c,field,"AX\0YB",5);
  FVM_Context *other=0;assert(fvm_context_create(rt,&other)==FVM_OK && fvm_standard_text_install(other)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=field;unsigned char *data=0;size_t size=0;
  assert(fvm_value_serialize(c,&value,&data,&size)==FVM_OK && fvm_value_deserialize(other,data,size,&out)==FVM_OK && fvm_gc_pin(other,out.object)==FVM_OK);fvm_serialized_free(data);
  assert(fvm_gc_collect(other,0)==FVM_OK);counted_text(other,out.object,"AX\0YB",5);assert(counted_metric(other,out.object,"length")==5 && counted_metric(other,out.object,"textWidth")==counted_metric(c,field,"textWidth"));
  FVM_Channel *channel=0;assert(fvm_channel_create(&channel)==FVM_OK && fvm_channel_send_value(c,channel,&value,0)==FVM_OK && fvm_channel_receive_value(other,channel,&out,0)==FVM_OK);counted_text(other,out.object,"AX\0YB",5);fvm_channel_destroy(channel);
  assert(!c->gc_frames && !c->gc_native_depth && !other->gc_frames && !other->gc_native_depth);fvm_context_destroy(other);fvm_context_destroy(c);
 }
#endif
 fvm_runtime_destroy(rt);
}
#endif
