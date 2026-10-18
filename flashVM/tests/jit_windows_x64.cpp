/* Execute the Microsoft x64 ABI on Linux GCC, without claiming a native Windows VM run. */
#include "../src/jit/internal.hpp"
#include <cassert>
#include <cstring>
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && !defined(__ILP32__)
#define TEST_MS_ABI __attribute__((ms_abi))
struct State { unsigned calls,predicates;unsigned operands[16];bool fail; };
static FVM_Result TEST_MS_ABI helper(State *s,unsigned operand)
{
 assert(s && s->calls<16);s->operands[s->calls++]=operand;
 /* Volatile Microsoft registers can be clobbered by every helper. */
 __asm__ __volatile__("xor %%r8,%%r8\n\txor %%r9,%%r9\n\txor %%r10,%%r10\n\txor %%r11,%%r11" ::: "r8","r9","r10","r11");
 return s->fail && operand==2?FVM_ERROR:FVM_OK;
}
static FVM_Result TEST_MS_ABI predicate(State *s,unsigned operand)
{assert(!operand);return ++s->predicates==2?(FVM_Result)1:FVM_OK;}
/* Verify helper entry alignment and overwrite every caller-owned home slot. */
static FVM_Result TEST_MS_ABI __attribute__((naked)) shadow_probe(State *,unsigned)
{
 __asm__ __volatile__(
  "mov %rsp,%rax\n\tand $15,%eax\n\tcmp $8,%eax\n\tjne 1f\n\t"
  "movabs $0x123456789abcdef0,%rax\n\tmov %rax,8(%rsp)\n\tmov %rax,16(%rsp)\n\t"
  "mov %rax,24(%rsp)\n\tmov %rax,32(%rsp)\n\txor %eax,%eax\n\tret\n\t"
  "1: mov $-1,%eax\n\tret");
}
static bool resolve(FVM_JitStep step,unsigned char *out,void *)
{
 typedef FVM_Result (TEST_MS_ABI *Fn)(State *,unsigned);
 Fn fn=step==fvm_jit_truth?predicate:step==fvm_jit_finish?shadow_probe:helper;
 std::memcpy(out,&fn,sizeof(fn));return true;
}
#endif
int main()
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && !defined(__ILP32__)
 FVM_JitCode code;code.actions.resize(4);
 code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_finish,0));
 code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint,0xffffffffU));
 code.actions[0].calls.push_back(FVM_JitCall(fvm_jit_checkpoint,0x80000000U));
 code.actions[1].calls.push_back(FVM_JitCall(fvm_jit_push,1));
 code.actions[1].branch=FVM_JIT_CONDITIONAL;code.actions[1].target=3;
 code.actions[2].calls.push_back(FVM_JitCall(fvm_jit_push,2));
 code.actions[2].branch=FVM_JIT_JUMP;code.actions[2].target=1;
 code.actions[3].calls.push_back(FVM_JitCall(fvm_jit_push,3));code.actions[3].branch=FVM_JIT_EXIT;
 fvm_detail::vector<unsigned char> bytes;
 assert(fvm_jit_encode(FVM_JIT_TARGET_WINDOWS_X64,code,bytes,resolve,0)==FVM_OK);
 void *memory=0;size_t mapped=0;
 assert(fvm_port_code_create(bytes.begin(),bytes.size(),&memory,&mapped)==FVM_OK);
 typedef FVM_Result (TEST_MS_ABI *Entry)(State *);
 Entry entry=0;std::memcpy(&entry,&memory,sizeof(entry));
 for(unsigned failure=0;failure<2;++failure)
 {
  State state;std::memset(&state,0,sizeof(state));state.fail=failure!=0;
  register size_t preserved __asm__("r12")=(size_t)0x76543210U;
  __asm__ __volatile__("" : "+r"(preserved));
  FVM_Result status=entry(&state);
  __asm__ __volatile__("" : "+r"(preserved));
  assert(status==(failure?FVM_ERROR:FVM_OK) && preserved==(size_t)0x76543210U);
  const unsigned expected[]={0xffffffffU,0x80000000U,1,2,1,3};
  assert(state.calls==(failure?4U:6U));
  for(unsigned i=0;i<state.calls;++i)assert(state.operands[i]==expected[i]);
  assert(state.predicates==(failure?1U:2U));
 }
 fvm_port_code_release(memory,mapped);
 FVM_JitCode far;far.actions.resize(22000);
 far.actions[0].branch=FVM_JIT_JUMP;far.actions[0].target=far.actions.size()-1;
 for(size_t i=1;i+1<far.actions.size();++i){far.actions[i].calls.push_back(FVM_JitCall(fvm_jit_push,99));far.actions[i].calls.push_back(FVM_JitCall(fvm_jit_push,99));}
 far.actions.back().calls.push_back(FVM_JitCall(fvm_jit_push,3));far.actions.back().branch=FVM_JIT_EXIT;
 assert(fvm_jit_encode(FVM_JIT_TARGET_WINDOWS_X64,far,bytes,resolve,0)==FVM_OK && bytes.size()>1048576);
 assert(fvm_port_code_create(bytes.begin(),bytes.size(),&memory,&mapped)==FVM_OK);std::memcpy(&entry,&memory,sizeof(entry));
 State far_state;std::memset(&far_state,0,sizeof(far_state));assert(entry(&far_state)==FVM_OK && far_state.calls==1 && far_state.operands[0]==3);
 fvm_port_code_release(memory,mapped);
 /* Invalid branch destinations preserve an existing output vector. */
 size_t before=bytes.size();unsigned char first=bytes[0];code.actions[1].target=99;
 assert(fvm_jit_encode(FVM_JIT_TARGET_WINDOWS_X64,code,bytes,resolve,0)==FVM_INVALID_ARGUMENT && bytes.size()==before && bytes[0]==first);
#endif
 return 0;
}
