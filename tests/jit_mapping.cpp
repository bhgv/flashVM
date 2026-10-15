/* Native code pages execute successfully but reject writes in an isolated child process. */
#include "../src/jit/internal.hpp"
#include <cassert>
#if defined(__linux__) && defined(__x86_64__)
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#endif
int main()
{
 if(!fvm_jit_available())return 0;
#if defined(__linux__) && defined(__x86_64__)
 const unsigned char body[]={0};FVM_JitCode *code=0;
 assert(fvm_jit_compile(body,sizeof(body),&code)==FVM_OK && code->memory && code->bytes>0 && !code->active);
 pid_t child=fork();assert(child>=0);
 if(!child){struct rlimit limit;limit.rlim_cur=limit.rlim_max=0;setrlimit(RLIMIT_CORE,&limit);signal(SIGSEGV,SIG_DFL);volatile unsigned char *memory=(volatile unsigned char *)code->memory;*memory=0;_exit(1);}
 int status=0;assert(waitpid(child,&status,0)==child && WIFSIGNALED(status) && WTERMSIG(status)==SIGSEGV);
 assert(fvm_jit_destroy(code)==FVM_OK);
#endif
 return 0;
}
