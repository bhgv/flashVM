/* Wrapped automatic fields preserve both horizontal edges for every anchor. */
#define main anchor_reference_main
#include "textfield_autosize_anchors.cpp"
#undef main
int main()
{
#ifdef FVM_HAS_FONTS
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned mode=0;mode<3;++mode){assert(fvm_context_create(rt,&c)==FVM_OK);FVM_Object *field=counted_field(c);FVM_Value v;
  put_number(c,field,"_x",-10);put_number(c,field,"_y",25);put_number(c,field,"_width",30);
  fvm_value_boolean(&v,1);assert(fvm_object_set(c,field,"wordWrap",&v)==FVM_OK && fvm_object_set(c,field,"multiline",&v)==FVM_OK);
  put_text(c,field,"autoSize",mode==0?"left":mode==1?"center":"right");put_text(c,field,"text","AV AV AV AV");
  assert(counted_metric(c,field,"_width")==30 && counted_metric(c,field,"_x")==-10 && counted_metric(c,field,"_y")==25);
  double h=counted_metric(c,field,"_height");put_number(c,field,"_width",20);assert(counted_metric(c,field,"_x")==-10 && counted_metric(c,field,"_height")>=h);
  fvm_value_boolean(&v,0);assert(fvm_object_set(c,field,"wordWrap",&v)==FVM_OK);double w=counted_metric(c,field,"_width");
  assert(std::fabs(counted_metric(c,field,"_x")-(-10+(20-w)*mode*0.5))<1e-9);
  put_text(c,field,"text","");assert(counted_metric(c,field,"_width")==4);
  fvm_context_destroy(c);
 }
 fvm_runtime_destroy(rt);
#endif
 return 0;
}
