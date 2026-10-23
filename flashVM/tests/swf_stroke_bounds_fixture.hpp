#ifndef FVM_SWF_STROKE_BOUNDS_FIXTURE_HPP
#define FVM_SWF_STROKE_BOUNDS_FIXTURE_HPP
#include "swf_stroke_caps_fixture.hpp"
#include "path_style_bounds_fixture.hpp"
static FVM_Object *bounds_part(FVM_Object *root)
{ return root->resource_result.object->properties["library"].object->properties["sprites"].object->indexed[9].object->properties["paths"].object->indexed[0].object; }
static FVM_Result clip_bounds(FVM_Context *c,FVM_Object *root,FVM_PathBounds *out,const char *method="getBounds")
{ FVM_Value a,v; fvm_value_undefined(&a); a.type=FVM_VALUE_OBJECT; a.object=root; FVM_Result r=fvm_object_call(c,root,method,1,&a,&v); if(r!=FVM_OK) return r;
 assert(v.type==FVM_VALUE_OBJECT); out->x_min=v.object->properties["xMin"].number; out->y_min=v.object->properties["yMin"].number; out->x_max=v.object->properties["xMax"].number; out->y_max=v.object->properties["yMax"].number; return FVM_OK; }
static void selected_bounds(FVM_Context *c,FVM_Object *root,unsigned cap,unsigned join=0,unsigned width=80)
{ FVM_PathBounds b; assert(clip_bounds(c,root,&b)==FVM_OK); double radius=(width?width/20.0:1)/2;
 if(!cap && !join && !width) radius=0;
 bounds_equal(b,cap==1?2:2-radius,8-radius,cap==1?14:14+radius,8+radius); }
static void bounds_number(FVM_Object *o,const char *key,double n) { FVM_Value v; fvm_value_number(&v,n); o->properties[key]=v; }
#endif
