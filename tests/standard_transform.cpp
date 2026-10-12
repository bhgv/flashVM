#define FVM_MOVIECLIP_GRADIENT_FIXTURE_ONLY
#include "movieclip_gradient.cpp"
#include "flashvm/serialization.h"
#include "flashvm/capability.h"
#include "flashvm/extensions.h"
static double num(FVM_Context *c,FVM_Object *o,const char *key) {FVM_Value v;assert(fvm_object_get(c,o,key,&v)==FVM_OK && v.type==FVM_VALUE_NUMBER);return v.number;}
static void near(double a,double b) {assert(std::fabs(a-b)<1e-9);}
static FVM_Object *property(FVM_Context *c,FVM_Object *o,const char *key) {FVM_Value v;assert(fvm_object_get(c,o,key,&v)==FVM_OK && v.type==FVM_VALUE_OBJECT);assert(fvm_gc_pin(c,v.object)==FVM_OK);return v.object;}
static FVM_Object *construct(FVM_Context *c,const char *name,unsigned n,const double *values) {FVM_Value a[8];for(unsigned i=0;i<n;++i)fvm_value_number(a+i,values[i]);FVM_Object *o=0;assert(fvm_construct(c,name,n,a,&o)==FVM_OK);assert(fvm_gc_pin(c,o)==FVM_OK);return o;}
static void assign(FVM_Context *c,FVM_Object *o,const char *name,FVM_Object *value) {FVM_Value v;ref(&v,value);assert(fvm_object_set(c,o,name,&v)==FVM_OK);}
static void matrix(FVM_Context *c,FVM_Object *o,const double *expected) {const char *names[]={"a","b","c","d","tx","ty"};for(unsigned i=0;i<6;++i)near(num(c,o,names[i]),expected[i]);}
static void color(FVM_Context *c,FVM_Object *o,const double *expected) {const char *names[]={"redMultiplier","greenMultiplier","blueMultiplier","alphaMultiplier","redOffset","greenOffset","blueOffset","alphaOffset"};for(unsigned i=0;i<8;++i)near(num(c,o,names[i]),expected[i]);}
static void restored(FVM_Context *c,FVM_Object *bundle) {
    FVM_Object *clip=property(c,bundle,"clip"),*t=property(c,bundle,"transform");double pose[]={1,0.25,0.5,1,4,5};matrix(c,property(c,t,"matrix"),pose);
    double world[]={2,0.75,1,3,18,35};matrix(c,property(c,t,"concatenatedMatrix"),world);
    double local[]={1,0.5,0.25,1.5,2,4,6,-100},global[]={0.5,0.125,0.25,0.75,11,21,36,-10};color(c,property(c,t,"colorTransform"),local);color(c,property(c,t,"concatenatedColorTransform"),global);
    assert(fvm_gc_collect(c,0)==FVM_OK);matrix(c,property(c,t,"matrix"),pose);
    FVM_Object *changed=property(c,t,"matrix");set(c,changed,"tx",9);assign(c,t,"matrix",changed);near(num(c,clip,"_x"),9);
    FVM_Object *getter=property(c,clip,"transform");assert(getter!=t);near(num(c,property(c,getter,"matrix"),"tx"),9);
}
#ifndef FVM_TRANSFORM_FIXTURE_ONLY
int main() {
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0,*other=0;assert(fvm_config_init(&cfg)==FVM_OK);assert(fvm_runtime_create(&cfg,&rt)==FVM_OK);assert(fvm_context_create(rt,&c)==FVM_OK);assert(fvm_context_create(rt,&other)==FVM_OK);assert(fvm_standard_install(c)==FVM_OK);
    FVM_Object *root=0;assert(fvm_movieclip_create(c,"root",1,&root)==FVM_OK);assert(fvm_gc_pin(c,root)==FVM_OK);FVM_Object *child=new_child(c,root,"child",1);
    FVM_Object *parent_t=property(c,root,"transform"),*t=property(c,child,"transform");
    double parent[]={2,0,0,3,10,20},local[]={1,0.25,0.5,1,4,5};FVM_Object *parent_m=construct(c,"flash.geom.Matrix",6,parent),*local_m=construct(c,"flash.geom.Matrix",6,local);
    assign(c,parent_t,"matrix",parent_m);assign(c,t,"matrix",local_m);matrix(c,property(c,t,"matrix"),local);double world[]={2,0.75,1,3,18,35};matrix(c,property(c,t,"concatenatedMatrix"),world);
    set(c,local_m,"tx",100);matrix(c,property(c,t,"matrix"),local);
    FVM_Object *snapshot=property(c,t,"matrix");set(c,snapshot,"tx",99);matrix(c,property(c,t,"matrix"),local);
    double parent_color[]={0.5,0.25,1,0.5,10,20,30,40},local_color[]={1,0.5,0.25,1.5,2,4,6,-100};
    FVM_Object *pc=construct(c,"flash.geom.ColorTransform",8,parent_color),*lc=construct(c,"flash.geom.ColorTransform",8,local_color);
    assign(c,parent_t,"colorTransform",pc);assign(c,t,"colorTransform",lc);color(c,property(c,t,"colorTransform"),local_color);near(num(c,child,"_alpha"),150);
    double global_color[]={0.5,0.125,0.25,0.75,11,21,36,-10};color(c,property(c,t,"concatenatedColorTransform"),global_color);
    set(c,lc,"redOffset",100);color(c,property(c,t,"colorTransform"),local_color);
    set(c,property(c,t,"colorTransform"),"greenOffset",99);color(c,property(c,t,"colorTransform"),local_color);
    FVM_Value child_fill[2],child_result;fvm_value_number(child_fill,0x4080c0);fvm_value_number(child_fill+1,100);assert(fvm_object_call(c,child,"beginFill",2,child_fill,&child_result)==FVM_OK);geometry(c,child);FVM_Object *bounds=property(c,t,"pixelBounds");near(num(c,bounds,"x"),18);near(num(c,bounds,"y"),35);near(num(c,bounds,"width"),48);near(num(c,bounds,"height"),60);
    FVM_Value args[1],out;ref(args,child);FVM_Object *explicit_t=0;assert(fvm_construct(c,"flash.geom.Transform",1,args,&explicit_t)==FVM_OK);assert(fvm_gc_pin(c,explicit_t)==FVM_OK);matrix(c,property(c,explicit_t,"matrix"),local);
    FVM_Object *unbound=construct(c,"flash.geom.Transform",0,0);assert(fvm_object_get(c,unbound,"matrix",&out)==FVM_OK && out.type==FVM_VALUE_UNDEFINED);assign(c,unbound,"matrix",parent_m);
    assert(fvm_gc_unpin(c,root)==FVM_OK);assert(fvm_gc_unpin(c,parent_t)==FVM_OK);assert(fvm_gc_unpin(c,explicit_t)==FVM_OK);assert(fvm_gc_collect(c,0)==FVM_OK);matrix(c,property(c,t,"concatenatedMatrix"),world);
    FVM_Value attempt;ref(&attempt,parent_m);assert(fvm_object_set(c,t,"concatenatedMatrix",&attempt)==FVM_OK);matrix(c,property(c,t,"concatenatedMatrix"),world);
    FVM_Object *bundle=0;assert(fvm_object_create(c,&bundle)==FVM_OK);assert(fvm_gc_pin(c,bundle)==FVM_OK);assign(c,bundle,"clip",child);assign(c,bundle,"transform",t);
    FVM_Channel *ch=0;FVM_Value packet;ref(&packet,bundle);assert(fvm_channel_create(&ch)==FVM_OK);assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&out,0)==FVM_OK);assert(fvm_gc_pin(other,out.object)==FVM_OK);fvm_channel_destroy(ch);restored(other,out.object);
    FVM_Result opened=fvm_channel_open_file("transform-test.bin","w",&ch);if(opened==FVM_OK){assert(fvm_channel_send_value(c,ch,&packet,0)==FVM_OK);fvm_channel_destroy(ch);assert(fvm_channel_open_file("transform-test.bin","r",&ch)==FVM_OK);assert(fvm_channel_receive_value(other,ch,&out,0)==FVM_OK);assert(fvm_gc_pin(other,out.object)==FVM_OK);fvm_channel_destroy(ch);restored(other,out.object);assert(std::remove("transform-test.bin")==0);}else assert(opened==FVM_UNSUPPORTED);
    unsigned char *blob=0;size_t size=0;FVM_Value only_t;ref(&only_t,t);assert(fvm_value_serialize(c,&only_t,&blob,&size)==FVM_OK);size_t bad=size;
    for(size_t i=0;i+4<=size;++i)if(std::memcmp(blob+i,"clip",4)==0){bad=i+3;break;}assert(bad<size);blob[bad]='X';assert(fvm_value_deserialize(other,blob,size,&out)==FVM_INVALID_ARGUMENT);assert(out.type==FVM_VALUE_UNDEFINED);fvm_serialized_free(blob);
