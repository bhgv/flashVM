#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*child=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
 FVM_Value value,path,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_object_set(c,scope,"child",&value)==FVM_OK);
 fvm_value_string(&path,"/child");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);allowance=boundary;
 FVM_Result r=fvm_exec_scope_target(c,scope,scope,0,&path,&out);allowance=-1;assert(c->gc_frames==roots);
 if(r==FVM_OK){assert(out.type==FVM_VALUE_OBJECT && out.object==child);success=true;}
 else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
