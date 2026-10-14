/* Namespace member/path/delete allocation failures restore caller roots and outputs. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *clip=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&clip)==FVM_OK && fvm_gc_pin(c,clip)==FVM_OK);FVM_Value object,key,value,out;fvm_value_undefined(&object);object.type=FVM_VALUE_OBJECT;object.object=clip;fvm_value_number(&value,17);assert(fvm_set_global(c,"marker",&value)==FVM_OK);
 for(unsigned mode=0;mode<3;++mode){fvm_value_string(&key,mode==1?"_global":"_global:marker");bool success=false;unsigned failures=0;for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;int handled=99,present=99;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=mode==0?fvm_exec_variable_path(c,0,clip,0,&key,&out,&handled,&present):mode==1?fvm_exec_member(c,0,&object,&key,0,&out):fvm_exec_delete_reference(c,0,clip,0,&key,&out);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(mode==0?(handled && present && out.number==17):mode==1?(out.type==FVM_VALUE_OBJECT && out.object==c->global):(out.type==FVM_VALUE_BOOLEAN && out.boolean));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99 && handled==99 && present==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
