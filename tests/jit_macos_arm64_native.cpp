/* Native Apple Silicon acceptance only: real MAP_JIT, linked frames and simultaneous bodies. */
#include "../ports/mac/code_platform.h"
#include "../src/jit/internal.hpp"
#include <cassert>
#include <cstring>
#if FVM_MAC_CODE_ARM64 && (defined(__aarch64__) || defined(__arm64__))
struct State
{
    unsigned calls, predicates, operands[8];
    bool fail;
    const unsigned char *memory;
    size_t size;
    void *parent;
};
static FVM_Result __attribute__((noinline)) helper(State *state,unsigned operand)
{
    assert(state && state->calls<8);
    state->operands[state->calls++]=operand;
    void **frame=(void **)__builtin_frame_address(0);
    void **native=(void **)frame[0];
    assert(native && (size_t)native%16==0 && native[0]==state->parent);
    size_t returned=(size_t)__builtin_return_address(0);
    assert(returned>=(size_t)state->memory && returned<(size_t)state->memory+state->size);
    assert((size_t)native[1]<(size_t)state->memory || (size_t)native[1]>=(size_t)state->memory+state->size);
    __asm__ __volatile__("mov x0,#0\n\tmov x1,#0\n\tmov x16,#0\n\tmov x17,#0" ::: "x0","x1","x16","x17","cc");
    return state->fail && operand==2?FVM_ERROR:FVM_OK;
}
static FVM_Result predicate(State *state,unsigned operand)
{assert(!operand);return ++state->predicates==2?(FVM_Result)1:FVM_OK;}
static bool resolve(FVM_JitStep step,unsigned char *out,void *)
{
    typedef FVM_Result (*Helper)(State *,unsigned);
    Helper function=step==fvm_jit_truth?predicate:helper;
    std::memcpy(out,&function,sizeof(function));return true;
}
static FVM_Result __attribute__((noinline)) invoke(void *memory,State *state)
{
    typedef FVM_Result (*Entry)(State *);
    Entry entry=0;std::memcpy(&entry,&memory,sizeof(entry));
    state->parent=__builtin_frame_address(0);
    register size_t saved19 __asm__("x19")=(size_t)0x12345678UL;
    register size_t saved20 __asm__("x20")=(size_t)0x76543210UL;
    __asm__ __volatile__("" : "+r"(saved19),"+r"(saved20));
    FVM_Result result=entry(state);
    __asm__ __volatile__("" : "+r"(saved19),"+r"(saved20));
    assert(saved19==(size_t)0x12345678UL && saved20==(size_t)0x76543210UL);
    return result;
}
static void exercise(void *memory,size_t size)
{
    for(unsigned failure=0;failure<2;++failure)
    {
        State state;std::memset(&state,0,sizeof(state));
        state.fail=failure!=0;state.memory=(const unsigned char *)memory;state.size=size;
        assert(invoke(memory,&state)==(failure?FVM_ERROR:FVM_OK));
        const unsigned expected[]={0xffffffffU,0x80000000U,1,2,1,3};
        assert(state.calls==(failure?4U:6U) && state.predicates==(failure?1U:2U));
        for(unsigned i=0;i<state.calls;++i)assert(state.operands[i]==expected[i]);
    }
}
#endif
int main()
{
#if FVM_MAC_CODE_ARM64 && (defined(__aarch64__) || defined(__arm64__))
    if(!fvm_jit_available())return 77;
    assert(!std::strcmp(fvm_jit_backend_name(),"macos-arm64"));
    FVM_JitCode code;code.actions.resize(4);
    code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint,0xffffffffU));
    code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint,0x80000000U));
    code.actions[1].calls.push_back(FVM_JitCall(fvm_jit_push,1));
    code.actions[1].branch=FVM_JIT_CONDITIONAL;code.actions[1].target=3;
    code.actions[2].calls.push_back(FVM_JitCall(fvm_jit_push,2));
    code.actions[2].branch=FVM_JIT_JUMP;code.actions[2].target=1;
    code.actions[3].calls.push_back(FVM_JitCall(fvm_jit_push,3));code.actions[3].branch=FVM_JIT_EXIT;
    fvm_detail::vector<unsigned char> bytes;
    assert(fvm_jit_encode(FVM_JIT_TARGET_MACOS_ARM64,code,bytes,resolve,0)==FVM_OK);
    void *first=0;size_t first_cookie=0;
    FVM_Result result=fvm_port_code_create(bytes.begin(),bytes.size(),&first,&first_cookie);
    if(result==FVM_UNSUPPORTED)return 77;
    assert(result==FVM_OK && first);
    void *second=0;size_t second_cookie=0;
    assert(fvm_port_code_create(bytes.begin(),bytes.size(),&second,&second_cookie)==FVM_OK && second && second!=first);
    exercise(first,bytes.size());exercise(second,bytes.size());
    fvm_port_code_release(first,first_cookie);
    void *third=0;size_t third_cookie=0;
    assert(fvm_port_code_create(bytes.begin(),bytes.size(),&third,&third_cookie)==FVM_OK);
    exercise(second,bytes.size());exercise(third,bytes.size());
    fvm_port_code_release(second,second_cookie);fvm_port_code_release(third,third_cookie);
#else
    return 77;
#endif
    return 0;
}
