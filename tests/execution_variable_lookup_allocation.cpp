#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*fallback=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_object_create(c,&fallback)==FVM_OK && fvm_gc_pin(c,fallback)==FVM_OK);
 FVM_Value name,value,out;fvm_value_number(&value,17);assert(fvm_object_set(c,scope,"local",&value)==FVM_OK);fvm_value_string(&name,"local");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *owner=scope;int found=99;fvm_value_number(&out,99);allowance=boundary;
 FVM_Result r=fvm_exec_variable_lookup(c,scope,0,0,fallback,1,&name,&out,&owner,&found);allowance=-1;assert(c->gc_frames==roots);
 if(r==FVM_OK){assert(found && out.number==17 && owner==fallback);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.number==99 && owner==scope && found==99);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
