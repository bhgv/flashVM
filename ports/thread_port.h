#ifndef FVM_THREAD_PORT_H
#define FVM_THREAD_PORT_H
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*FVM_PortThreadFn)(void *);
typedef struct FVM_PortThread FVM_PortThread;
FVM_PortThread *fvm_port_thread_start(FVM_PortThreadFn fn, void *arg);
int fvm_port_thread_is_current(FVM_PortThread *thread);
void fvm_port_thread_sleep_us(unsigned long microseconds);
void fvm_port_thread_join(FVM_PortThread *thread);
#ifdef __cplusplus
}
#endif
#endif
