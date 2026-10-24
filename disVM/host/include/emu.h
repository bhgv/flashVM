/* POSIX host declaration adapter. This is not a complete emulator port. */
#ifndef DISVM_HOST_EMU_H
#define DISVM_HOST_EMU_H
#include <fenv.h>

/* Floating state is opaque host-owned storage, not an ARM register image. */
typedef struct FPU FPU;
struct FPU
{
    fenv_t environment;
};
#define KSTACK (32 * 1024)
extern Proc *getup(void);
#define up (getup())
typedef sigjmp_buf osjmpbuf;
#define ossetjmp(buffer) sigsetjmp(buffer, 1)
#endif
