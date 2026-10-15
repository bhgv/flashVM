/* Invalid/unsupported native inputs preserve handles and never replace an interpreted function. */
#include "flashvm/jit.h"
#include <assert.h>
int main(void)
{
 const unsigned char good[]={0};const unsigned char unsupported[]={0x26,0};
 const unsigned char short_record[]={0x96,5,0,7};const unsigned char bad_string[]={0x96,2,0,0,'x'};
 const unsigned char bad_target[]={0x99,2,0,1,0,0x96,5,0,7,1,0,0,0,0};
 const unsigned char bad_branch[]={0x99,1,0,0,0};FVM_JitCode *code=0,*keep;
 assert(fvm_jit_compile(0,1,&code)==FVM_INVALID_ARGUMENT && !code && fvm_jit_compile(good,0,&code)==FVM_INVALID_ARGUMENT && !code && fvm_jit_compile(good,1,0)==FVM_INVALID_ARGUMENT);
 if(!fvm_jit_available())return 0;
 assert(fvm_jit_compile(good,sizeof(good),&code)==FVM_OK);keep=code;
 assert(fvm_jit_compile(unsupported,sizeof(unsupported),&code)==FVM_UNSUPPORTED && code==keep);
 assert(fvm_jit_compile(short_record,sizeof(short_record),&code)==FVM_INVALID_ARGUMENT && code==keep);
 assert(fvm_jit_compile(bad_string,sizeof(bad_string),&code)==FVM_INVALID_ARGUMENT && code==keep);
 assert(fvm_jit_compile(bad_target,sizeof(bad_target),&code)==FVM_INVALID_ARGUMENT && code==keep);
 assert(fvm_jit_compile(bad_branch,sizeof(bad_branch),&code)==FVM_INVALID_ARGUMENT && code==keep);
 assert(fvm_jit_destroy(code)==FVM_OK);return 0;
}
