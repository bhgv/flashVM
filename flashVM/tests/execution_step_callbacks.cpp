/* Step retains an otherwise unrooted coercion receiver and restores roots on exceptions. */
#define FVM_VALUE_CALLBACKS_FIXTURE_ONLY
#include "execution_value_callbacks.cpp"
#include "../src/core/runtime.hpp"
static unsigned conversions;
static bool throw_step;
static FVM_Result convert_step(FVM_Context *c,const FVM_ScriptFrame *frame,FVM_Value *out,void *)
{
    ++conversions; assert(fvm_gc_collect(c,0)==FVM_OK);
    FVM_Value marker;
    assert(fvm_object_get(c,frame->receiver,"alive",&marker)==FVM_OK && marker.number==17);
    fvm_value_number(out,throw_step?77:41);
    return throw_step?fvm_exception_raise(c,out):FVM_OK;
}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    {
        FVM_Context *c=0;
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        swf[3]=(unsigned char)version;
        assert(fvm_context_create(rt,&c)==FVM_OK && fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        for(unsigned mode=0;mode<2;++mode)
        {
            FVM_Object *o=0;FVM_Value v,out;
            assert(fvm_object_create(c,&o)==FVM_OK);
            fvm_value_number(&v,17);assert(fvm_object_set(c,o,"alive",&v)==FVM_OK);
            attach(c,o,"valueOf",convert_step,0);
            fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
            FVM_GCRootFrame *roots=c->gc_frames;unsigned before=conversions;
            throw_step=mode!=0;fvm_value_number(&out,99);
            FVM_Result r=fvm_exec_step(c,0,&v,&out);
            assert(conversions==before+1 && c->gc_frames==roots);
            if(mode)assert(r==FVM_EXCEPTION && out.number==99 && fvm_exception_clear(c)==FVM_OK);
            else assert(r==FVM_OK && out.number==42);
        }
        throw_step=false;
        FVM_Object *o=0;FVM_Value v,out;
        assert(fvm_object_create(c,&o)==FVM_OK);
        fvm_value_number(&v,17);assert(fvm_object_set(c,o,"alive",&v)==FVM_OK);
        attach(c,o,"valueOf",convert_step,0);
        fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;
        assert(fvm_set_global(c,"step_input",&v)==FVM_OK);
        for(unsigned mode=0;mode<2;++mode)
        {
            Bytes code;text(code,"step_output");variable(code,"step_input");
            code.push_back(mode?0x51:0x50);code.push_back(0x1d);code.push_back(0);
            unsigned before=conversions;
            assert(fvm_avm1_execute(c,code.begin(),code.size())==FVM_OK);
            assert(conversions==before+1 && fvm_get_global(c,"step_output",&out)==FVM_OK);
            assert(out.number==(mode?40:42));
        }
        fvm_context_destroy(c);
    }
    assert(conversions==20);fvm_runtime_destroy(rt);return 0;
}