#ifdef OS_PORTS
    FVM_ResourceBroker *broker=0;FVM_Channel *queue=0,*network=0;FVM_Object *wrapped=0;char *uri=0;assert(fvm_broker_start("127.0.0.1","127.0.0.1",0,&broker)==FVM_OK);assert(fvm_context_set_broker(c,broker)==FVM_OK);assert(fvm_channel_create(&queue)==FVM_OK);assert(fvm_extensions_wrap_channel(c,queue,&wrapped)==FVM_OK);assert(fvm_broker_export(c,wrapped,&uri)==FVM_OK);
    assert(fvm_channel_open_network(other,uri,&network)==FVM_OK);assert(fvm_channel_send_value(c,queue,&packet,0)==FVM_OK);assert(fvm_channel_receive_value(other,network,&out,0)==FVM_OK);assert(fvm_gc_pin(other,out.object)==FVM_OK);restored(other,out.object);
    fvm_channel_release(network);fvm_broker_stop(broker);fvm_channel_release(queue);fvm_broker_release(broker);fvm_serialized_free(uri);
#endif
    FVM_Object *alpha_clip=0;assert(fvm_movieclip_create(c,"alpha",1,&alpha_clip)==FVM_OK);assert(fvm_gc_pin(c,alpha_clip)==FVM_OK);FVM_Value fill[2];fvm_value_number(fill,0x4080c0);fvm_value_number(fill+1,100);assert(fvm_object_call(c,alpha_clip,"beginFill",2,fill,&out)==FVM_OK);geometry(c,alpha_clip);
    double alpha_color[]={1,1,1,1.5,0,0,0,-100};FVM_Object *ac=construct(c,"flash.geom.ColorTransform",8,alpha_color);assign(c,property(c,alpha_clip,"transform"),"colorTransform",ac);
    unsigned char pixels[16*16*4];std::memset(pixels,0,sizeof(pixels));FVM_RasterSurface surface={pixels,16,16,64};FVM_Result rendered=fvm_movieclip_render(c,alpha_clip,&surface);if(rendered==FVM_OK){unsigned char *p=pixels+4*(8*16+8);assert(p[0]==64 && p[1]==128 && p[2]==192 && p[3]==255);}else assert(rendered==FVM_UNSUPPORTED);
    assert(fvm_standard_install(c)==FVM_OK);fvm_context_destroy(other);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
#endif
