/* Real AVM1 direct/alias calls, interval and Mouse handlers obey redraw eligibility in all modes. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/stage.h"
#include "flashvm/timers.h"
#include "flashvm/mouse.h"
#include "../ports/monotonic_port.h"
static double sample=1000;
extern "C" int fvm_port_monotonic_ms(double *out){*out=sample;return 1;}
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=4;version<=8;++version)
    for(class_mode=0;class_mode<(fvm_jit_available()?3U:1U);++class_mode){
        sample=1000;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        Program direct;direct.number(0);direct.text("updateAfterEvent");direct.op(0x3d);direct.finish();class_run(c,direct,version<5?FVM_UNSUPPORTED:FVM_OK);assert(!c->stage_update_pending && !c->stage_update_allowed);
        if(version<6){fvm_context_destroy(c);continue;}
        FVM_Value alias;assert(fvm_get_global(c,"updateAfterEvent",&alias)==FVM_OK && fvm_set_global(c,"redrawAlias",&alias)==FVM_OK);
        Program cb;cb.number(0);cb.text("redrawAlias");cb.op(0x3d);cb.op(0x17);cb.number(0);cb.text("redrawAlias");cb.op(0x3d);cb.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;assert(fvm_function_create_avm1(c,cb.bytes.begin(),cb.bytes.size(),0,0,0,&fn)==FVM_OK);
        if(class_mode==1){assert(fvm_jit_compile(cb.bytes.begin(),cb.bytes.size(),&code)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);}
        FVM_Value args[]={class_value(fn),class_value(0)},id;fvm_value_number(&args[1],1);
        assert(fvm_call(c,"setInterval",2,args,&id)==FVM_OK && fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        unsigned long before=c->execution_mode.stats.native_entries;sample=1001;unsigned delivered=0;assert(fvm_timers_poll(c,&delivered)==FVM_OK && delivered==1);
        assert(c->execution_mode.stats.native_entries==before+(class_mode!=0));int pending=0;assert(fvm_stage_take_update(c,&pending)==FVM_OK && pending==1 && !c->stage_update_allowed);
        FVM_Value ignored;assert(fvm_call(c,"clearInterval",1,&id,&ignored)==FVM_OK);
        FVM_Object *listener=0;assert(fvm_object_create(c,&listener)==FVM_OK);FVM_Value method=class_value(fn);
        assert(fvm_object_set(c,listener,"onMouseMove",&method)==FVM_OK && fvm_object_set(c,listener,"onMouseWheel",&method)==FVM_OK && fvm_input_add_mouse_listener(c,listener)==FVM_OK);
        before=c->execution_mode.stats.native_entries;assert(fvm_input_dispatch_mouse(c,FVM_MOUSE_EVENT_MOVE,1,2)==FVM_OK);
        assert(c->execution_mode.stats.native_entries==before+(class_mode!=0));assert(fvm_stage_take_update(c,&pending)==FVM_OK && pending==1 && !c->stage_update_allowed);
        assert(fvm_input_dispatch_mouse_wheel(c,1,1,2)==FVM_OK && fvm_stage_take_update(c,&pending)==FVM_OK && !pending);
        if(code){assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);}
        assert(!c->gc_frames && !c->gc_native_depth && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
