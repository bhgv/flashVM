#include "../thread_port.h"
#include <pthread.h>
#include <stdlib.h>
struct FVM_PortThread { pthread_t thread; FVM_PortThreadFn fn; void *arg; };
static void *thread_entry(void *arg) {
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
    if (pthread_create(&t->thread,0,thread_entry,t)!=0) { free(t); return 0; }
    return t;
}
void fvm_port_thread_join(FVM_PortThread *t) {
    if (t) { pthread_join(t->thread,0); free(t); }
}
