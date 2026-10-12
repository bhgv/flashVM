#define FVM_GEOMETRY_FIXTURE_ONLY
#include "standard_geometry.cpp"
static char events[64]; static unsigned position=0;
struct Coordinate { char read, write; FVM_Value value; };
static FVM_Result getter(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *u) {
    Coordinate *v=(Coordinate *)u; events[position++]=v->read; *out=v->value; return FVM_OK;
}
static FVM_Result setter(FVM_Context *,FVM_Object *,unsigned n,const FVM_Value *a,FVM_Value *out,void *u) {
    Coordinate *v=(Coordinate *)u; events[position++]=v->write; assert(n==1); v->value=a[0]; fvm_value_undefined(out); return FVM_OK;
}
static void reset() { position=0; std::memset(events,0,sizeof(events)); }
static void bind(FVM_Context *c,FVM_Object *o,const char *name,Coordinate *state,char read,char write,double n) {
    state->read=read;state->write=write;fvm_value_number(&state->value,n);
    assert(fvm_object_define_accessor_rooted(c,o,name,getter,setter,state,0)==FVM_OK);
}
static unsigned overrides=0;
static FVM_Result delta(FVM_Context *c,FVM_Object *self,unsigned n,const FVM_Value *a,FVM_Value *out,void *) {
    ++overrides; assert(n==1); near(get(c,self,"a"),-1); near(get(c,a[0].object,"x"),5); near(get(c,a[0].object,"y"),6);
    *out=a[0]; return FVM_OK;
}
int main() {
    FVM_Config cfg; FVM_Runtime *rt=0; FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK); assert(fvm_runtime_create(&cfg,&rt)==FVM_OK); assert(fvm_context_create(rt,&c)==FVM_OK); assert(fvm_standard_install(c)==FVM_OK);
    double xy[]={3,4}; FVM_Object *p=make(c,"flash.geom.Point",2,xy),*q=make(c,"flash.geom.Point",2,xy);
    FVM_Value str; fvm_value_string(&str,"A"); assert(fvm_object_set(c,p,"x",&str)==FVM_OK);
    FVM_Value arg=ref(q), out=call(c,p,"add",1,&arg), value;
    assert(fvm_object_get(c,out.object,"x",&value)==FVM_OK); assert(value.type==FVM_VALUE_STRING && std::strcmp(value.string,"A3")==0);
    FVM_Value offsets[2]; fvm_value_string(offsets,"B"); fvm_value_number(offsets+1,2); call(c,p,"offset",2,offsets);
    assert(fvm_object_get(c,p,"x",&value)==FVM_OK && std::strcmp(value.string,"AB")==0); near(get(c,p,"y"),6);
    const char nul[]={'X',0,'Y'}; fvm_value_string_bytes(&str,nul,3); assert(fvm_object_set(c,p,"x",&str)==FVM_OK);
    out=call(c,p,"add",1,&arg); assert(fvm_object_get(c,out.object,"x",&value)==FVM_OK && fvm_value_string_size(&value)==4); assert(std::memcmp(value.string,"X\0Y3",4)==0);
    FVM_Object *borrowed=0,*other=0; assert(fvm_object_create(c,&borrowed)==FVM_OK); assert(fvm_gc_pin(c,borrowed)==FVM_OK); assert(fvm_object_create(c,&other)==FVM_OK); assert(fvm_gc_pin(c,other)==FVM_OK);
    Coordinate x,y,ox,oy; bind(c,borrowed,"x",&x,'x','X',3); bind(c,borrowed,"y",&y,'y','Y',4); bind(c,other,"x",&ox,'u','U',2); bind(c,other,"y",&oy,'v','V',1);
    FVM_Value ctor,proto; assert(fvm_object_get(c,q,"constructor",&ctor)==FVM_OK); assert(fvm_object_get(c,ctor.object,"prototype",&proto)==FVM_OK);
    assert(fvm_object_set_prototype(c,borrowed,proto.object)==FVM_OK); arg=ref(other); reset(); out=call(c,borrowed,"add",1,&arg); assert(std::strcmp(events,"yvxu")==0); near(get(c,out.object,"x"),5);
    reset(); call(c,borrowed,"offset",2,offsets); assert(std::strcmp(events,"xXyY")==0);
    Coordinate length; bind(c,borrowed,"length",&length,'l','L',10); fvm_value_number(&x.value,3); fvm_value_number(&y.value,4);
    FVM_Value desired; fvm_value_number(&desired,20); reset(); call(c,borrowed,"normalize",1,&desired); assert(std::strcmp(events,"lxXyY")==0); near(x.value.number,6); near(y.value.number,8);
    FVM_Value ns,geom,point_class; assert(fvm_get_global(c,"flash",&ns)==FVM_OK); assert(fvm_object_get(c,ns.object,"geom",&geom)==FVM_OK); assert(fvm_object_get(c,geom.object,"Point",&point_class)==FVM_OK);
    fvm_value_string(&str,"2"); assert(fvm_object_set(c,q,"x",&str)==FVM_OK); assert(fvm_object_set(c,q,"y",&str)==FVM_OK);
    FVM_Value interpolation[3]={ref(borrowed),ref(q),desired}; fvm_value_number(interpolation+2,0.5); out=call(c,point_class.object,"interpolate",3,interpolation);
    assert(fvm_object_get(c,out.object,"x",&value)==FVM_OK && std::strcmp(value.string,"22")==0); assert(fvm_object_get(c,out.object,"y",&value)==FVM_OK && std::strcmp(value.string,"23")==0);
    double affine[]={2,4,3,4,5,6}; FVM_Object *m=make(c,"flash.geom.Matrix",6,affine);
    assert(fvm_object_define_method_rooted(c,m,"deltaTransformPoint",delta,0,0)==FVM_OK); call(c,m,"invert"); assert(overrides==1); near(get(c,m,"tx"),-5); near(get(c,m,"ty"),-6);
    FVM_Object *identity=make(c,"flash.geom.Matrix",0,0); fvm_value_string(&str,"T"); assert(fvm_object_set(c,identity,"tx",&str)==FVM_OK); call(c,identity,"translate",2,offsets); assert(fvm_object_get(c,identity,"tx",&value)==FVM_OK && std::strcmp(value.string,"TB")==0);
    FVM_Value box[5]; fvm_value_number(box,100); fvm_value_number(box+1,50); fvm_value_number(box+2,0); fvm_value_string(box+3,"X"); fvm_value_string(box+4,"Y");
    call(c,identity,"createGradientBox",5,box); assert(fvm_object_get(c,identity,"tx",&value)==FVM_OK && std::strcmp(value.string,"X50")==0); assert(fvm_object_get(c,identity,"ty",&value)==FVM_OK && std::strcmp(value.string,"Y25")==0);
    fvm_context_destroy(c); fvm_runtime_destroy(rt); return 0;
}
