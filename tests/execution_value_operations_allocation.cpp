#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_Value a,b,out;fvm_value_number(&a,1);fvm_value_number(&b,2);
    for(unsigned operation=0;operation<3;++operation){
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<100 && !success;++boundary){
            FVM_GCRootFrame *roots=c->gc_frames;fvm_value_number(&out,99);allowance=boundary;
            FVM_Result r=operation==0 ? fvm_exec_logical(c,FVM_EXEC_LOGICAL_AND,&a,&b,&out) :
                         operation==1 ? fvm_exec_legacy_compare(c,FVM_EXEC_LEGACY_LESS,&a,&b,&out) :
                                        fvm_exec_delete_member(c,&a,&b,&out);
            allowance=-1;assert(c->gc_frames==roots);
            if(r==FVM_OK){assert(out.type==FVM_VALUE_BOOLEAN);success=true;}
            else{assert(r==FVM_OUT_OF_MEMORY && out.type==FVM_VALUE_NUMBER && out.number==99);++failures;}
        }
        assert(success && failures>0);
    }
    unsigned char swf[]={'F','W','S',8,18,0,0,0,8,0,0,12,1,0,0x40,0,0,0};
    assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
    unsigned count=99;FVM_GCRootFrame *roots=c->gc_frames;allowance=0;
    assert(fvm_exec_argument_count(c,&a,1,&count)==FVM_OUT_OF_MEMORY);
    allowance=-1;assert(count==99 && c->gc_frames==roots);
    assert(fvm_exec_argument_count(c,&a,1,&count)==FVM_OK && count==1);
    fvm_value_number(&out,99);allowance=0;
    assert(fvm_exec_to_string(c,&a,&out)==FVM_OUT_OF_MEMORY);
    allowance=-1;assert(out.type==FVM_VALUE_NUMBER && out.number==99 && c->gc_frames==roots);
    assert(fvm_exec_to_string(c,&a,&out)==FVM_OK && out.type==FVM_VALUE_STRING);
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
