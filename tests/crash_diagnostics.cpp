/* VM faults retain mixed-engine source/argument metadata and live debugger views. */
#include "flashvm/crash.h"
#include "flashvm/standard.h"
#include "flashvm/exception.h"
#include "flashvm/gc.h"
#include "../src/avm1/handlers/script_call.hpp"
#include "../src/jit/internal.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

static unsigned callbacks;
static bool forced;
static FVM_Value input;
static FVM_Result fail(FVM_Context *c,FVM_Object *,unsigned,const FVM_Value *,FVM_Value *out,void *)
{
    fvm_value_number(&input,99);
    assert(fvm_gc_collect(c,0)==FVM_OK);
    fvm_value_number(out,123);
    return fvm_exception_raise(c,out);
}
static void paused(FVM_Context *c,FVM_Result result,void *)
{
    ++callbacks;
    assert(result==FVM_EXCEPTION);
    FVM_Value arg;assert(fvm_crash_argument(c,0,0,&arg)==FVM_OK);
    assert(arg.type==FVM_VALUE_NUMBER && arg.number==(forced?42:99));
    assert(fvm_crash_configure(c,FVM_CRASH_NONE,0,0,0)==FVM_INVALID_ARGUMENT);
    assert(fvm_crash_clear(c)==FVM_INVALID_ARGUMENT);
    const char *name;size_t n;bool found=false;
    for(size_t i=0;fvm_crash_local(c,0,i,&name,&n,&arg)==FVM_OK;++i)
        if(n==5 && !std::memcmp(name,"input",5))found=true;
    assert(found);
}
static void registered(FVM_Context *c,FVM_Result result,void *)
{
    ++callbacks;assert(result==FVM_EXCEPTION);
    FVM_Value v;assert(fvm_crash_argument(c,0,0,&v)==FVM_OK && v.number==42);
    assert(fvm_crash_register(c,0,1,&v)==FVM_OK && v.number==99);
    const char *name;size_t n;bool found=false;
    for(size_t i=0;fvm_crash_local(c,0,i,&name,&n,&v)==FVM_OK;++i)
        if(n==5 && !std::memcmp(name,"input",5)){found=true;assert(v.number==99);}
    assert(found==forced);
}
static FVM_Result stopped(FVM_Context *c,const FVM_DebugBreakpoint *,void *)
{
    ++callbacks;size_t count;assert(fvm_debug_frame_count(c,&count)==FVM_OK && count==1);
    FVM_DebugFrame frame;assert(fvm_debug_frame_at(c,0,&frame)==FVM_OK && frame.argc==1 && frame.engine==FVM_CRASH_ENGINE_INTERPRETER);
    assert(frame.forced==(int)forced && frame.preservation_status==FVM_OK);
    FVM_Value value;assert(fvm_crash_argument(c,0,0,&value)==FVM_OK && value.number==42);
    const char *name;size_t n;bool found=false;
    for(size_t i=0;fvm_crash_local(c,0,i,&name,&n,&value)==FVM_OK;++i)
        if(n==5 && !std::memcmp(name,"input",5))found=true;
    assert(found);return FVM_OK;
}
static FVM_Result host_body(FVM_Context *c,const FVM_ScriptFrame *,FVM_Value *out,void *)
{fvm_value_number(out,88);return fvm_exception_raise(c,out);}
static void body(FVM_ScriptFunction &fn,const char *name,const unsigned char *data,size_t n)
{
    fn.name=name;fn.body.assign(data,data+n);fn.parameters.push_back(fvm_detail::string("input"));
    fn.parameter_registers.push_back(0);
}
int main(int argc,char **argv)
{
    const unsigned char call[]={0x96,5,0,7,0,0,0,0,0x96,7,0,0,'f','a','u','l','t',0,0x3d,0x3e,0};
    const unsigned char throwing[]={0x96,7,0,0,'i','n','p','u','t',0,0x1c,0x2a,0};
    const unsigned char outer_body[]={0x96,7,0,0,'i','n','p','u','t',0,0x1c,0x96,5,0,7,1,0,0,0,0x96,7,0,0,'i','n','n','e','r',0,0x3d,0x3e,0};
    FVM_Config config;FVM_Runtime *rt;assert(fvm_config_init(&config)==FVM_OK && fvm_runtime_create(&config,&rt)==FVM_OK);
    const char *path=argc>1?argv[1]:"crash-diagnostics.zip";
    for(unsigned engine=0;engine<2;++engine)for(unsigned detail=0;detail<2;++detail)
    {
        FVM_Context *c;assert(fvm_context_create(rt,&c)==FVM_OK && fvm_standard_install(c)==FVM_OK);
        assert(fvm_execution_set_mode(c,engine?FVM_EXECUTION_JIT:FVM_EXECUTION_INTERPRETER,0)==FVM_OK);
        FVM_Object *native;FVM_Value ref,out;
        assert(fvm_function_create_native(c,fail,0,0,&native)==FVM_OK);native->invocable.collector_entry=true;fvm_value_undefined(&ref);ref.type=FVM_VALUE_OBJECT;ref.object=native;
        assert(fvm_set_global(c,"fault",&ref)==FVM_OK);
        FVM_ScriptFunction caller;body(caller,"caller",call,sizeof(call));
        forced=detail!=0;callbacks=0;
        assert(fvm_crash_set_detail(c,forced?FVM_CRASH_DETAIL_FORCE:FVM_CRASH_DETAIL_NATURAL)==FVM_OK);
        assert(fvm_crash_configure(c,FVM_CRASH_DEBUGGER,0,paused,0)==FVM_OK);
        fvm_value_number(&input,42);FVM_GCRootFrame *roots=c->gc_frames;
        assert(fvm_avm1_call_script(c,caller.name,caller,1,&input,c->global,&out)==FVM_EXCEPTION);
        assert(callbacks==1 && !c->crash.active && c->gc_frames==roots);
        FVM_CrashFrame frame;assert(fvm_crash_frame(c,0,&frame)==FVM_OK);
        assert(frame.forced==(int)forced && frame.jit==(engine && fvm_jit_available()));
        assert(frame.function_size==6 && !std::memcmp(frame.function,"caller",6));
        assert(frame.bytecode_size==sizeof(call) && !std::memcmp(frame.bytecode,call,sizeof(call)));
        assert(fvm_crash_argument(c,0,0,&out)==FVM_NOT_FOUND);
        assert(fvm_exception_get(c,&out)==FVM_OK && out.number==123);
        assert(fvm_exception_clear(c)==FVM_OK && fvm_crash_clear(c)==FVM_OK);
        /* Ordinary breakpoints can inspect live variables without producing crash reports. */
        assert(fvm_crash_configure(c,FVM_CRASH_NONE,0,0,0)==FVM_OK);
        assert(fvm_debug_set_breakpoint(c,"caller",0,1)==FVM_OK && fvm_debug_set_breakpoint_handler(c,stopped,0)==FVM_OK);
        fvm_value_number(&input,42);callbacks=0;
        assert(fvm_avm1_call_script(c,caller.name,caller,1,&input,c->global,&out)==FVM_EXCEPTION && callbacks==1);
        assert(fvm_debug_set_breakpoint(c,"caller",0,0)==FVM_OK && fvm_debug_set_breakpoint_handler(c,0,0)==FVM_OK);
        assert(fvm_exception_clear(c)==FVM_OK);
        /* Real private register parameters are available by name only in explicit FORCE mode. */
        const unsigned char regbody[]={0x96,5,0,7,99,0,0,0,0x87,1,0,1,0x17,0x96,2,0,4,1,0x2a,0};
        unsigned char swf[]={'F','W','S',8,16,0,0,0,8,0,0,12,1,0,0,0};
        assert(fvm_load_memory(c,swf,sizeof(swf))==FVM_OK);
        FVM_ScriptFunction registers;body(registers,"registers",regbody,sizeof(regbody));
        registers.function2=true;registers.register_count=3;registers.parameter_registers[0]=1;
        assert(fvm_crash_configure(c,FVM_CRASH_DEBUGGER,0,registered,0)==FVM_OK);
        fvm_value_number(&input,42);callbacks=0;
        assert(fvm_avm1_call_script(c,registers.name,registers,1,&input,c->global,&out)==FVM_EXCEPTION);
        assert(callbacks==1 && !c->crash.active && c->gc_frames==roots);
        assert(fvm_exception_clear(c)==FVM_OK);
        /* Persistent reports survive generated-code destruction and collector execution. */
        assert(fvm_crash_configure(c,FVM_CRASH_BACKTRACE,0,0,0)==FVM_OK);
        FVM_Object *inner;const char *names[]={"input"};
        assert(fvm_function_create_avm1(c,throwing,sizeof(throwing),1,names,0,&inner)==FVM_OK);
        ref.object=inner;assert(fvm_set_global(c,"inner",&ref)==FVM_OK);
        FVM_ScriptFunction outer;body(outer,"outer",outer_body,sizeof(outer_body));
        assert(fvm_debug_map_action(c,"outer",0,"example.as",17)==FVM_OK);
        fvm_value_number(&input,42);
        assert(fvm_avm1_call_script(c,outer.name,outer,1,&input,c->global,&out)==FVM_EXCEPTION);
        size_t count;FVM_Result status,diagnostic;
        assert(fvm_crash_info(c,&count,&status,&diagnostic)==FVM_OK && count==2 && status==FVM_EXCEPTION && diagnostic==FVM_OK);
        assert(fvm_crash_frame(c,1,&frame)==FVM_OK && frame.function_size==5 && !std::memcmp(frame.function,"outer",5));
        assert(frame.line==17 && frame.source_size==10 && !std::memcmp(frame.source,"example.as",10));
        assert(fvm_exception_clear(c)==FVM_OK);
        assert(fvm_crash_configure(c,FVM_CRASH_NONE,0,0,0)==FVM_OK && fvm_crash_clear(c)==FVM_OK);
        assert(fvm_avm1_call_script(c,outer.name,outer,1,&input,c->global,&out)==FVM_EXCEPTION);
        assert(fvm_crash_info(c,&count,&status,&diagnostic)==FVM_NOT_FOUND && count==0);
        assert(fvm_exception_clear(c)==FVM_OK);
        /* ZIP modes differ only in whether active machine bytes are included. */
        for(unsigned code=0;code<2;++code)
        {
            assert(fvm_crash_configure(c,code?FVM_CRASH_ZIP_CODE:FVM_CRASH_ZIP,path,0,0)==FVM_OK);
            assert(fvm_avm1_call_script(c,outer.name,outer,1,&input,c->global,&out)==FVM_EXCEPTION);
            assert(fvm_crash_info(c,&count,&status,&diagnostic)==FVM_OK && diagnostic==FVM_OK);
            const void *data;size_t size;assert(fvm_crash_data(c,1,&data,&size)==FVM_OK && size>4 && !std::memcmp(data,"PK\003\004",4));
            assert(fvm_crash_frame(c,0,&frame)==FVM_OK && (frame.machine_code_size!=0)==(code && engine && fvm_jit_available()));
            assert(fvm_exception_clear(c)==FVM_OK);
        }
        assert(fvm_gc_collect(c,0)==FVM_OK && fvm_crash_frame(c,0,&frame)==FVM_OK && frame.bytecode_size==sizeof(throwing));
        assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK);
        assert(fvm_crash_configure(c,FVM_CRASH_BACKTRACE,0,0,0)==FVM_OK);
        assert(fvm_function_set_body(c,inner,host_body,0,0)==FVM_OK);
        assert(fvm_function_invoke(c,inner,0,1,&input,&out)==FVM_EXCEPTION);
        FVM_CrashEngine actual;assert(fvm_crash_frame_engine(c,0,&actual)==FVM_OK && actual==FVM_CRASH_ENGINE_NATIVE);
        assert(fvm_function_set_body(c,inner,0,0,0)==FVM_OK && fvm_exception_clear(c)==FVM_OK);
        fvm_context_destroy(c);
    }
    if(argc==1)assert(std::remove(path)==0);
    fvm_runtime_destroy(rt);return 0;
}
