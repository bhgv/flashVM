/* Real SWF5--8 Mouse.hide/show under interpreter/manual/automatic JIT; version and coercion guards. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/mouse.h"
static unsigned coerced=0;
static FVM_Result poison(FVM_Context *,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *,void *)
{ ++coerced;return FVM_UNSUPPORTED; }
static FVM_Object *make(FVM_Context *c,const char *name,bool argument,FVM_JitCode **code)
{
    Program p;
    if(argument)p.variable("poison");
    p.number(argument?1:0);p.variable("Mouse");p.text(name);p.op(0x52);p.finish();
    FVM_Object *fn=0;
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK);
    assert(fvm_gc_pin(c,fn)==FVM_OK && fvm_jit_compile(p.bytes.begin(),p.bytes.size(),code)==FVM_OK);
    if(class_mode==1)assert(fvm_function_set_body(c,fn,fvm_jit_body,*code,0)==FVM_OK);
    return fn;
}
static void run(FVM_Context *c,FVM_Object *fn,int expected,int visible)
{
    FVM_Value out;unsigned long entries=c->execution_mode.stats.native_entries;
    assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.type==FVM_VALUE_NUMBER && out.number==expected);
    if(class_mode)assert(c->execution_mode.stats.native_entries>entries);
    int state=-1;assert(fvm_input_get_cursor_visible(c,&state)==FVM_OK && state==visible);
    assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth);
}
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *r=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&r)==FVM_OK);
    for(unsigned version=4;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode) {
        FVM_Context *c=0;assert(fvm_context_create(r,&c)==FVM_OK);
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        if(version<5) {assert(fvm_standard_mouse_install(c)==FVM_UNSUPPORTED);fvm_context_destroy(c);continue;}
        assert(fvm_standard_mouse_install(c)==FVM_OK && fvm_standard_mouse_install(c)==FVM_OK);
        assert(fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        FVM_Object *p=0;assert(fvm_object_create(c,&p)==FVM_OK);
        assert(fvm_object_define_method_rooted(c,p,"valueOf",poison,0,0)==FVM_OK);
        assert(fvm_object_define_method_rooted(c,p,"toString",poison,0,0)==FVM_OK);
        FVM_Value value=class_value(p);assert(fvm_set_global(c,"poison",&value)==FVM_OK);
        FVM_JitCode *codes[3]={0,0,0};
        FVM_Object *hide=make(c,"hide",false,codes),*show=make(c,"show",false,codes+1),*extra=make(c,"hide",true,codes+2);
        run(c,hide,1,0);run(c,hide,0,0);run(c,show,0,1);run(c,show,1,1);run(c,extra,1,0);
        assert(!coerced && fvm_gc_collect(c,0)==FVM_OK);run(c,show,0,1);
        assert(fvm_get_global(c,"Mouse",&value)==FVM_OK);
        FVM_Value out;
        if(version==5)assert(fvm_object_call(c,value.object,"addListener",0,0,&out)==FVM_UNSUPPORTED);
        FVM_Value zero;fvm_value_number(&zero,0);
        assert(fvm_object_set(c,value.object,"hide",&zero)==FVM_OK);
        assert(fvm_standard_mouse_install(c)==FVM_INVALID_ARGUMENT && c->cursor_visible);
        FVM_Object *functions[]={hide,show,extra};
        for(unsigned i=0;i<3;++i) {
            assert(fvm_function_set_body(c,functions[i],0,0,0)==FVM_OK && fvm_jit_destroy(codes[i])==FVM_OK);
        }
        fvm_context_destroy(c);
    }
    FVM_Context *c=0;assert(fvm_context_create(r,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
    assert(fvm_standard_mouse_install(c)==FVM_OK);
    FVM_Value ns,out;assert(fvm_get_global(c,"Mouse",&ns)==FVM_OK);
    assert(fvm_gc_pin(c,ns.object)==FVM_OK);
    int previous;assert(fvm_input_set_cursor_visible(c,0,&previous)==FVM_OK);
    unsigned char old[]={'F','W','S',4,16,0,0,0,8,0,0,12,1,0,0,0};
    assert(fvm_load_memory(c,old,sizeof(old))==FVM_OK);
    assert(fvm_object_call(c,ns.object,"show",0,0,&out)==FVM_UNSUPPORTED && !c->cursor_visible);
    assert(fvm_gc_unpin(c,ns.object)==FVM_OK);
    fvm_context_destroy(c);fvm_runtime_destroy(r);
}
