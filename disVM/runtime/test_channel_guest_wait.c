/* Copyright (c) 2026 bhgv
 * EN: Channel regression composition and shared fixture state; no production code.
 * RU: Композиция проверок каналов и общее состояние теста; не код production.
 */
#define _GNU_SOURCE
/* Hold a guest mover while another guest accesses the same channel. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "disvm/native_context.h"
#include "disvm/native_owner.h"
#include "disvm/gc_workers.h"
#include "runtime_gc.h"
#include "dispatch_roots.h"
#include "managed_owner.h"
#include "shared_access.h"
#include "runqueue.h"
#include "raise.h"
#include "kernel.h"
#include "thread_port.h"
#include "channel_sync.h"
#include "file_gate.h"
#include "file_port.h"
#include "filesystem_port.h"
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


/* Exercise the production WORD mover from the rendezvous fixture. */
extern void movw(void);


extern int dvm_runtime_channel_post_close(Channel *channel);

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;
static DVM_NativeContext *first_owner;           /* Published under gate. */
static DVM_NativeContext *alt_owner;             /* ALT continuation identity under gate. */
static Prog *first_process;                      /* Identity only while its continuation is live. */
static int resume_mover;                         /* Protected by gate. */
static int pause_at;                             /* First SEND, or first RECV after two seed SENDs. */
static int sleep_mode;                           /* Targeted sleep cancellation or fatal-worker shutdown. */
static int verify_sleep_cancel_handoff;          /* Inspect queue capacity before releasing NativeWorld. */
static int sleeping_pid;                         /* Captured before the long sleep releases its guest. */
static int host_cancel_mode;                     /* Submit before or after the park wake snapshot. */
static int host_cancel_no_idle_yield;            /* Observe the one-executor boundary. */
static int host_cancel_on_publish;               /* Submit as the last runnable guest parks. */
static int host_cancel_delivery_seen;
static int host_cancel_sent;                     /* Exactly one fixture submission per process. */
static int child_exit_cancel_mode;               /* Hold child I/O while its parent waits twice. */
static int child_exit_host_cancel;               /* Queue cancellation from outside NativeWorld. */
static int child_exit_wait_count;                /* Published killwait parks, under gate. */
static int parked_cancel_mode;                   /* Cancel an ordinary scheduler-blocked guest. */
static int parked_count;                         /* Number of fixture registrations published under gate. */
static int second_parked_pid;                    /* Published under gate. */
static int parked_pid;                           /* Published under gate after the guest parks. */
static int directory_fault_mode;                 /* Fail managed publication with a second read queued. */
static int directory_fault_strings;              /* Fail after one mapped string was published. */
static int directory_native_calls;               /* Fixture native reads, under gate. */
static DVM_FileGate *directory_gate;             /* Shared directory FIFO, borrowed by the fixture. */
static DVM_NativeContext *directory_fault_owner; /* First read's owner, published under gate. */
static int broken_pipe_mode;                     /* Drop the external reader immediately before splice. */
static int broken_pipe_splices;                  /* Actual stdout splice attempts by this guest. */
static int protected_output;                     /* Force the fixture write to retain NativeWorld. */
static int stdout_alias_mode;                    /* stdout/stderr refer to the same pipe. */
static DVM_FileGate *aliased_output_gate;        /* Borrowed while the first write is paused. */
static int stdout_retry_count;        /* Inject two transient failures before the first committed fragment. */
static int stdout_cancel_mode;        /* Test cancellation of backpressured output. */
static int stdout_wait_pid;           /* Captured before the staged output wait. */
static int stdout_cancelled;          /* Later output uses the normal pipe path. */
static int stdin_cancel_mode;         /* Exercise a guest blocked in a real pipe poll. */
static int stdin_wait_pid;            /* PID captured before the first pipe read. */
static int stdin_readiness_raced;     /* Force one stale readiness before the first pipe fragment. */
static int stdin_partial_received;    /* One fragment committed before cancellation. */
static int stdin_cancelled;           /* Only the first read is cancelled. */
static int file_cancel_mode;          /* Enable the shared-descriptor cancellation fixture. */
static int first_file_entered;        /* First write owns the cursor, under gate. */
static int file_wait_pid;             /* PID of the second write, under gate. */
static int io_refusals;               /* Remaining injected native I/O admission failures. */
static int thread_retry;              /* Inject one failure at the OS thread port. */
static int growth_armed;              /* The current native wait is requesting growth. */
static int growth_attempts;           /* Counts the injected refusal and real retry. */
static int pool_refusal;              /* A capped pool cannot admit the second native wait. */
static int worker_failure;            /* A third guest fails while the channel waiter sleeps. */
static int isolate_waits;             /* Two waiters share a reservation. */
static int waiting_count;             /* Fixture arrivals, under gate. */
static int target_cancel;             /* Cancel the selected guest instead of stopping all waits. */
static _Thread_local int isolation_cancel_target;
static int isolation_wait_registered; /* Protected by gate. */
static int dedicated_cancel_posted;   /* Protected by gate. */
static int waiting_pid;               /* Published by the fixture under gate. */
static int cancel_wait;               /* Simulate shutdown after the waiting guest yields. */
static int alternate_peer;            /* The spare sender, not the mover, wakes ALT. */
static int alt_wait_entered;          /* Gate for that sender, protected by gate. */
static int alt_bypass;                /* Expect ALT to complete without waiting. */
static int fail_mover;                /* Inject failure after the first sender resumes. */
static int moves;                     /* Updated under NativeWorld. */


Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), WORD);


int __real_dvm_runtime_channel_wait_guest(Channel *);


void __real_xecalt(int block);


int __real_dvm_runtime_channel_wait_change(uint64_t generation);


extern void dvm_runtime_cancel_native_waits(void);


extern int dvm_runtime_cancel_guest_wait(int pid);


void __real_dvm_runtime_channel_waiter_published(void);


int __real_dvm_managed_workers_park(int (*pending)(void));


extern int dvm_runtime_request_guest_cancel(int pid);


int __real_dvm_runtime_prepare_killwait(Prog *caller, int pid);


FVM_PortThread *__real_fvm_port_thread_start(FVM_PortThreadFn fn, void *argument);


int __real_dvm_runtime_guest_sleep(unsigned long milliseconds);


int __real_fvm_port_sync_wait_ms(FVM_PortSync *sync, unsigned long milliseconds);


FVM_Result __real_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size);


int __real_dvm_file_gate_try_release(DVM_FileGate *file_gate, DVM_FileTicket *ticket);


int __real_dvm_file_gate_enter(DVM_FileGate *file_gate, DVM_FileTicket *ticket);


int __real_dvm_runtime_gc_native_try_leave(void);


static void check(int condition, const char *reason);


#include "test_channel_admission.inc"

#include "test_channel_movers.inc"

#include "test_channel_io.inc"

#include "test_channel_cancel.inc"

#include "test_channel_waits.inc"

#include "test_channel_alt.inc"
