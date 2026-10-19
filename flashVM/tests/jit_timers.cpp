/* Real AVM1 registration, timer callbacks and alias cancellation execute through native JIT entries. */
#define FVM_CLASS_FIXTURE_ONLY
#include "jit_classes.cpp"
#include "flashvm/timers.h"
#include "../ports/monotonic_port.h"
static double sample;
extern "C" int fvm_port_monotonic_ms(double *out) { *out=sample;return 1; }
int main()
{
    FVM_Config cfg;FVM_Runtime *rt=0;assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK);
    for(unsigned version=6;version<=8;++version)
    for(class_mode=0;class_mode<(fvm_jit_available()?3U:1U);++class_mode) {
        sample=1000;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK && fvm_standard_install(c)==FVM_OK);
        FVM_Value v;fvm_value_number(&v,0);assert(fvm_set_global(c,"ticks",&v)==FVM_OK);
        Program cb;cb.text("ticks");cb.variable("ticks");cb.number(1);cb.op(0x47);cb.op(0x1d);cb.variable("ticks");cb.finish();
        FVM_Object *fn=0;FVM_JitCode *code=0;assert(fvm_function_create_avm1(c,cb.bytes.begin(),cb.bytes.size(),0,0,0,&fn)==FVM_OK);
        if(class_mode==1) {assert(fvm_jit_compile(cb.bytes.begin(),cb.bytes.size(),&code)==FVM_OK);assert(fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);}
        v=class_value(fn);assert(fvm_set_global(c,"timerCallback",&v)==FVM_OK);
        bool timeout=version==8;const char *name=timeout?"setTimeout":"setInterval";
        assert(fvm_get_global(c,name,&v)==FVM_OK && fvm_set_global(c,"timerAlias",&v)==FVM_OK);
        Program start;start.number(5);start.variable("timerCallback");start.number(2);start.text("timerAlias");start.op(0x3d);start.finish();
        FVM_Value id=class_run(c,start);assert(id.type==FVM_VALUE_NUMBER && id.number==1);
        /* The fixture restores manual policy after registration; re-enable automatic delivery explicitly. */
        assert(fvm_execution_set_mode(c,class_mode==2?FVM_EXECUTION_JIT:FVM_EXECUTION_MANUAL,0)==FVM_OK);
        unsigned delivered=0;sample=1005;unsigned long entries=c->execution_mode.stats.native_entries;
        assert(fvm_timers_poll(c,&delivered)==FVM_OK && delivered==1);
        assert(c->execution_mode.stats.native_entries==entries+(class_mode!=0));assert(fvm_get_global(c,"ticks",&v)==FVM_OK && v.number==1);
        assert(fvm_set_global(c,"timerId",&id)==FVM_OK);
        Program stop;stop.variable("timerId");stop.number(1);stop.text(timeout?"clearTimeout":"clearInterval");stop.op(0x3d);stop.finish();
        assert(class_run(c,stop).type==FVM_VALUE_UNDEFINED);sample=2000;assert(fvm_timers_poll(c,&delivered)==FVM_OK && !delivered && c->scheduled_calls.empty());
        if(code) {assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK);assert(fvm_jit_destroy(code)==FVM_OK);}
        assert(!c->gc_frames && !c->gc_native_depth && !c->timers_dispatching && !c->avm1_execution_depth);fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
