#include "../thread_port.h"
#include <windows.h>
#include <stdlib.h>
struct FVM_PortThread { HANDLE thread; FVM_PortThreadFn fn; void *arg; };
static DWORD WINAPI thread_entry(LPVOID arg) {
    FVM_PortThread *t=(FVM_PortThread *)arg;
    t->fn(t->arg);
    return 0;
}
FVM_PortThread *fvm_port_thread_start(FVM_PortThreadFn fn, void *arg) {
    FVM_PortThread *t;
    if (!fn) return 0;
    t=(FVM_PortThread *)malloc(sizeof(*t));
    if (!t) return 0;
    t->fn=fn; t->arg=arg;
    t->thread=CreateThread(0,0,thread_entry,t,0,0);
    if (!t->thread) { free(t); return 0; }
    return t;
}
void fvm_port_thread_join(FVM_PortThread *t) {
    if (t) { WaitForSingleObject(t->thread,INFINITE); CloseHandle(t->thread); free(t); }
}
