/* Native inheritance/interfaces and membership match interpretation in isolated versioned contexts. */
#define main class_reference_fixture_main
#include "jit_references.cpp"
#undef main
#include "flashvm/exception.h"
static unsigned class_mode;
static FVM_Value class_value(FVM_Object *object)
{FVM_Value value;fvm_value_undefined(&value);value.type=FVM_VALUE_OBJECT;value.object=object;return value;}
static FVM_Object *class_create(FVM_Context *c,const char *name)
{
    const unsigned char empty[]={0};FVM_Object *object=0;
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&object)==FVM_OK && fvm_gc_pin(c,object)==FVM_OK);
    FVM_Value value=class_value(object);assert(fvm_set_global(c,name,&value)==FVM_OK);return object;
}
static FVM_Value class_run(FVM_Context *c,const Program &p,FVM_Result expected=FVM_OK,FVM_Object *receiver=0)
{
    FVM_Object *fn=0;FVM_JitCode *code=0;FVM_Value out;fvm_value_undefined(&out);
    assert(fvm_function_create_avm1(c,p.bytes.begin(),p.bytes.size(),0,0,0,&fn)==FVM_OK && fvm_gc_pin(c,fn)==FVM_OK);
    assert(fvm_jit_compile(p.bytes.begin(),p.bytes.size(),&code)==FVM_OK);
    assert(fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
    if(class_mode==1)assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
    unsigned long entries=c->execution_mode.stats.native_entries;FVM_GCRootFrame *roots=c->gc_frames;
    assert(fvm_function_invoke(c,fn,receiver,0,0,&out)==expected);
    assert(c->execution_mode.stats.native_entries==entries+(class_mode!=0));
    assert(c->gc_frames==roots && !c->gc_native_depth && !c->script_depth && !code->active);
    if(expected==FVM_EXCEPTION)assert(fvm_exception_clear(c)==FVM_OK);
    if(expected==FVM_OK && out.type==FVM_VALUE_OBJECT)assert(fvm_gc_pin(c,out.object)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
    assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);return out;
}
#ifndef FVM_CLASS_FIXTURE_ONLY
int main()
{
    if(!fvm_jit_available())return 0;
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=0;version<=8;++version)for(class_mode=0;class_mode<3;++class_mode)
    {
        if(version && version<4)continue;
        FVM_Context *c=0;unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK);if(version)assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        assert(fvm_standard_install(c)==FVM_OK);
        FVM_Object *base=class_create(c,"Base"),*child=class_create(c,"Child"),*iface=class_create(c,"Iface"),*other=class_create(c,"Other");
        FVM_Value proto,value;assert(fvm_object_get(c,base,"prototype",&proto)==FVM_OK);fvm_value_number(&value,42);
        assert(fvm_object_set(c,proto.object,"marker",&value)==FVM_OK);
        Program extend;extend.variable("Child");extend.variable("Base");extend.op(0x69);extend.number(42);extend.finish();
        value=class_run(c,extend);assert(value.type==FVM_VALUE_NUMBER && value.number==42);
        FVM_Object *item=0;assert(fvm_function_construct(c,child,0,0,&item)==FVM_OK && fvm_gc_pin(c,item)==FVM_OK);
        value=class_value(item);assert(fvm_set_global(c,"item",&value)==FVM_OK);
        assert(fvm_object_get(c,item,"marker",&value)==FVM_OK && value.type==FVM_VALUE_NUMBER && value.number==42);
        Program instance;instance.variable("item");instance.variable("Base");instance.op(0x54);instance.finish();
        value=class_run(c,instance);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean);
        Program cast;cast.variable("Base");cast.variable("item");cast.op(0x2b);cast.finish();
        value=class_run(c,cast);assert(value.type==FVM_VALUE_OBJECT && value.object==item);
        Program failed;failed.variable("Other");failed.variable("item");failed.op(0x2b);failed.finish();
        value=class_run(c,failed);assert(value.type==FVM_VALUE_NULL);
        Program interfaces;interfaces.variable("Iface");interfaces.number(1);interfaces.variable("Child");interfaces.op(0x2c);interfaces.number(42);interfaces.finish();
        value=class_run(c,interfaces);assert(value.type==FVM_VALUE_NUMBER && value.number==42);
        Program membership;membership.variable("item");membership.variable("Iface");membership.op(0x54);membership.finish();
        value=class_run(c,membership);assert(value.type==FVM_VALUE_BOOLEAN && !!value.boolean==(!version || version>=7));
        if(version>=7)
        {
            FVM_Value old;FVM_Object *replacement=0,*alias=class_create(c,"Alias");
            assert(fvm_object_get(c,iface,"prototype",&old)==FVM_OK && fvm_object_create(c,&replacement)==FVM_OK);
            value=class_value(replacement);assert(fvm_object_set(c,iface,"prototype",&value)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
            value=class_run(c,membership);assert(value.type==FVM_VALUE_BOOLEAN && !value.boolean);
            assert(fvm_object_set(c,alias,"prototype",&old)==FVM_OK);
            Program snapshot;snapshot.variable("item");snapshot.variable("Alias");snapshot.op(0x54);snapshot.finish();
            value=class_run(c,snapshot);assert(value.type==FVM_VALUE_BOOLEAN && value.boolean);
        }
        const unsigned char missing[]={0x69,0x2c,0x54,0x2b};
        for(unsigned i=0;i<4;++i)
        {
            Program p;p.op(missing[i]);if(i<2)p.number(42);p.finish();
            value=class_run(c,p,version?FVM_OK:FVM_ERROR);
            if(version){if(i<2)assert(value.type==FVM_VALUE_NUMBER && value.number==42);else if(i==2)assert(value.type==FVM_VALUE_BOOLEAN && !value.boolean);else assert(value.type==FVM_VALUE_NULL);}
        }
        assert(fvm_gc_collect(c,0)==FVM_OK);(void)iface;(void)other;fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
#endif
