#define main existing_allocation_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_SETTER_COLLECTOR_FIXTURE_ONLY
#include "script_setter_collector.c"
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;FVM_Context *c=0;FVM_Object *o=0,*plain=0,*capture=0,*fn=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_construct(c,"Object",0,0,&o)==FVM_OK && fvm_gc_pin(c,o)==FVM_OK && fvm_construct(c,"Object",0,0,&plain)==FVM_OK && fvm_object_create(c,&capture)==FVM_OK);
    const unsigned char code[]={0};FVM_Value value,out;fvm_value_number(&value,99);assert(fvm_object_set(c,capture,"alive",&value)==FVM_OK);
    assert(fvm_function_create_avm1(c,code,sizeof(code),0,0,0,&fn)==FVM_OK && fvm_function_set_body(c,fn,setter,0,capture)==FVM_OK);
    install(c,o,plain,fn);do_collect=0;fvm_value_string(&value,"fault");bool success=false;unsigned failures=0;
    for(long boundary=0;boundary<2000 && !success;++boundary){
        FVM_GCRootFrame *roots=c->gc_frames;FVM_Object *old_owner=c->calling_setter_owner;const fvm_detail::string *old_name=c->calling_setter_name;
        unsigned native_depth=c->gc_native_depth,script_depth=c->script_depth;
        allowance=boundary;FVM_Result r=fvm_object_set(c,o,"value",&value);allowance=-1;
        assert(c->gc_frames==roots && c->calling_setter_owner==old_owner && c->calling_setter_name==old_name && c->gc_native_depth==native_depth && c->script_depth==script_depth);
        if(r==FVM_OK){success=true;assert(fvm_object_get(c,o,"value",&out)==FVM_OK && out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"fault"));}
        else{assert(r==FVM_OUT_OF_MEMORY);++failures;}
        assert(fvm_gc_collect(c,0)==FVM_OK);
    }
    assert(success && failures>0 && setter_entries>0);fvm_context_destroy(c);fvm_runtime_destroy(rt);
}
