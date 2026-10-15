/* Allocation failures across every native string-service family restore invocation roots and state. */
#define main string_allocation_fixture_main
#include "execution_services_allocation.cpp"
#undef main
#define FVM_STRING_FIXTURE_ONLY
#include "jit_strings.cpp"
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned profile=0;profile<3;++profile)
    {
        FVM_Context *c=0;FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)(profile==1?4:8);
        assert(fvm_context_create(rt,&c)==FVM_OK);if(profile)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK && fvm_gc_string_mode(c,FVM_GC_STRINGS_TRACED)==FVM_OK);
        Program p;p.text("abcd");p.op(0x14);p.op(0x17);
        p.text("abcd");p.op(0x32);p.op(0x33);p.op(0x17);
        p.text("abcd");p.number(2);p.number(2);p.op(0x15);p.text("b");p.op(0x21);p.op(0x17);
        const unsigned char compare[]={0x13,0x29,0x68};
        for(unsigned i=0;i<3;++i){p.text("abcd");p.text("b");p.op(compare[i]);p.op(0x17);}
        p.text("abcd");p.number(2);p.number(2);p.op(0x35);p.finish();
        assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK);
        assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        bool success=false;unsigned failures=0;
        for(long boundary=0;boundary<4000 && !success;++boundary)
        {
            FVM_GCRootFrame *roots=c->gc_frames;allowance=boundary;
            FVM_Result status=fvm_function_invoke(c,fn,0,0,0,&out);allowance=-1;
            assert(c->gc_frames==roots && !c->script_depth && !c->gc_native_depth && !code->active);
            if(status==FVM_OK){success=true;assert(out.type==FVM_VALUE_STRING && !std::strcmp(out.string,"bc"));}
            else{assert(status==FVM_OUT_OF_MEMORY);++failures;}
            if(!success)assert(fvm_gc_collect(c,0)==FVM_OK);
        }
        FVM_GCRootScope *hold=0;assert(success && failures && fvm_gc_roots_begin(c,&hold)==FVM_OK);
        assert(fvm_gc_roots_add(hold,&out)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK);
        assert(fvm_jit_destroy(code)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK && !std::strcmp(out.string,"bc"));
        assert(fvm_gc_roots_end(hold)==FVM_OK);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
