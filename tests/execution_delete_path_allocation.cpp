#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *o=0,*child=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    assert(fvm_object_create(c,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK && fvm_object_create(c,&child)==FVM_OK);
    FVM_Value base,path,value,out;fvm_value_undefined(&base);base.type=FVM_VALUE_OBJECT;base.object=o;value=base;value.object=child;assert(fvm_object_set(c,o,"child",&value)==FVM_OK);
    fvm_value_number(&value,17);
    for(unsigned variant=0;variant<2;++variant){fvm_value_string(&path,variant?"/child:victim":"child.victim");bool success=false;unsigned failures=0;
    for(long boundary=0;boundary<2000 && !success;++boundary){
        assert(fvm_object_set(c,child,"victim",&value)==FVM_OK);FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);
        allowance=boundary;FVM_Result r=fvm_exec_delete_path(c,&base,&path,&out);allowance=-1;
        assert(c->gc_frames==roots);int own=0;assert(fvm_object_has_own(c,child,"victim",&own)==FVM_OK);
        if(r==FVM_OK){assert(out.type==FVM_VALUE_BOOLEAN && out.boolean && !own);success=true;}
        else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99 && own);++failures;}
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success && failures>0);}
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
