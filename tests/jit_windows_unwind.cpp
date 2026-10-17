/* Native Windows-only OS lookup/virtual-unwind fixture, including partially executed prologues. */
#include "flashvm/jit.h"
#include <cassert>
#include <cstring>
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__)) && !defined(_M_ARM64EC) && !defined(__arm64ec__)
#include <windows.h>
#endif
int main()
{
#if defined(_WIN32) && (defined(_M_X64) || defined(__x86_64__)) && !defined(_M_ARM64EC) && !defined(__arm64ec__)
 assert(fvm_jit_available() && !std::strcmp(fvm_jit_backend_name(),"windows-x64"));
 const unsigned char actions[]={0};FVM_JitCode *code=0;
 assert(fvm_jit_compile(actions,sizeof(actions),&code)==FVM_OK);
 const void *machine=0;size_t size=0;assert(fvm_jit_machine_code(code,&machine,&size)==FVM_OK && size>13);
 DWORD64 base=0;PRUNTIME_FUNCTION entry=RtlLookupFunctionEntry((DWORD64)(ULONG_PTR)machine+13,&base,0);
 assert(entry && base==(DWORD64)(ULONG_PTR)machine && entry->BeginAddress==0 && entry->EndAddress==size);
 const unsigned offsets[]={0,4,6,10,13};
 for(size_t i=0;i<sizeof(offsets)/sizeof(offsets[0]);++i)
 {
  DWORD64 stack[8];std::memset(stack,0,sizeof(stack));CONTEXT context;std::memset(&context,0,sizeof(context));
  unsigned allocated=offsets[i]>=10?4:0;unsigned pushed=offsets[i]>=6?1:0;
  const DWORD64 saved=0x12345678U,returned=0x23456789U;
  if(pushed)stack[allocated]=saved;stack[allocated+pushed]=returned;
  context.Rip=base+offsets[i];context.Rsp=(DWORD64)(ULONG_PTR)stack;context.R12=pushed?0:saved;
  void *handler=0;DWORD64 establisher=0;
  RtlVirtualUnwind(UNW_FLAG_NHANDLER,base,context.Rip,entry,&context,&handler,&establisher,0);
  assert(context.Rip==returned && context.Rsp==(DWORD64)(ULONG_PTR)(stack+allocated+pushed+1) && context.R12==saved);
 }
 assert(fvm_jit_destroy(code)==FVM_OK);
 assert(!RtlLookupFunctionEntry((DWORD64)(ULONG_PTR)machine+13,&base,0));
#endif
 return 0;
}
