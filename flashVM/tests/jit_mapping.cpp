/* Native code pages execute successfully but reject writes in an isolated child process. */
#include <stdio.h>
#include <stdlib.h>
#include "../src/jit/internal.hpp"

#if defined(__linux__)
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>

#endif


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
#if defined(__linux__)
    const unsigned char body[] = {0};
    FVM_JitCode *code = 0;
    if (!(fvm_jit_compile(body, sizeof(body), &code) == FVM_OK && code->memory && code->bytes > 0 && !code->active))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    pid_t child = fork();
    if (!(child >= 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!child)
    {
        struct rlimit limit;
        limit.rlim_cur = limit.rlim_max = 0;
        setrlimit(RLIMIT_CORE, &limit);
        signal(SIGSEGV, SIG_DFL);
        volatile unsigned char *memory = (volatile unsigned char *)code->memory;
        *memory = 0;
        _exit(1);
    }
    int status = 0;
    if (!(waitpid(child, &status, 0) == child && WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_jit_destroy(code) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
