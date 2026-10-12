#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include "flashvm/transfer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
static FVM_Value ref(FVM_Object *o) { FVM_Value v; fvm_value_undefined(&v); v.type=FVM_VALUE_OBJECT; v.object=o; return v; }
static FVM_Object *make(FVM_Context *c, const char *name, unsigned n, const double *data) {
    FVM_Value a[6]; for(unsigned i=0;i<n;++i) fvm_value_number(a+i,data[i]);
    FVM_Object *o=0; assert(fvm_construct(c,name,n,a,&o)==FVM_OK); assert(fvm_gc_pin(c,o)==FVM_OK); return o;
}
static double get(FVM_Context *c,FVM_Object *o,const char *name) {
    FVM_Value v; assert(fvm_object_get(c,o,name,&v)==FVM_OK); assert(v.type==FVM_VALUE_NUMBER); return v.number;
}
static void near(double a,double b) { assert(std::fabs(a-b)<1e-10); }
static FVM_Value call(FVM_Context *c,FVM_Object *o,const char *name,unsigned n=0,const FVM_Value *a=0) {
    FVM_Value v; assert(fvm_object_call(c,o,name,n,a,&v)==FVM_OK); return v;
}
static void exercise(FVM_Context *c,FVM_Object *matrix,FVM_Object *point) {
    FVM_Value p=ref(point), out=call(c,matrix,"transformPoint",1,&p);
    near(get(c,out.object,"x"),23); near(get(c,out.object,"y"),34);
    out=call(c,matrix,"deltaTransformPoint",1,&p);
    near(get(c,out.object,"x"),18); near(get(c,out.object,"y"),28);
    FVM_Value clone=call(c,matrix,"clone"); assert(clone.object!=matrix); assert(fvm_gc_pin(c,clone.object)==FVM_OK);
    call(c,clone.object,"invert"); FVM_Value m=ref(matrix); call(c,clone.object,"concat",1,&m);
    near(get(c,clone.object,"a"),1); near(get(c,clone.object,"d"),1);
    near(get(c,clone.object,"b"),0); near(get(c,clone.object,"c"),0);
    near(get(c,clone.object,"tx"),0); near(get(c,clone.object,"ty"),0);
    out=call(c,point,"clone"); assert(out.object!=point); assert(fvm_gc_pin(c,out.object)==FVM_OK);
    FVM_Value other=ref(out.object); out=call(c,point,"equals",1,&other); assert(out.type==FVM_VALUE_BOOLEAN && out.boolean);
    out=call(c,point,"toString"); assert(std::strcmp(out.string,"(x=3, y=4)")==0);
    near(get(c,point,"length"),5);
    FVM_Value desired; fvm_value_number(&desired,10); call(c,other.object,"normalize",1,&desired);
    near(get(c,other.object,"x"),6); near(get(c,other.object,"y"),8);
}
#ifndef FVM_GEOMETRY_FIXTURE_ONLY
int main() {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0,*receiver=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);
    assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_context_create(rt,&receiver)==FVM_OK);
    assert(fvm_standard_install(c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    FVM_Value ns,geom,klass; assert(fvm_get_global(c,"flash",&ns)==FVM_OK);
    assert(fvm_object_get(c,ns.object,"geom",&geom)==FVM_OK); assert(fvm_object_get(c,geom.object,"Point",&klass)==FVM_OK);
    double xy[]={3,4}, affine[]={2,4,3,4,5,6};
    FVM_Object *p=make(c,"flash.geom.Point",2,xy),*m=make(c,"flash.geom.Matrix",6,affine);
    exercise(c,m,p);
    FVM_Object *identity=make(c,"flash.geom.Matrix",0,0); near(get(c,identity,"a"),1); near(get(c,identity,"d"),1); near(get(c,identity,"tx"),0);
    FVM_Object *partial=make(c,"flash.geom.Point",1,xy); FVM_Value undefined; assert(fvm_object_get(c,partial,"y",&undefined)==FVM_OK && undefined.type==FVM_VALUE_UNDEFINED);
    FVM_Object *plain=0; assert(fvm_object_create(c,&plain)==FVM_OK); FVM_Value value; fvm_value_number(&value,3); assert(fvm_object_set(c,plain,"x",&value)==FVM_OK);
    fvm_value_number(&value,4); assert(fvm_object_set(c,plain,"y",&value)==FVM_OK); value=ref(plain); FVM_Value equal=call(c,p,"equals",1,&value); assert(!equal.boolean);
    FVM_Value gradient[5]; double box[]={100,50,0,7,9}; for(unsigned i=0;i<5;++i) fvm_value_number(gradient+i,box[i]);
    call(c,identity,"createGradientBox",5,gradient); near(get(c,identity,"a"),100/1638.4); near(get(c,identity,"d"),50/1638.4); near(get(c,identity,"tx"),57); near(get(c,identity,"ty"),34);
    FVM_Value pa=ref(p), result=call(c,p,"add",1,&pa); near(get(c,result.object,"x"),6); near(get(c,result.object,"y"),8);
    result=call(c,p,"subtract",1,&pa); near(get(c,result.object,"x"),0); near(get(c,result.object,"y"),0);
    FVM_Value pair[]={pa,pa}; result=call(c,klass.object,"distance",2,pair); near(result.number,0);
    FVM_Value interpolated[3]={pa,pa,gradient[2]}; result=call(c,klass.object,"interpolate",3,interpolated); near(get(c,result.object,"x"),3);
    FVM_Value polar[2]; fvm_value_number(polar,5); fvm_value_number(polar+1,0); result=call(c,klass.object,"polar",2,polar); near(get(c,result.object,"x"),5); near(get(c,result.object,"y"),0);
    FVM_Value offset[2]; fvm_value_number(offset,2); fvm_value_number(offset+1,-1); FVM_Value pc=call(c,p,"clone"); assert(fvm_gc_pin(c,pc.object)==FVM_OK);
    call(c,pc.object,"offset",2,offset); near(get(c,pc.object,"x"),5); near(get(c,pc.object,"y"),3);
    call(c,identity,"identity"); call(c,identity,"translate",2,offset); near(get(c,identity,"tx"),2); near(get(c,identity,"ty"),-1);
    call(c,identity,"scale",2,offset); near(get(c,identity,"a"),2); near(get(c,identity,"d"),-1); near(get(c,identity,"tx"),4); near(get(c,identity,"ty"),1);
    FVM_Value angle; fvm_value_number(&angle,3.14159265358979323846/2); call(c,identity,"rotate",1,&angle);
    near(get(c,identity,"a"),0); near(get(c,identity,"b"),2); near(get(c,identity,"c"),1); near(get(c,identity,"tx"),-1); near(get(c,identity,"ty"),4);
    call(c,identity,"createBox",5,gradient); near(get(c,identity,"a"),100); near(get(c,identity,"d"),50); near(get(c,identity,"tx"),7); near(get(c,identity,"ty"),9);
    result=call(c,identity,"toString"); assert(std::strcmp(result.string,"(a=100, b=0, c=0, d=50, tx=7, ty=9)")==0);
    double singular[]={1,1,1,1,8,9}; FVM_Object *singular_m=make(c,"flash.geom.Matrix",6,singular); call(c,singular_m,"invert"); near(get(c,singular_m,"a"),1); near(get(c,singular_m,"b"),0); near(get(c,singular_m,"tx"),0);
    FVM_Value self=ref(m), before=call(c,m,"clone"); assert(fvm_gc_pin(c,before.object)==FVM_OK); call(c,before.object,"concat",1,&self);
    FVM_Object *self_m=make(c,"flash.geom.Matrix",6,affine); self=ref(self_m); call(c,self_m,"concat",1,&self);
    const char *keys[]={"a","b","c","d","tx","ty"}; for(unsigned i=0;i<6;++i) near(get(c,self_m,keys[i]),get(c,before.object,keys[i]));
    FVM_Value packet=ref(m),received; FVM_Channel *ch=0; assert(fvm_channel_create(&ch)==FVM_OK);
    assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK); assert(fvm_channel_receive_value(receiver,ch,&received,0)==FVM_OK); assert(fvm_gc_pin(receiver,received.object)==FVM_OK); fvm_channel_destroy(ch);
    FVM_Object *received_matrix = received.object;
    assert(fvm_channel_create(&ch)==FVM_OK); packet=ref(p);
    assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK); assert(fvm_channel_receive_value(receiver,ch,&received,0)==FVM_OK); assert(fvm_gc_pin(receiver,received.object)==FVM_OK); fvm_channel_destroy(ch);
    assert(fvm_gc_collect(receiver,0)==FVM_OK); exercise(receiver,received_matrix,received.object);
    received=ref(received_matrix); packet=ref(m);
    FVM_Value transformed=call(receiver,received.object,"clone"); assert(fvm_gc_pin(receiver,transformed.object)==FVM_OK);
    FVM_Value proto; assert(fvm_object_get(receiver,received.object,"constructor",&proto)==FVM_OK);
    assert(fvm_gc_collect(receiver,0)==FVM_OK); call(receiver,received.object,"invert"); call(receiver,received.object,"invert"); near(get(receiver,received.object,"a"),2);
    assert(fvm_channel_open_file("geometry-test.bin","w",&ch)==FVM_OK || !ch);
    if(ch) { assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK); fvm_channel_destroy(ch); assert(fvm_channel_open_file("geometry-test.bin","r",&ch)==FVM_OK); assert(fvm_channel_receive_value(receiver,ch,&received,0)==FVM_OK); call(receiver,received.object,"clone"); fvm_channel_destroy(ch); assert(std::remove("geometry-test.bin")==0); }
    fvm_context_destroy(receiver); fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}

#endif
