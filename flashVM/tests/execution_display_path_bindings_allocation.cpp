/* Intrinsic display path reads preserve value/presence metadata and roots under allocation failure. */
#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#include "../src/core/runtime.hpp"
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *root=0,*child=0,*scope=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK && fvm_movieclip_create(c,"root",1,&root)==FVM_OK && fvm_gc_pin(c,root)==FVM_OK);FVM_Value args[2],value,key,out;fvm_value_string(args,"child");fvm_value_number(args+1,1);assert(fvm_object_call(c,root,"createEmptyMovieClip",2,args,&out)==FVM_OK);child=out.object;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_exec_with_scope(c,0,&value,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK);
 for(unsigned mode=0;mode<4;++mode){fvm_value_string(&key,mode==2?"child:_root":mode==1?"_parent":"_root");bool success=false;unsigned failures=0;for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *owner=scope;int handled=99,present=99,with_scope=99;fvm_value_number(&out,99);allowance=boundary;FVM_Result r=mode==0?fvm_exec_get_variable(c,0,child,child,0,&key,&out):mode==1?fvm_exec_member(c,0,&value,&key,0,&out):mode==2?fvm_exec_variable_path(c,0,root,root,&key,&out,&handled,&present):fvm_exec_scope_resolve(c,scope,&key,&out,&owner,&present,&with_scope);allowance=-1;assert(c->gc_frames==roots);if(r==FVM_OK){assert(out.type==FVM_VALUE_OBJECT && out.object==root && (mode!=2 || (handled && present)) && (mode!=3 || (owner==child && present && with_scope)));success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99 && owner==scope && handled==99 && present==99 && with_scope==99);++failures;}assert(fvm_gc_collect(c,0)==FVM_OK);}assert(success && failures);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
