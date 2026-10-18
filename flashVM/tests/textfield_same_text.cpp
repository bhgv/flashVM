/* Identical loaded text assignments preserve current format while changed text uses future format. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#ifdef FVM_HAS_FONTS
static void same_future(FVM_Context *c,FVM_Object *field,double size)
{
 FVM_Object *format=0;FVM_Value value,out;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);fvm_value_number(&value,size);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&value,&out)==FVM_OK);
}
static double same_format(FVM_Context *c,FVM_Object *field,bool future)
{FVM_Value value;assert(fvm_object_call(c,field,future?"getNewTextFormat":"getTextFormat",0,0,&value)==FVM_OK);return counted_metric(c,value.object,"size");}
static void same_load(FVM_Context *c,unsigned version)
{if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}}
#endif
#ifndef FVM_SAME_FIELD_FIXTURE_ONLY
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 const char *texts[]={"","AB","A\0B"};const size_t sizes[]={0,2,3};const char *changed[]={"A","CD","A\0C"};const size_t changed_sizes[]={1,2,3};
 for(unsigned version=0;version<=8;++version){if(version && version<4)continue;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);same_load(c,version);FVM_Object *first=counted_field(c);
  for(unsigned i=0;i<3;++i){FVM_Object *field=first;if(i)assert(fvm_textfield_create(c,0,100,100,&field)==FVM_OK && fvm_gc_pin(c,field)==FVM_OK);FVM_Value value;
   assert(fvm_value_string_bytes(&value,texts[i],sizes[i])==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);same_future(c,field,20);assert(same_format(c,field,false)==12 && same_format(c,field,true)==20);double width=counted_metric(c,field,"textWidth");
   assert(fvm_object_set(c,field,"text",&value)==FVM_OK && same_format(c,field,false)==(version?12:20));if(version)assert(counted_metric(c,field,"textWidth")==width);
   assert(fvm_value_string_bytes(&value,changed[i],changed_sizes[i])==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && same_format(c,field,false)==20);counted_text(c,field,changed[i],changed_sizes[i]);
   same_future(c,field,30);assert(fvm_object_get(c,field,"text",&value)==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK && same_format(c,field,false)==(version?20:30) && same_format(c,field,true)==30);
   assert(fvm_gc_collect(c,0)==FVM_OK);counted_text(c,field,changed[i],changed_sizes[i]);assert(!c->gc_frames && !c->gc_native_depth);
  }
  fvm_context_destroy(c);
 }
#endif
 fvm_runtime_destroy(rt);
}
#endif
