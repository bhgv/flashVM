#include "../src/jit/internal.hpp"
#include "../src/jit/mode_internal.hpp"
#include <cassert>
#include <cstring>
static unsigned hits;
static FVM_Result hit(FVM_Context *c, const FVM_DebugBreakpoint *location, void *)
{
    assert(location->offset == 0 && !std::strcmp(location->function, "work"));
    ++hits;
    assert(fvm_debug_set_breakpoint(c,"work",0,0) == FVM_OK);
    return FVM_OK;
}
int main()
{
    FVM_Config cfg; FVM_Runtime *rt; FVM_Context *c; FVM_Value out;
    const unsigned char body[] = {0x96,5,0,7,42,0,0,0,0x3e,0};
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&rt)==FVM_OK && fvm_context_create(rt,&c)==FVM_OK);
    FVM_ScriptFunction script;script.name="work";script.body.assign(body,body+sizeof(body));c->script_functions["work"]=script;
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK);
    assert(fvm_avm1_call(c,"work",0,0,0,&out)==FVM_OK && out.number==42);
    FVM_ExecutionStats before;before.struct_size=sizeof(before);assert(fvm_execution_get_stats(c,&before)==FVM_OK);
    assert(fvm_debug_set_breakpoint_handler(c,hit,0)==FVM_OK && fvm_debug_set_breakpoint(c,"work",0,1)==FVM_OK);
    assert(fvm_avm1_call(c,"work",0,0,0,&out)==FVM_OK && out.number==42 && hits==1);
    FVM_ExecutionStats after;after.struct_size=sizeof(after);assert(fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries && after.interpreter_entries>before.interpreter_entries);
    assert(fvm_avm1_call(c,"work",0,0,0,&out)==FVM_OK && hits==1);
    assert(fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries+(fvm_jit_available()?1:0));
    /* Explicit attachments are also bypassed, independently of the requested manual mode. */
    FVM_Object *fn;assert(fvm_function_create_avm1(c,body,sizeof(body),0,0,0,&fn)==FVM_OK);
    FVM_JitCode *code=0;
    if(fvm_jit_available())assert(fvm_jit_compile(body,sizeof(body),&code)==FVM_OK && fvm_function_set_body(c,fn,fvm_jit_body,code,0)==FVM_OK);
    assert(fvm_debug_set_breakpoint_handler(c,0,0)==FVM_OK && fvm_debug_set_breakpoint(c,"<function>",0,1)==FVM_OK);
    assert(fvm_execution_set_mode(c,FVM_EXECUTION_MANUAL,0)==FVM_OK && fvm_execution_get_stats(c,&before)==FVM_OK);
    assert(fvm_function_invoke(c,fn,0,0,0,&out)==FVM_OK && out.number==42 && fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries);
    assert(fvm_debug_set_breakpoint(c,"",0,1)==FVM_OK && fvm_execution_set_mode(c,FVM_EXECUTION_JIT,0)==FVM_OK && fvm_avm1_execute(c,body,sizeof(body))==FVM_OK && fvm_execution_get_stats(c,&after)==FVM_OK && after.native_entries==before.native_entries);
    if(code)assert(fvm_function_set_body(c,fn,0,0,0)==FVM_OK && fvm_jit_destroy(code)==FVM_OK);
    assert(fvm_debug_set_breakpoint(c,"",0,0)==FVM_OK && !fvm_debug_has_breakpoints(c,fvm_detail::string()));
    fvm_context_destroy(c);fvm_runtime_destroy(rt);
    return 0;
}
