/* Counted formatting strings cannot enable unsupported effects by hiding a NUL suffix. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
#ifdef FVM_HAS_FONTS
 FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK && fvm_gc_pin(c,format)==FVM_OK);
 FVM_Value value,arg,out,text;assert(fvm_value_string(&text,"AB")==FVM_OK && fvm_object_set(c,field,"text",&text)==FVM_OK);double before=counted_metric(c,field,"textWidth");
 const char *names[]={"align","align","font","url","target"};const char *bad[]={"left\0right","center\0","Geometry\0Other","\0url","\0target"};const size_t sizes[]={10,7,14,4,7};
 for(unsigned i=0;i<5;++i){assert(fvm_value_string_bytes(&value,bad[i],sizes[i])==FVM_OK && fvm_object_set(c,format,names[i],&value)==FVM_OK);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
  assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED && counted_metric(c,field,"textWidth")==before);
  assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_UNSUPPORTED);
  assert(fvm_object_call(c,format,"getTextExtent",1,&text,&out)==FVM_UNSUPPORTED);
  fvm_value_undefined(&value);value.type=FVM_VALUE_NULL;assert(fvm_object_set(c,format,names[i],&value)==FVM_OK);
 }
 assert(fvm_value_string(&value,"left")==FVM_OK && fvm_object_set(c,format,"align",&value)==FVM_OK && fvm_value_string(&value,"Geometry")==FVM_OK && fvm_object_set(c,format,"font",&value)==FVM_OK);
 assert(fvm_value_string_bytes(&value,"not empty",0)==FVM_OK && fvm_object_set(c,format,"url",&value)==FVM_OK && fvm_object_set(c,format,"target",&value)==FVM_OK);
 assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK && fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK && fvm_object_call(c,format,"getTextExtent",1,&text,&out)==FVM_OK);
 assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);counted_text(c,field,"AB",2);fvm_context_destroy(c);
#endif
 fvm_runtime_destroy(rt);
}
