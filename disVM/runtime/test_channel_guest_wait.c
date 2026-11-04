/* Hold one real guest SEND in its mover while a second guest sends. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "runtime_gc.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;
static DVM_NativeContext *first_owner; /* Published under gate. */
static Prog *first_process; /* Identity only while its continuation is live. */
static int reached_wait; /* Protected by gate. */
static int moves; /* Updated under NativeWorld. */

Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), WORD);
int __real_dvm_runtime_channel_wait_guest(Channel *);


/* Fail before inspecting data after a broken ownership transition. */
static void check(int condition, const char *reason)
{
    if (!condition)
    {
        fprintf(stderr, "channel guest wait: %s\n", reason);
        exit(10);
    }
}


/* Suspend the first sender with its reservation and rooted channel intact. */
static void rendezvous_mover(void)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    if (++moves == 1)
    {
        int released;
        pthread_mutex_lock(&gate);
        first_owner = owner;
        first_process = dvm_native_context_process(owner);
        check(first_process != NULL, "first sender is a real guest");
        pthread_mutex_unlock(&gate);
        released = dvm_runtime_gc_native_leave();
        check(released, "first sender releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (!reached_wait)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
    }
    movw();
}


/* Instrument only the guest's buffered word channel. */
Channel *__wrap_dvm_runtime_channel_new(void *destination, Type *type, WORD capacity,
    void (*mover)(void), WORD width)
{
    if (type == &Tword && capacity == 2)
    {
        mover = rendezvous_mover;
    }
    return __real_dvm_runtime_channel_new(destination, type, capacity, mover, width);
}


/* Verify that the second guest reaches the production wait on another owner. */
int __wrap_dvm_runtime_channel_wait_guest(Channel *channel)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;
    pthread_mutex_lock(&gate);
    check(first_owner && owner != first_owner && process && process != first_process &&
        !reached_wait, "second sender owns a different guest and native context");
    reached_wait = 1;
    pthread_cond_signal(&waiting);
    pthread_mutex_unlock(&gate);
    result = __real_dvm_runtime_channel_wait_guest(channel);
    check(result && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process, "same continuation resumes");
    puts("PASS foreign channel guest wait");
    return result;
}
