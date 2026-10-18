#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "flashvm/property.h"
#include <cstring>
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
 for(unsigned loaded=0;loaded<2;++loaded){FVM_Context *c=0;FVM_Object *frame=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};if(loaded)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);assert(fvm_standard_install(c)==FVM_OK && fvm_object_create(c,&frame)==FVM_OK && fvm_gc_pin(c,frame)==FVM_OK);
 FVM_Value name,value,out;fvm_value_string(&name,"cell");fvm_value_string(&value,"owned");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){int removed=0;assert(fvm_object_delete(c,frame,"cell",&removed)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
 FVM_Result r=fvm_exec_variable_declare(c,frame,frame,0,0,FVM_EXEC_DECLARE_INITIALIZED,&name,&value);allowance=-1;assert(c->gc_frames==roots);int own=0;assert(fvm_object_has_own(c,frame,"cell",&own)==FVM_OK);
 if(r==FVM_OK){assert(own && fvm_object_get(c,frame,"cell",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"owned"));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && !own);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_context_destroy(c);}
 fvm_runtime_destroy(rt);return 0;
}
