/* Coordinate overflow rejects activation and edits before publishing any candidate state. */
#define main anchor_reference_main
#include "textfield_autosize_anchors.cpp"
#undef main
#include <limits>
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned anchor=1;anchor<=2;++anchor){FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c),*format=0;FVM_Value value,arg,out;
  assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);put_number(c,format,"size",40);fvm_value_undefined(&arg);arg.type=FVM_VALUE_OBJECT;arg.object=format;
  assert(fvm_object_call(c,field,"setNewTextFormat",1,&arg,&out)==FVM_OK);put_text(c,field,"text","AV");put_number(c,field,"_x",1000000);
  const char *mode=anchor==1?"center":"right";assert(fvm_value_string(&value,mode)==FVM_OK && fvm_object_set(c,field,"autoSize",&value)==FVM_UNSUPPORTED);
  assert(counted_metric(c,field,"_width")==100 && counted_metric(c,field,"_x")==1000000 && fvm_object_get(c,field,"autoSize",&out)==FVM_OK && !std::strcmp(out.string,"none"));
  put_number(c,field,"_x",0);put_text(c,field,"autoSize",mode);put_number(c,field,"_x",-1000000);
  assert(fvm_value_string(&value,"AVA")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);counted_text(c,field,"AV",2);
  assert(counted_metric(c,field,"_width")==52 && counted_metric(c,field,"_x")==-1000000);
  put_number(c,format,"size",80);assert(fvm_object_call(c,field,"setTextFormat",1,&arg,&out)==FVM_UNSUPPORTED && counted_metric(c,field,"_width")==52);
  put_number(c,field,"_x",1000000);assert(fvm_value_string(&value,"A")==FVM_OK && fvm_object_set(c,field,"text",&value)==FVM_UNSUPPORTED);counted_text(c,field,"AV",2);
  double invalid[]={std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()};
  for(unsigned i=0;i<3;++i){fvm_value_number(&value,invalid[i]);assert(fvm_object_set(c,field,"_x",&value)==FVM_INVALID_ARGUMENT && fvm_object_set(c,field,"_y",&value)==FVM_INVALID_ARGUMENT);}
  put_number(c,field,"_x",0);put_text(c,field,"text","AVA");assert(counted_metric(c,field,"_width")==76 && counted_metric(c,field,"_x")==-24.0*anchor*0.5);
  assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
