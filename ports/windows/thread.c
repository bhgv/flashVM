/* FlashVM: Portable platform adapter interface or implementation: thread.
 * File: ports/windows/thread.c.
 */

#include "../thread_port.h"
#include <windows.h>
#include <stdlib.h>
/* Define FVM_PortThread struct and its representation. */
struct FVM_PortThread
{
    DWORD id;
    HANDLE thread;
    FVM_PortThreadFn fn;
    void *arg;
};

/* Compute and return thread entry. */
static DWORD WINAPI thread_entry(LPVOID arg)
{
    FVM_PortThread *t = (FVM_PortThread *)arg;
    t->fn(t->arg);
    return 0;
}

/* Compute and return port thread start. */
FVM_PortThread *fvm_port_thread_start(FVM_PortThreadFn fn, void *arg)
{
    FVM_PortThread *t;
    if (!fn)
        return 0;
    t = (FVM_PortThread *)malloc(sizeof(*t));
    if (!t)
        return 0;

    /* Computation / Вычисление и обновление состояния. */
    t->fn = fn;
    t->arg = arg;
    t->thread = CreateThread(0, 0, thread_entry, t, 0, &t->id);
    if (!t->thread)
    {
        free(t);
        return 0;
    }
    return t;
}

/* Join port thread. */
void fvm_port_thread_join(FVM_PortThread *t)
{
    if (t)
    {
        WaitForSingleObject(t->thread, INFINITE);
        CloseHandle(t->thread);
        free(t);
    }
}

/* Test whether port thread current. */
int fvm_port_thread_is_current(FVM_PortThread *t)
{
    return t && GetCurrentThreadId() == t->id;
}

/* Sleep for port thread us. */
void fvm_port_thread_sleep_us(unsigned long us)
{
    while (us)
    {
        unsigned long chunk = us > 1000000UL ? 1000000UL : us;
        Sleep((DWORD)((chunk + 999UL) / 1000UL));
        us -= chunk;
    }
}
