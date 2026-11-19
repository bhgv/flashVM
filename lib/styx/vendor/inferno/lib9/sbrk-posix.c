/* Copyright (c) 2026 bhgv — standalone annotations; original component notices continue to apply.
 * EN: original POSIX lib9 sbrk-posix support.
 * RU: исходная POSIX-поддержка lib9 sbrk-posix.
 */
#ifdef __APPLE_CC__ /* look for a better way */
#include "lib9.h"
#undef _POSIX_C_SOURCE
#undef getwd

#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <termios.h>
#include <signal.h>
#include <pwd.h>
#include <sys/resource.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <sched.h>
#include <errno.h>
#include <sys/ucontext.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <mach/mach_init.h>

#include <mach/task.h>

#include <mach/vm_map.h>

__typeof__(sbrk(0)) sbrk(int size)
{
    void *brk;
    kern_return_t err;

    err = vm_allocate((vm_map_t)mach_task_self(), (vm_address_t *)&brk, size, VM_FLAGS_ANYWHERE);
    if (err != KERN_SUCCESS)
    {
        brk = (void *)-1;
    }

    return brk;
}
#else
/* dummy function for everyone else, in case its ar complains about empty files */


/* [standalone-contract:__fakesbrk]
 * EN: __fakesbrk: original POSIX lib9 sbrk-posix support.
 * RU: __fakesbrk: исходная POSIX-поддержка lib9 sbrk-posix.
 * Parameters: none. Параметры: нет.
 * Return: No C return value; output/state/ownership effects are specified in Description.
 * Возврат: Возвращаемого C-значения нет; изменения output/state/владения заданы в описании.
 * Ownership/threading: retain the original contract immediately above and the public header's requirements.
 * Владение/потоки: действуют исходный контракт выше и требования публичного header.
 */
void __fakesbrk(void)
{
}
#endif
