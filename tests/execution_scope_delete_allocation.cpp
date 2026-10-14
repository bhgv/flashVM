#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *scope=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_object_create(c,&scope)==FVM_OK && fvm_gc_pin(c,scope)==FVM_OK);
    FVM_Value key,value,out;fvm_value_string(&key,"victim");fvm_value_number(&value,17);
    bool success=false;unsigned failures=0;
    for(long boundary=0;boundary<2000 && !success;++boundary){
        assert(fvm_object_set(c,scope,"victim",&value)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);
        allowance=boundary;FVM_Result r=fvm_exec_scope_delete(c,scope,0,&key,&out);allowance=-1;
        assert(c->gc_frames==roots);int own=0;assert(fvm_object_has_own(c,scope,"victim",&own)==FVM_OK);
        if(r==FVM_OK){assert(out.type==FVM_VALUE_BOOLEAN && out.boolean && !own);success=true;}
        else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99 && own);++failures;}
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success && failures>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
