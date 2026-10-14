#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*child=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
 FVM_Value name,value,old,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_object_set(c,scope,"child",&value)==FVM_OK);fvm_value_number(&value,42);fvm_value_number(&old,17);
 for(unsigned mode=0;mode<2;++mode){FVM_Object *destination=mode?child:scope;fvm_value_string(&name,mode?"child.member":"member");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){assert(fvm_object_set(c,destination,"member",&old)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
 FVM_Result r=fvm_exec_variable_set(c,scope,scope,0,&name,&value);allowance=-1;assert(c->gc_frames==roots && fvm_object_get(c,destination,"member",&out)==FVM_OK);
 if(r==FVM_OK){assert(out.number==42);success=true;}else{assert(r==FVM_OUT_OF_MEMORY && out.number==17);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);}
 fvm_value_string(&name,"");allowance=0;assert(fvm_exec_variable_set(c,scope,scope,0,&name,&value)==FVM_OK);allowance=-1;
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
