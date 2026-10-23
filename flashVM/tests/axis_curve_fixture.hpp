#ifndef FVM_AXIS_CURVE_FIXTURE_HPP
#define FVM_AXIS_CURVE_FIXTURE_HPP
#include "round_curve_fixture.hpp"
static FVM_Path *ac_path(unsigned degree=2,bool vertical=false,bool reverse=false)
{ FVM_Path *p=0; assert(fvm_path_create(&p)==FVM_OK); double a=reverse?14:2,b=reverse?2:14; assert(hit_add(p,FVM_PATH_MOVE,vertical?8:a,vertical?a:8)==FVM_OK); if(degree==2) assert(hit_add(p,FVM_PATH_QUADRATIC,8,8,vertical?8:b,vertical?b:8)==FVM_OK); else assert(hit_add(p,FVM_PATH_CUBIC,vertical?8:(reverse?10:6),vertical?(reverse?10:6):8,vertical?8:(reverse?6:10),vertical?(reverse?6:10):8,vertical?8:b,vertical?b:8)==FVM_OK); return p; }
static void ac_expected(const FVM_PathBounds &b,unsigned cap,double width=4,bool vertical=false)
{ double r=width/2,lo=cap==1?2:2-r,hi=cap==1?14:14+r; if(vertical) bounds_equal(b,8-r,lo,8+r,hi); else bounds_equal(b,lo,8-r,hi,8+r); }
static FVM_Object *ac_clip(FVM_Context *c,unsigned cap,unsigned join,double width=4,unsigned kind=0)
{ FVM_Object *clip=ss_clip(c,cap,join,kind,1); FVM_Object *part=rc_part(clip),*cmd=part->properties["commands"].object->indexed[1].object; rc_number(cmd,"y0",8); rc_number(part,"width",width); return clip; }
static void ac_move(FVM_Context *c,FVM_Object *clip) { ss_xy(c,clip,"moveTo",100,100); }
#endif
