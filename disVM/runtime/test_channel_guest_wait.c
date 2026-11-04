/* Hold a guest mover while another guest accesses the same channel. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "runtime_gc.h"
#include "raise.h"
#include "kernel.h"
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;
static DVM_NativeContext *first_owner; /* Published under gate. */
static DVM_NativeContext *alt_owner; /* ALT continuation identity under gate. */
static Prog *first_process; /* Identity only while its continuation is live. */
static int reached_wait; /* Protected by gate. */
static int pause_at; /* First SEND, or first RECV after two seed SENDs. */
static int fail_mover; /* Inject failure after the first sender resumes. */
static int moves; /* Updated under NativeWorld. */

Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), WORD);
int __real_dvm_runtime_channel_wait_guest(Channel *);
void __real_xecalt(int block);


/* Fail before inspecting data after a broken ownership transition. */
static void check(int condition, const char *reason)
{
    if (!condition)
    {
        fprintf(stderr, "channel guest wait: %s\n", reason);
        exit(10);
    }
}


/* Suspend the first transfer with its reservation and rooted channel intact. */
static void rendezvous_mover(void)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    if (++moves == pause_at)
    {
        int released;
        pthread_mutex_lock(&gate);
        first_owner = owner;
        first_process = dvm_native_context_process(owner);
        check(first_process != NULL, "first transfer is a real guest");
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        released = dvm_runtime_gc_native_leave();
        check(released, "first transfer releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (!reached_wait)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
        if (fail_mover)
        {
            error("fixture channel mover failure");
        }
    }
    movw();
}


/* Instrument only the guest's buffered word channel. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity,
    void (*mover)(void), WORD width)
{
    if (type == &Tword && capacity >= 2 && capacity <= 4)
    {
        pause_at = capacity == 3 ? 3 : 1;
        fail_mover = capacity == 4;
        mover = rendezvous_mover;
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, width);
}


/* Exercise finalizer admission while the foreign reservation is active. */
static int try_buffer_finalization(void *argument)
{
    extern void freechan(Heap *, int);
    freechan(D2H(argument), 0);
    return 1;
}


/* Start the real ALT only after a different guest owns the reservation.
 * block is the VM opcode's blocking flag. This test-only gate releases
 * NativeWorld while it waits and acquires it before entering ALT. */
void __wrap_xecalt(int block)
{
    int released;

    pthread_mutex_lock(&gate);
    alt_owner = dvm_native_context_current();
    if (!first_owner)
    {
        pthread_mutex_unlock(&gate);
        released = dvm_runtime_gc_native_leave();
        check(released, "ALT gate releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (!first_owner)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
    }
    else
    {
        pthread_mutex_unlock(&gate);
    }
    __real_xecalt(block);
}


/* Verify that the second guest reaches the production wait on another owner. */
int __wrap_dvm_runtime_channel_wait_guest(Channel *channel)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;
    pthread_mutex_lock(&gate);
    check(first_owner && owner != first_owner && process && process != first_process &&
        !reached_wait, "second transfer owns a different guest and native context");
    pthread_mutex_unlock(&gate);
    check(!kprotect(try_buffer_finalization, channel) &&
        strstr(getup()->env->errstr, "channel buffer transfer already active") &&
        channel->send && channel->recv, "foreign finalizer preserves both queue owners");
    pthread_mutex_lock(&gate);
    if (owner == alt_owner)
    {
        puts("PASS ALT owns the waiting continuation");
    }
    reached_wait = 1;
    pthread_cond_signal(&waiting);
    pthread_mutex_unlock(&gate);
    result = __real_dvm_runtime_channel_wait_guest(channel);
    check(result && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process, "same continuation resumes");
    puts("PASS foreign channel guest wait");
    return result;
}
