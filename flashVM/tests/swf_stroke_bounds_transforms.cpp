#include "swf_stroke_bounds_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
 assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
 const FVM_Affine transforms[]={{2,0,0,2,20,20},{0,2,-2,0,40,20},{-2,0,0,2,40,20},{0,-2,2,0,20,40}};
 for(unsigned type=0;type<4;++type) for(unsigned cap=0;cap<3;++cap) for(unsigned join=0;join<2;++join) {
  FVM_Object *root=load(c,bs_tags(caps_shape(type,cap,join)),1); tick(c,root); FVM_Object *node=display_node(root);
  for(unsigned m=0;m<4;++m) { const FVM_Affine &a=transforms[m]; assert(fvm_movieclip_set_matrix(c,node,&a)==FVM_OK);
   double xs[]={cap==1?2.0:0.0,cap==1?14.0:16.0},ys[]={6,10}; FVM_PathBounds expected={1e9,1e9,-1e9,-1e9};
   for(unsigned x=0;x<2;++x) for(unsigned y=0;y<2;++y) { double px=a.xx*xs[x]+a.xy*ys[y]+a.x0,py=a.yx*xs[x]+a.yy*ys[y]+a.y0;
    if(px<expected.x_min) expected.x_min=px; if(px>expected.x_max) expected.x_max=px; if(py<expected.y_min) expected.y_min=py; if(py>expected.y_max) expected.y_max=py;
   }
   FVM_PathBounds b; assert(clip_bounds(c,root,&b)==FVM_OK); bounds_equal(b,expected.x_min,expected.y_min,expected.x_max,expected.y_max);
  }
  const FVM_Affine collapsed={0,0,0,0,20,20}; assert(fvm_movieclip_set_matrix(c,node,&collapsed)==FVM_OK);
  FVM_PathBounds empty; assert(clip_bounds(c,root,&empty)==FVM_OK);
  if(cap || join) bounds_equal(empty,0,0,0,0); else bounds_equal(empty,20,20,20,20);
  assert(fvm_gc_unpin(c,root)==FVM_OK); assert(fvm_gc_collect(c,0)==FVM_OK);
 }
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
