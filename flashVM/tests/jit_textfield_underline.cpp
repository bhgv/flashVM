/* AVM1 formatting changes actual exported geometry equally in interpreter and native JIT. */
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
int main()
{
 if(!fvm_jit_available())return 0;
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(class_mode=0;class_mode<3;++class_mode){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
  const unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
  assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
  FVM_Object *field=counted_field(c),*format=0;FVM_Value value,out,arg;
  assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);
  value=class_value(field);assert(fvm_set_global(c,"field",&value)==FVM_OK);
  value=class_value(format);assert(fvm_set_global(c,"format",&value)==FVM_OK);
  fvm_value_number(&value,40);assert(fvm_object_set(c,format,"size",&value)==FVM_OK);
  assert(fvm_value_string(&value,"AV")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_OK);
  arg=class_value(format);assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_OK);
  FVM_Path *path=0;assert(fvm_textfield_path(c,field,&path)==FVM_OK);size_t plain=fvm_path_count(path);fvm_path_destroy(path);
  for(unsigned step=0;step<3;++step){Program p;bool underlined=step==1;
   p.variable("format");p.text("underline");p.number(underlined?1:0);p.op(0x4f);
   p.variable("format");p.number(1);p.variable("field");p.text("setTextFormat");p.op(0x52);p.op(0x17);
   p.number(0);p.variable("field");p.text("getTextFormat");p.op(0x52);p.finish();out=class_run(c,p);
   assert(out.type==FVM_VALUE_OBJECT && fvm_object_get(c,out.object,"underline",&value)==FVM_OK && value.type==FVM_VALUE_BOOLEAN && !!value.boolean==underlined);
   assert(fvm_textfield_path(c,field,&path)==FVM_OK && fvm_path_count(path)==plain+(underlined?5:0));fvm_path_destroy(path);
   assert(counted_metric(c,field,"textWidth")==48 && fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);
  }
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
