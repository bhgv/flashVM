/* Real SWF6–8 dimension SetMember instructions publish geometry before script handlers in all execution modes. */
#define FVM_SCROLL_EVENTS_TRANSFER_FIXTURE_ONLY
#include "textfield_scroll_events_transfer.cpp"
int main()
{
#ifdef FVM_HAS_FONTS
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=6;version<=8;++version)for(unsigned mode=0;mode<3;++mode)
    {
        FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Object *field=scroll_field(c),*listener=script_listener(c);assert(scroll_listen(c,field,listener));FVM_Value v;assert(fvm_object_get(c,listener,"onScroller",&v)==FVM_OK && fvm_object_set(c,field,"onScroller",&v)==FVM_OK);
        fvm_value_number(&v,0);assert(fvm_object_set(c,field,"seen",&v)==FVM_OK);v=class_value(field);assert(fvm_set_global(c,"field",&v)==FVM_OK);
        Program p;p.variable("field");p.text("_width");p.number(60);p.op(0x4f);p.variable("field");p.text("_height");p.number(200);p.op(0x4f);p.number(91);p.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
        assert(fvm_execution_set_mode(c,mode==0?FVM_EXECUTION_INTERPRETER:mode==1?FVM_EXECUTION_MANUAL:FVM_EXECUTION_JIT,0)==FVM_OK);
        if(mode==1)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
        FVM_ExecutionStats before,after;before.struct_size=sizeof(before);after.struct_size=sizeof(after);assert(fvm_execution_get_stats(c,&before)==FVM_OK);
        assert(fvm_function_invoke(c,fn,0,0,0,&v)==FVM_OK && v.type==FVM_VALUE_NUMBER && v.number==91 && fvm_execution_get_stats(c,&after)==FVM_OK);
        assert(mode?after.native_entries>before.native_entries:after.interpreter_entries>before.interpreter_entries && after.native_entries==before.native_entries);
        assert(counted_metric(c,field,"seen")==2 && counted_metric(c,listener,"seen")==2);assert(fvm_object_get(c,listener,"source",&v)==FVM_OK && v.object==field);
        assert(fvm_function_invoke(c,fn,0,0,0,&v)==FVM_OK && counted_metric(c,field,"seen")==2 && counted_metric(c,listener,"seen")==2);
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
        assert(fvm_gc_collect(c,0)==FVM_OK && !c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
#endif
    return 0;
}
