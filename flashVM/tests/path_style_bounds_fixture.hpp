#ifndef FVM_PATH_STYLE_BOUNDS_FIXTURE_HPP
#define FVM_PATH_STYLE_BOUNDS_FIXTURE_HPP
#include "path_style_hit_fixture.hpp"
static void bound_near(double a,double b) { assert(std::fabs(a-b)<1e-8); }
static FVM_PathBounds stroke_bounds(FVM_Path *p,unsigned cap,unsigned join=0,double width=4,int expected=1)
{ FVM_PathBounds b; int any=-1; assert(fvm_path_stroke_bounds_style(p,width,cap,join,&b,&any)==FVM_OK && any==expected); return b; }
static void bounds_equal(const FVM_PathBounds &b,double x0,double y0,double x1,double y1)
{ bound_near(b.x_min,x0); bound_near(b.y_min,y0); bound_near(b.x_max,x1); bound_near(b.y_max,y1); }
#endif
