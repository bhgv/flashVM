#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
 FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0,*child=0;
 assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
 assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
 FVM_Value value,path,name,reference,out;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=child;assert(fvm_object_set(c,scope,"child",&value)==FVM_OK);
 fvm_value_string(&path,"child");fvm_value_string(&name,"victim");fvm_value_string(&reference,"child.victim");fvm_value_number(&value,17);
 for(unsigned mode=0;mode<2;++mode){bool success=false;unsigned failures=0;
 for(long boundary=0;boundary<2000 && !success;++boundary){assert(fvm_object_set(c,child,"victim",&value)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);int present=99;allowance=boundary;
 FVM_Result r=mode?fvm_exec_delete_reference(c,scope,0,0,&reference,&out):fvm_exec_scope_member(c,scope,0,0,&path,&name,&out,&present);allowance=-1;assert(c->gc_frames==roots);
 int own=0;assert(fvm_object_has_own(c,child,"victim",&own)==FVM_OK);
 if(r==FVM_OK){if(mode)assert(out.boolean && !own);else assert(present && out.number==17 && own);success=true;}
 else{assert(r==FVM_OUT_OF_MEMORY && out.number==99 && present==99 && own);++failures;}
 assert(fvm_gc_collect(c,0)==FVM_OK);}
 assert(success && failures>0);}
 fvm_context_destroy(c);fvm_runtime_destroy(rt);return 0;
}
