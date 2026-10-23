#include "subnormal_stroke_fixture.hpp"
int main() {
FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *ctx=0;assert(fvm_config_init(&cfg)==FVM_OK&&fvm_runtime_create(&cfg,&rt)==FVM_OK);ss_context(rt,&ctx);
for(int e=-520;e>=-540;e-=2) for(unsigned c=0;c<3;++c) for(unsigned j=0;j<2;++j) {
 double s=std::ldexp(1.0,e); FVM_Object *clip=lc_clip(ctx,c,j,4*s),*cmds=rc_part(clip)->properties["commands"].object;
 for(unsigned i=0;i<2;++i) { FVM_Object *cmd=cmds->indexed[i].object;const char *keys[]={"x0","y0","x1","y1"}; for(unsigned k=0;k<(i?4U:2U);++k)rc_number(cmd,keys[k],cmd->properties[keys[k]].number*s); }
 lc_expected(sn_unit(rb_bounds(ctx,clip),s),c);FVM_Value w,h;assert(fvm_object_get(ctx,clip,"_width",&w)==FVM_OK&&fvm_object_get(ctx,clip,"_height",&h)==FVM_OK);FVM_PathBounds b=sn_unit(rb_bounds(ctx,clip),s);bound_near(w.number/s,b.x_max-b.x_min);bound_near(h.number/s,b.y_max-b.y_min);
}fvm_context_destroy(ctx);fvm_runtime_destroy(rt);
return 0; }
