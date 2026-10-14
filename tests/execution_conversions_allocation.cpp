#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Value value,out;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    fvm_value_number(&value,17);fvm_value_number(&out,99);FVM_GCRootFrame *roots=c->gc_frames;allowance=0;
    assert(fvm_exec_to_number(c,0,&value,&out)==FVM_OUT_OF_MEMORY);allowance=-1;
    assert(out.number==99 && c->gc_frames==roots && fvm_exec_to_number(c,0,&value,&out)==FVM_OK && out.number==17);
    allowance=0;assert(fvm_exec_type_of(c,&value,&out)==FVM_OK && out.type==FVM_VALUE_STRING);
    assert(fvm_exec_to_boolean(c,&value,&out)==FVM_OK && out.boolean);allowance=-1;
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
