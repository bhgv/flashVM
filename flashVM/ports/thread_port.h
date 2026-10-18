/* FlashVM: Portable platform adapter interface or implementation: thread port.
 * File: ports/thread_port.h.
 */

#ifndef FVM_THREAD_PORT_H
/* Define FVM THREAD PORT H constant or compilation switch. */
#define FVM_THREAD_PORT_H
#ifdef __cplusplus
extern "C"
{
#endif
    typedef void (*FVM_PortThreadFn)(void *);
    typedef struct FVM_PortThread FVM_PortThread;
    /* Start a platform thread running the supplied callback and argument. */
    FVM_PortThread *fvm_port_thread_start(FVM_PortThreadFn fn, void *arg);
    /* Test whether port thread current. */
    int fvm_port_thread_is_current(FVM_PortThread *thread);
    /* Sleep for port thread us. */
    void fvm_port_thread_sleep_us(unsigned long microseconds);
    /* Join port thread. */
    void fvm_port_thread_join(FVM_PortThread *thread);
#ifdef __cplusplus
}
#endif
#endif
