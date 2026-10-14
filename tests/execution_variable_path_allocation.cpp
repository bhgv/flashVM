#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*child=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
 FVM_Value value,name,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_object_set(c,scope,"child",&value)==FVM_OK);fvm_value_number(&value,17);assert(fvm_object_set(c,child,"member",&value)==FVM_OK);
 fvm_value_string(&name,"child.member");bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);int handled=21,present=22;allowance=boundary;
 FVM_Result r=fvm_exec_variable_path(c,scope,0,0,&name,&out,&handled,&present);allowance=-1;assert(c->gc_frames==roots);
 if(r==FVM_OK){assert(handled && present && out.number==17);success=true;}
 else{assert(r==FVM_OUT_OF_MEMORY && out.number==99 && handled==21 && present==22);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);fvm_value_string(&name,"plain");fvm_value_number(&out,99);int handled=21,present=22;allowance=0;
 assert(fvm_exec_variable_path(c,scope,0,0,&name,&out,&handled,&present)==FVM_OK && !handled && !present && out.number==99);allowance=-1;
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
