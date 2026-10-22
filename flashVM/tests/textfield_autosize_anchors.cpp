/* Horizontal fitting preserves the selected parent-space anchor; paths stay local. */
#include <cmath>
#define FVM_COUNTED_FIELD_FIXTURE_ONLY
#include "textfield_counted_text.cpp"
static void put_number(FVM_Context *c,FVM_Object *o,const char *key,double n)
{ FVM_Value v;fvm_value_number(&v,n);assert(fvm_object_set(c,o,key,&v)==FVM_OK); }
static void put_text(FVM_Context *c,FVM_Object *o,const char *key,const char *s)
{ FVM_Value v;assert(fvm_value_string(&v,s)==FVM_OK && fvm_object_set(c,o,key,&v)==FVM_OK); }
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned mode=0;mode<3;++mode){assert(fvm_context_create(rt,&c)==FVM_OK);
  FVM_Object *field=counted_field(c);FVM_Value value,out;FVM_Object *format=0;
  assert(fvm_construct(c,"TextFormat",0,0,&format)==FVM_OK);put_number(c,format,"size",40);
  fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=format;assert(fvm_object_call(c,field,"setNewTextFormat",1,&value,&out)==FVM_OK);
  put_number(c,field,"_x",10);put_number(c,field,"_y",-20);put_text(c,field,"text","AV");
  const char *name=mode==0?"left":mode==1?"center":"right";double fraction=mode*0.5;
  put_text(c,field,"autoSize",name);assert(counted_metric(c,field,"_width")==52 && counted_metric(c,field,"_x")==10+48*fraction);
  put_text(c,field,"text","AVA");assert(counted_metric(c,field,"_width")==76 && counted_metric(c,field,"_x")==10+24*fraction && counted_metric(c,field,"_y")==-20);
  put_number(c,field,"_x",-100);put_text(c,field,"text","A");assert(counted_metric(c,field,"_x")==-100+48*fraction);
  put_text(c,field,"autoSize","none");double x=counted_metric(c,field,"_x");put_text(c,field,"text","AVAV");assert(counted_metric(c,field,"_x")==x && counted_metric(c,field,"_width")==28);
  put_text(c,field,"autoSize",name);put_number(c,field,"_y",1000000);
  FVM_Value bad;fvm_value_number(&bad,1000001);assert(fvm_object_set(c,field,"_y",&bad)==FVM_INVALID_ARGUMENT && counted_metric(c,field,"_y")==1000000);
  FVM_Path *before=0,*after=0;assert(fvm_textfield_path(c,field,&before)==FVM_OK);put_number(c,field,"_x",123);
  assert(fvm_textfield_path(c,field,&after)==FVM_OK && fvm_path_count(before)==fvm_path_count(after));fvm_path_destroy(before);fvm_path_destroy(after);
  assert(fvm_gc_collect(c,0)==FVM_OK);fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
