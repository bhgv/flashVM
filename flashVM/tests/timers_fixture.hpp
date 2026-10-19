/* Shared independent clock and public registration helpers for scheduler regressions. */
#include "../src/core/runtime.hpp"
#include "flashvm/standard.h"
#include "flashvm/timers.h"
#include "flashvm/gc.h"
#include "flashvm/exception.h"
#include "../ports/monotonic_port.h"
#include <cassert>
#include <cstring>
#include <cmath>
static double timer_sample=1000;
static int timer_available=1;
extern "C" int fvm_port_monotonic_ms(double *out)
{ if(!timer_available)return 0;*out=timer_sample;return 1; }
static FVM_Value timer_number(double n)
{ FVM_Value v;fvm_value_number(&v,n);return v; }
static FVM_Value timer_object(FVM_Object *o)
{ FVM_Value v;fvm_value_undefined(&v);v.type=FVM_VALUE_OBJECT;v.object=o;return v; }
static FVM_Context *timer_context(FVM_Runtime *rt,unsigned version=8)
{
    timer_sample=1000;timer_available=1;FVM_Context *c=0;assert(fvm_context_create(rt,&c)==FVM_OK);
    if(version){unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};swf[3]=(unsigned char)version;assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);}
    assert(fvm_standard_install(c)==FVM_OK);return c;
}
static FVM_Value timer_call(FVM_Context *c,const char *name,unsigned n,const FVM_Value *a,FVM_Result expected=FVM_OK,bool extracted=false)
{
    FVM_Value out;
    if(extracted) {FVM_Value fn;assert(fvm_get_global(c,name,&fn)==FVM_OK);assert(fvm_function_invoke(c,fn.object,0,n,a,&out)==expected);}
    else assert(fvm_call(c,name,n,a,&out)==expected);
    if(expected!=FVM_OK)assert(out.type==FVM_VALUE_UNDEFINED);
    return out;
}
static unsigned timer_poll(FVM_Context *c,double sample,FVM_Result expected=FVM_OK)
{
    timer_sample=sample;unsigned count=999;assert(fvm_timers_poll(c,&count)==expected);
    assert(!c->gc_frames && !c->gc_native_depth && !c->script_depth && !c->timers_dispatching && !c->avm1_execution_depth);return count;
}
struct TimerProbe { unsigned calls;FVM_Result status;FVM_Object *receiver;TimerProbe():calls(0),status(FVM_OK),receiver(0){} };
static FVM_Result timer_probe(FVM_Context *,FVM_Object *self,unsigned,const FVM_Value *,FVM_Value *out,void *raw)
{ TimerProbe *p=(TimerProbe *)raw;++p->calls;p->receiver=self;fvm_value_undefined(out);return p->status; }
static FVM_Value timer_function(FVM_Context *c,TimerProbe *p)
{ FVM_Object *fn=0;assert(fvm_function_create_native(c,timer_probe,p,0,&fn)==FVM_OK);return timer_object(fn); }
