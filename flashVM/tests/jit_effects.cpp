/* Time, random state, trace and thrown values agree across all three execution modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include <cmath>
static unsigned traces,conversions;
static bool raising;
static FVM_Result sink_status;
static FVM_Result effect_sink(FVM_Context *c,FVM_Object *receiver,unsigned argc,const FVM_Value *argv,FVM_Value *,void *)
{
    ++traces;assert(receiver==c->global && argc==1 && argv[0].type==FVM_VALUE_STRING);
    assert(fvm_value_string_size(argv)==3 && !std::memcmp(argv[0].string,"a\0b",3));
    return sink_status;
}
static FVM_Result effect_convert(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *raw)
{
    ++conversions;FVM_Value empty;fvm_value_undefined(&empty);
    assert(fvm_set_global(c,"input",&empty)==FVM_OK && fvm_gc_collect(c,0)==FVM_OK);
    FVM_Value marker;assert(fvm_object_get(c,frame->receiver,"alive",&marker)==FVM_OK && marker.number==17);
    if(raw)assert(fvm_value_string_bytes(out,"a\0b",3)==FVM_OK);else fvm_value_number(out,10);
    return raising?fvm_exception_raise(c,out):FVM_OK;
}
static void effect_object(FVM_Context *c,bool text)
{
    const unsigned char empty[]={0};FVM_Object *object=0,*fn=0;FVM_Value value;
    assert(fvm_object_create(c,&object)==FVM_OK);fvm_value_number(&value,17);
    assert(fvm_object_set(c,object,"alive",&value)==FVM_OK);
    assert(fvm_function_create_avm1(c,empty,sizeof(empty),0,0,0,&fn)==FVM_OK);
    assert(fvm_function_set_body(c,fn,effect_convert,text?(void *)1:0,0)==FVM_OK);
    value=class_value(fn);assert(fvm_object_set(c,object,text?"toString":"valueOf",&value)==FVM_OK);
    value=class_value(object);assert(fvm_set_global(c,"input",&value)==FVM_OK);
}
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
        Program random;random.number(10);random.op(0x30);random.finish();FVM_Value out=class_run(c,random);
        assert(out.type==FVM_VALUE_NUMBER && out.number==8 && c->random_state==1015568748UL);
        Program zero;zero.number(0);zero.op(0x30);zero.finish();out=class_run(c,zero);
        assert(out.number==0 && c->random_state==1015568748UL);
        effect_object(c,false);Program bound;bound.variable("input");bound.op(0x30);bound.finish();
        conversions=0;raising=false;out=class_run(c,bound);assert(conversions==1 && out.number==7 && c->random_state==1586005467UL);
        effect_object(c,false);conversions=0;raising=true;class_run(c,bound,FVM_EXCEPTION);
        assert(conversions==1 && c->random_state==1586005467UL);raising=false;
        Program time;time.op(0x34);time.finish();double previous=0;
        for(unsigned i=0;i<3;++i){out=class_run(c,time);assert(out.type==FVM_VALUE_NUMBER && out.number>=previous && out.number==std::floor(out.number));previous=out.number;}
        Program trace;trace.variable("input");trace.op(0x26);trace.number(42);trace.finish();
        for(unsigned which=0;which<4;++which)
        {
            effect_object(c,true);conversions=traces=0;raising=which==3;sink_status=which==2?FVM_ERROR:FVM_OK;
            assert(fvm_avm1_set_trace_rooted(c,which?effect_sink:0,0,0)==FVM_OK);
            out=class_run(c,trace,raising?FVM_EXCEPTION:sink_status);
            assert(conversions==1 && traces==((which && !raising)?1U:0U));
            if(!which || which==1)assert(out.number==42);
        }
        raising=false;assert(fvm_avm1_set_trace_rooted(c,0,0,0)==FVM_OK);
        Program thrown;thrown.text("retained exception");thrown.op(0x2a);thrown.number(99);thrown.finish();
        /* This helper clears the exception only after checking status and native unwinding. */
        class_run(c,thrown,FVM_EXCEPTION);
        const unsigned char missing[]={0x30,0x26,0x2a};
        for(unsigned i=0;i<3;++i){Program p;p.op(missing[i]);p.number(42);p.finish();class_run(c,p,version?(i==2?FVM_EXCEPTION:FVM_OK):FVM_ERROR);}
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);return 0;
}
