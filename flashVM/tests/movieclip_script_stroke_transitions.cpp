#include "script_stroke_fixture.hpp"
int main() {
 FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0; assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK); ss_context(rt,&c);
 FVM_Object *clip=ss_clip(c,2,1); FVM_Object *data=clip->resource_result.object,*first=data->properties["active"].object; FVM_Value a[8],out;
 ss_style(c,clip,1,0); ss_xy(c,clip,"moveTo",2,12); ss_xy(c,clip,"lineTo",14,12); FVM_Object *second=data->properties["active"].object;
 assert(first!=second && first->properties["lineCap"].number==2 && first->properties["lineJoin"].number==1 && second->properties["lineCap"].number==1 && second->properties.find("lineJoin")==second->properties.end());
 assert(fvm_object_call(c,clip,"clear",0,0,&out)==FVM_OK && data->properties.find("lineCap")==data->properties.end() && data->properties.find("lineJoin")==data->properties.end());
 fvm_value_number(a,4); fvm_value_number(a+1,0xff0000); assert(fvm_object_call(c,clip,"lineStyle",2,a,&out)==FVM_OK); ss_geometry(c,clip); ss_metadata(clip,0,0); ss_pixels(c,clip,0,0);
 fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
