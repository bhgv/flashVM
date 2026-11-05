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
#include "managed_owner.h"
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

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t waiting = PTHREAD_COND_INITIALIZER;
static DVM_NativeContext *first_owner; /* Published under gate. */
static DVM_NativeContext *alt_owner; /* ALT continuation identity under gate. */
static Prog *first_process; /* Identity only while its continuation is live. */
static int resume_mover; /* Protected by gate. */
static int pause_at; /* First SEND, or first RECV after two seed SENDs. */
static int sleep_mode; /* Targeted sleep cancellation or fatal-worker shutdown. */
static int verify_sleep_cancel_handoff; /* Inspect queue capacity before releasing NativeWorld. */
static int sleeping_pid; /* Captured before the long sleep releases its guest. */
static int host_cancel_mode; /* Submit before or after the park wake snapshot. */
static int host_cancel_no_idle_yield; /* Observe the one-executor boundary. */
static int host_cancel_on_publish; /* Submit as the last runnable guest parks. */
static int host_cancel_delivery_seen;
static int host_cancel_sent; /* Exactly one fixture submission per process. */
static int child_exit_cancel_mode; /* Hold child I/O while its parent waits twice. */
static int child_exit_host_cancel; /* Queue cancellation from outside NativeWorld. */
static int child_exit_wait_count; /* Published killwait parks, under gate. */
static int parked_cancel_mode; /* Cancel an ordinary scheduler-blocked guest. */
static int parked_count; /* Number of fixture registrations published under gate. */
static int parked_pid; /* Published under gate after the guest parks. */
static int directory_fault_mode; /* Fail managed publication with a second read queued. */
static int directory_fault_strings; /* Fail after one mapped string was published. */
static int directory_native_calls; /* Fixture native reads, under gate. */
static DVM_FileGate *directory_gate; /* Shared directory FIFO, borrowed by the fixture. */
static DVM_NativeContext *directory_fault_owner; /* First read's owner, published under gate. */
static int broken_pipe_mode; /* Drop the external reader immediately before splice. */
static int broken_pipe_splices; /* Actual stdout splice attempts by this guest. */
static int protected_output; /* Force the fixture write to retain NativeWorld. */
static int stdout_alias_mode; /* stdout/stderr refer to the same pipe. */
static DVM_FileGate *aliased_output_gate; /* Borrowed while the first write is paused. */
static int stdout_retry_count; /* Inject two transient failures before the first committed fragment. */
static int stdout_cancel_mode; /* Test cancellation of backpressured output. */
static int stdout_wait_pid; /* Captured before the staged output wait. */
static int stdout_cancelled; /* Later output uses the normal pipe path. */
static int stdin_cancel_mode; /* Exercise a guest blocked in a real pipe poll. */
static int stdin_wait_pid; /* PID captured before the first pipe read. */
static int stdin_readiness_raced; /* Force one stale readiness before the first pipe fragment. */
static int stdin_partial_received; /* One fragment committed before cancellation. */
static int stdin_cancelled; /* Only the first read is cancelled. */
static int file_cancel_mode; /* Enable the shared-descriptor cancellation fixture. */
static int first_file_entered; /* First write owns the cursor, under gate. */
static int file_wait_pid; /* PID of the second write, under gate. */
static int io_refusals; /* Remaining injected native I/O admission failures. */
static int thread_retry; /* Inject one failure at the OS thread port. */
static int growth_armed; /* The current native wait is requesting growth. */
static int growth_attempts; /* Counts the injected refusal and real retry. */
static int pool_refusal; /* A capped pool cannot admit the second native wait. */
static int worker_failure; /* A third guest fails while the channel waiter sleeps. */
static int isolate_waits; /* Two waiters share a reservation. */
static int waiting_count; /* Fixture arrivals, under gate. */
static int target_cancel; /* Cancel the selected guest instead of stopping all waits. */
static int waiting_pid; /* Published by the fixture under gate. */
static int cancel_wait; /* Simulate shutdown after the waiting guest yields. */
static int alternate_peer; /* The spare sender, not the mover, wakes ALT. */
static int alt_wait_entered; /* Gate for that sender, protected by gate. */
static int alt_bypass; /* Expect ALT to complete without waiting. */
static int fail_mover; /* Inject failure after the first sender resumes. */
static int moves; /* Updated under NativeWorld. */

Channel *__real_dvm_runtime_channel_new(void *, Type *, WORD, void (*)(void), WORD);
int __real_dvm_runtime_channel_wait_guest(Channel *);
void __real_xecalt(int block);
int __real_dvm_runtime_channel_wait_change(void);
extern void dvm_runtime_cancel_native_waits(void);
extern int dvm_runtime_cancel_guest_wait(int pid);
void __real_dvm_runtime_channel_waiter_published(void);
int __real_dvm_managed_workers_park(int (*pending)(void));
extern int dvm_runtime_request_guest_cancel(int pid);
int __real_dvm_runtime_prepare_killwait(Prog *caller, int pid);
FVM_PortThread *__real_fvm_port_thread_start(FVM_PortThreadFn fn, void *argument);
const char *__real_dvm_managed_workers_native_wait(void);
int __real_dvm_runtime_guest_sleep(unsigned long milliseconds);
int __real_fvm_port_sync_wait_ms(FVM_PortSync *sync, unsigned long milliseconds);
FVM_Result __real_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size);
int __real_dvm_file_gate_try_release(DVM_FileGate *file_gate, DVM_FileTicket *ticket);
int __real_dvm_file_gate_enter(DVM_FileGate *file_gate, DVM_FileTicket *ticket);
int __real_dvm_runtime_gc_native_try_leave(void);

/* Exercise the production no-release path while a different guest owns I/O. */
int __wrap_dvm_runtime_gc_native_try_leave(void)
{
    return protected_output ? 0 : __real_dvm_runtime_gc_native_try_leave();
}
FVM_Result __real_dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received);
FVM_Result __real_dvm_runtime_write_stdio(int descriptor, const void *bytes,
    size_t capacity, size_t *written, int *released);
ssize_t __real_splice(int source, off64_t *source_offset, int destination,
    off64_t *destination_offset, size_t size, unsigned int flags);


/* Fail before inspecting data after a broken ownership transition. */
static void check(int condition, const char *reason)
{
    if (!condition)
    {
        fprintf(stderr, "channel guest wait: %s\n", reason);
        exit(10);
    }
}


static __thread int inject_native_return; /* The first mover requests a private fault. */
int __real_dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch);


/* Inject only after reacquiring NativeWorld. The saved guest and queue remain
 * untouched for the runtime's refusal check. */
static void corrupt_native_return(void)
{
    static const size_t offsets[] = {
        offsetof(REG, FP), offsetof(REG, SP), offsetof(REG, EX), offsetof(REG, TS),
        offsetof(REG, PC), offsetof(REG, M), offsetof(REG, MP),
        offsetof(REG, s), offsetof(REG, m), offsetof(REG, d), offsetof(REG, xpc),
    };
    const char *selection = getenv("DVM_TEST_NATIVE_RETURN_FIELD");
    unsigned long field;
    void *replacement;
    unsigned char *slot;

    if (!selection)
    {
        return;
    }
    field = strtoul(selection, NULL, 10);
    check(field < sizeof(offsets)/sizeof(offsets[0]), "unknown native return fixture field");
    slot = (unsigned char *)&DVM_R + offsets[field];
    memmove(&replacement, slot, sizeof(replacement));
    replacement = replacement == NULL ? H : NULL;
    memmove(slot, &replacement, sizeof(replacement));
}


int __wrap_dvm_native_owner_progress(DVM_HeapOwner *owner, unsigned long epoch)
{
    int ok = __real_dvm_native_owner_progress(owner, epoch);

    if (ok && inject_native_return)
    {
        inject_native_return = 0;
        corrupt_native_return();
    }
    return ok;
}


static DVM_WorldRoots real_root_visitor; /* Production callback retained through the fixture. */
static DVM_GCWorkers *root_probe_workers; /* Borrowed service for the explicit suspended-root run. */
static int root_probe_reported; /* Written only under the world collector lock. */
DVM_GCWorkers *__real_dvm_gc_workers_create(const DVM_GCWorkersConfig *config);


/* Check that a stopped collector uses the saved native continuation roots.
 * Only the non-root decoded destination field is perturbed, and restored
 * before the collector leaves this callback. */
static void verify_suspended_roots(DVM_NativeContext *context, void *registers,
    size_t size, void *process, void *user)
{
    Prog *guest = process;
    REG *current = registers;

    if (guest && guest == first_process && guest->state == Prelease && size == sizeof(*current))
    {
        REG saved;
        void *original = current->d;

        memmove(&saved, &guest->R, sizeof(saved));
        current->d = saved.d ? NULL : H;
        real_root_visitor(context, registers, size, process, user);
        current->d = original;
        check(!memcmp(&guest->R, &saved, sizeof(saved)),
            "collector overwrote the saved suspended registers");
        if (!root_probe_reported)
        {
            root_probe_reported = 1;
            puts("PASS collector preserves suspended continuation roots");
        }
        return;
    }
    real_root_visitor(context, registers, size, process, user);
}


/* Install the probe only for the explicit collector control run. */
DVM_GCWorkers *__wrap_dvm_gc_workers_create(const DVM_GCWorkersConfig *config)
{
    DVM_GCWorkersConfig probe = *config;
    DVM_GCWorkers *workers;

    if (getenv("DVM_TEST_SUSPENDED_ROOTS") && probe.roots)
    {
        real_root_visitor = probe.roots;
        probe.roots = verify_suspended_roots;
    }
    workers = __real_dvm_gc_workers_create(&probe);
    if (probe.roots == verify_suspended_roots)
    {
        root_probe_workers = workers;
    }
    return workers;
}


/* Suspend the first transfer with its reservation and rooted channel intact. */
static void rendezvous_mover(void)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    if (++moves == pause_at)
    {
        int released;
        uchar *saved_frame = DVM_R.FP;
        void *saved_destination = DVM_R.d;
        pthread_mutex_lock(&gate);
        first_owner = owner;
        first_process = dvm_native_context_process(owner);
        check(first_process != NULL, "first transfer is a real guest");
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        if (dvm_runtime_shared_mode() == 2)
        {
            WORD private_word = 0;
            DVM_RangeAccess access = {0};

            check(dvm_managed_ranges_begin(&private_word, sizeof(private_word), NULL, 0,
                &access) == DVM_CELL_ACQUIRED, "fixture acquires an operand guard");
            check(dvm_runtime_gc_native_try_leave() == -1 &&
                dvm_runqueue_head() == first_process &&
                dvm_native_context_process(owner) == first_process &&
                !dvm_runqueue_owner(first_process),
                "guarded suspension refuses without withdrawing the guest");
            check(dvm_range_access_end(&access), "fixture releases the operand guard");
        }
        released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "first transfer releases NativeWorld");
        if (root_probe_workers)
        {
            check(dvm_gc_workers_request(root_probe_workers, DVM_GC_CYCLES) &&
                dvm_gc_workers_wait_idle(root_probe_workers) && root_probe_reported,
                "collector visits saved roots before the mover can reenter");
        }
        pthread_mutex_lock(&gate);
        while (!resume_mover)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        {
            const char *phase = getenv("DVM_TEST_NATIVE_RETURN_PHASE");

            if (phase && strcmp(phase, "nested-leave") == 0)
            {
                dvm_runtime_gc_native_try_leave();
            }
            if (phase && strcmp(phase, "missing") == 0)
            {
                dvm_runtime_gc_native_enter(0);
            }
            if (phase && strcmp(phase, "negative") == 0)
            {
                dvm_runtime_gc_native_enter(-1);
            }
            if (phase && strcmp(phase, "noncanonical") == 0)
            {
                dvm_runtime_gc_native_enter(2);
            }
            inject_native_return = getenv("DVM_TEST_NATIVE_RETURN_FIELD") != NULL;
            dvm_runtime_gc_native_enter(released);
            if (phase && strcmp(phase, "duplicate") == 0)
            {
                dvm_runtime_gc_native_enter(released);
            }
        }
        check(dvm_native_context_current() == owner &&
            dvm_native_context_process(owner) == first_process &&
            dvm_runqueue_head() == first_process &&
            DVM_R.FP == saved_frame && DVM_R.d == saved_destination,
            "native return preserves the original frame and result address");
        if (target_cancel)
        {
            check(!dvm_runtime_cancel_guest_wait(first_process->pid) &&
                dvm_runtime_cancel_guest_wait(waiting_pid) &&
                dvm_runtime_cancel_guest_wait(waiting_pid), "cancel exactly the active guest wait");
        }
        else if (cancel_wait)
        {
            dvm_runtime_cancel_native_waits();
        }
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
    if (type == &Tword && (capacity == 35 || capacity == 36))
    {
        host_cancel_mode = (int)capacity;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 44 || capacity == 46))
    {
        host_cancel_on_publish = 1;
        host_cancel_no_idle_yield = capacity == 46;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 40 || capacity == 43))
    {
        child_exit_cancel_mode = 1;
        child_exit_host_cancel = capacity == 43;
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 41 || capacity == 42))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "exit observer releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (child_exit_wait_count < capacity - 40)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        if (capacity == 42)
        {
            resume_mover = 1;
            pthread_cond_broadcast(&waiting);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
    }
    if (type == &Tword && capacity == 39)
    {
        parked_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 32 || capacity == 34))
    {
        parked_cancel_mode = capacity == 34 ? 2 : 1;
    }
    if (type == &Tword && (capacity == 33 || capacity == 37))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "cancellation coordinator releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (parked_count < parked_cancel_mode)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        if (capacity == 37)
        {
            dvm_runtime_gc_native_enter(released);
            return __real_dvm_runtime_channel_new(destination, type, capacity, mover, width);
        }
        check(!dvm_runtime_cancel_guest_wait(parked_pid), "native-only caller cannot touch a parked guest");
        dvm_runtime_gc_native_enter(released);
        check(!dvm_runtime_cancel_guest_wait(0) &&
            dvm_runtime_cancel_guest_wait(parked_pid) &&
            !dvm_runtime_cancel_guest_wait(parked_pid),
            "parked waiter cancels exactly once through the scheduler");
        puts("PASS cancelled scheduler channel wait");
    }
    if (type == &Tword && capacity == 31)
    {
        directory_fault_mode = 1;
    }
    if (type == &Tword && (capacity == 28 || capacity == 29))
    {
        broken_pipe_mode = (int)capacity;
    }
    if (type == &Tword && (capacity == 26 || capacity == 27 || capacity == 30))
    {
        stdout_alias_mode = (int)capacity;
    }
    if (type == &Tword && (capacity >= 23 && capacity <= 25))
    {
        stdout_cancel_mode = (int)capacity;
    }
    if (type == &Tword && (capacity >= 20 && capacity <= 22))
    {
        stdin_cancel_mode = (int)capacity;
    }
    if (type == &Tword && capacity == 18)
    {
        file_cancel_mode = 1;
    }
    if (type == &Tword && (capacity == 16 || capacity == 17 || capacity == 38 || capacity == 45))
    {
        sleep_mode = (int)capacity;
    }
    if (type == &Tword && capacity == 15)
    {
        io_refusals = 10;
    }
    if (type == &Tword && (capacity == 6 || capacity == 9 || capacity == 19))
    {
        int released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "spare sender gate releases NativeWorld");
        pthread_mutex_lock(&gate);
        while (capacity == 6 ? !alt_wait_entered :
            (capacity == 19 ? !first_file_entered : !first_owner))
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
        dvm_runtime_gc_native_enter(released);
        if (capacity == 6 && stdout_alias_mode == 27)
        {
            check(dvm_runtime_cancel_guest_wait(stdout_wait_pid), "queued alias write is cancellable");
        }
        if (capacity == 6 && (stdout_cancel_mode == 23 || stdout_cancel_mode == 24))
        {
            check(dvm_runtime_cancel_guest_wait(stdout_wait_pid), "pipe output is still cancellable");
        }
        if (capacity == 6 && (stdin_cancel_mode == 20 || stdin_cancel_mode == 22))
        {
            check(dvm_runtime_cancel_guest_wait(stdin_wait_pid), "pipe poll is still cancellable");
        }
        if (capacity == 6 && file_cancel_mode)
        {
            check(dvm_runtime_cancel_guest_wait(file_wait_pid), "queued file operation can be cancelled");
        }
        if (capacity == 6 && sleep_mode == 16)
        {
            check(dvm_runtime_cancel_guest_wait(sleeping_pid), "targeted sleep is still registered");
        }
    }
    if (type == &Tword && ((capacity >= 2 && capacity <= 8 && capacity != 6) || (capacity >= 10 && capacity <= 14)))
    {
        pause_at = capacity == 3 ? 3 : 1;
        fail_mover = capacity == 4;
        alt_bypass = capacity == 5;
        alternate_peer = capacity == 7;
        pool_refusal = capacity == 13;
        thread_retry = capacity == 14;
        worker_failure = capacity == 12;
        isolate_waits = capacity == 11;
        target_cancel = capacity == 10 || isolate_waits;
        cancel_wait = capacity == 8 || target_cancel;
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

    if (parked_cancel_mode || sleep_mode == 38)
    {
        __real_xecalt(block);
        return;
    }
    pthread_mutex_lock(&gate);
    alt_owner = dvm_native_context_current();
    if (!first_owner)
    {
        pthread_mutex_unlock(&gate);
        released = dvm_runtime_gc_native_try_leave();
        check(released == 1, "ALT gate releases NativeWorld");
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
    pthread_mutex_lock(&gate);
    if (alt_bypass)
    {
        resume_mover = 1;
        pthread_cond_signal(&waiting);
        puts("PASS ALT bypassed the reservation");
    }
    else if (alternate_peer)
    {
        check(alt_wait_entered && !resume_mover, "spare peer wins before transfer completes");
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
    }
    else
    {
        check(resume_mover, "ALT returned without waiting for the active transfer");
    }
    pthread_mutex_unlock(&gate);
}


/* Cancel the first of two waiting guests and check that the other still
 * observes reservation completion. Both publish before the mover can reenter. */
static int wait_with_other_guest(Channel *channel, DVM_NativeContext *owner, Prog *process)
{
    int result, cancelled;

    pthread_mutex_lock(&gate);
    check(process && owner != first_owner && waiting_count < 2, "different guest waits on the reservation");
    if (!waiting_count)
    {
        waiting_pid = process->pid;
    }
    cancelled = waiting_pid == process->pid;
    if (++waiting_count == 2)
    {
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
    }
    pthread_mutex_unlock(&gate);

    result = __real_dvm_runtime_channel_wait_guest(channel);
    check(result == (cancelled ? -1 : 1) && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process, "only the targeted guest is cancelled");
    puts("PASS isolated channel guest wait");
    return result;
}


/* Check the cancelled native write publishes only the committed prefix. */
FVM_Result __wrap_dvm_runtime_write_stdio(int descriptor, const void *bytes,
    size_t capacity, size_t *written, int *released)
{
    FVM_Result result;
    Prog *process;

    if ((stdout_alias_mode == 27 || stdout_alias_mode == 30) && descriptor == 2 && !resume_mover)
    {
        process = dvm_native_context_process(dvm_native_context_current());
        check(process != NULL, "queued alias has a guest owner");
        stdout_wait_pid = process->pid;
        protected_output = stdout_alias_mode == 30;
        result = __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
        protected_output = 0;
        check(result != FVM_OK && !*written && !dvm_runtime_cancel_guest_wait(stdout_wait_pid),
            "queued alias cancellation leaves no bytes or wait record");
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        return result;
    }
    if (!stdout_cancel_mode || stdout_cancelled || descriptor != 1)
    {
        return __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
    }
    process = dvm_native_context_process(dvm_native_context_current());
    check(process != NULL, "stdout wait has a guest owner");
    stdout_wait_pid = process->pid;
    result = __real_dvm_runtime_write_stdio(descriptor, bytes, capacity, written, released);
    check((stdout_cancel_mode == 24 ? stdout_retry_count == 2 && result == FVM_OK && *written == 4096 :
        result != FVM_OK && !*written) && !dvm_runtime_cancel_guest_wait(stdout_wait_pid),
        "pipe cancellation publishes only committed output and retires the wait");
    stdout_cancelled = 1;
    fputs("PASS cancelled stdout wait\n", stderr);
    return result;
}


/* In the partial case, let the canceller run after the first real splice. */
ssize_t __wrap_splice(int source, off64_t *source_offset, int destination,
    off64_t *destination_offset, size_t size, unsigned int flags)
{
    ssize_t transferred;
    int closes_reader = 0;

    if (stdin_cancel_mode == 22 && source == 0 && !stdin_readiness_raced)
    {
        check((flags & SPLICE_F_NONBLOCK) != 0, "stdin consumes advisory readiness without blocking");
        stdin_readiness_raced = 1;
        errno = EAGAIN;
        return -1;
    }
    if (stdout_cancel_mode == 24 && destination == 1 && stdout_retry_count < 2)
    {
        check((flags & SPLICE_F_NONBLOCK) != 0, "stdout retries without blocking");
        stdout_retry_count++;
        errno = stdout_retry_count == 1 ? EINTR : EAGAIN;
        return -1;
    }
    if (broken_pipe_mode && destination == 1)
    {
        broken_pipe_splices++;
        closes_reader = broken_pipe_splices == (broken_pipe_mode == 28 ? 1 : 2);
        if (closes_reader)
        {
            char acknowledgement;
            fputs("READY close stdout reader\n", stderr);
            check(read(0, &acknowledgement, 1) == 1 && acknowledgement == 'x',
                "reader closed before the native splice");
        }
    }
    transferred = __real_splice(source, source_offset, destination,
        destination_offset, size, flags);
    if (closes_reader)
    {
        check(transferred < 0 && errno == EPIPE, "closed pipe reports EPIPE without terminating");
        fputs("PASS closed pipe splice\n", stderr);
        errno = EPIPE;
    }
    if (stdout_alias_mode && destination == 1 && transferred > 0)
    {
        pthread_mutex_lock(&gate);
        if (!first_file_entered)
        {
            first_file_entered = 1;
            pthread_cond_broadcast(&waiting);
            while (!resume_mover)
            {
                pthread_cond_wait(&waiting, &gate);
            }
        }
        pthread_mutex_unlock(&gate);
    }
    if (stdout_cancel_mode == 24 && !stdout_cancelled && destination == 1 && transferred > 0)
    {
        pthread_mutex_lock(&gate);
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    return transferred;
}


FVM_Result __real_fvm_port_directory_read(FVM_PortDirectory *directory,
    FVM_FileInfo **entries, size_t *count);
String *__real_c2string(char *value, int length);


/* Hold the first native batch until a second guest has queued on its cursor. */
FVM_Result __wrap_fvm_port_directory_read(FVM_PortDirectory *directory,
    FVM_FileInfo **entries, size_t *count)
{
    FVM_Result status = __real_fvm_port_directory_read(directory, entries, count);

    if (directory_fault_mode)
    {
        pthread_mutex_lock(&gate);
        directory_native_calls++;
        if (directory_native_calls == 1)
        {
            check(status == FVM_OK && *count, "first native directory batch succeeds");
            first_file_entered = 1;
            pthread_cond_broadcast(&waiting);
            while (!resume_mover)
            {
                pthread_cond_wait(&waiting, &gate);
            }
            directory_fault_owner = dvm_native_context_current();
        }
        pthread_mutex_unlock(&gate);
    }
    return status;
}


/* Fail after the mapped array exists. The original ticket must still precede
 * the competing read, which is already queued. */
String *__wrap_c2string(char *value, int length)
{
    size_t pending;

    if (directory_fault_mode)
    {
        pthread_mutex_lock(&gate);
        if (directory_fault_owner && directory_fault_owner == dvm_native_context_current() &&
            ++directory_fault_strings == 2)
        {
            directory_fault_owner = NULL;
            check(directory_native_calls == 1, "second read cannot overtake publication");
            pthread_mutex_unlock(&gate);
            dvm_file_gate_snapshot(directory_gate, &pending, NULL);
            check(pending == 2, "directory publication retains its original cursor ticket");
            puts("PASS directory publication retains FIFO");
            error("fixture directory publication failure");
        }
        pthread_mutex_unlock(&gate);
    }
    return __real_c2string(value, length);
}


static void submit_host_cancel_batch(int pid);


/* Observe the actual prepared MCALL becoming a scheduler wait. */
int __wrap_dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    int ready = __real_dvm_runtime_prepare_killwait(caller, pid);

    if (child_exit_cancel_mode && !ready)
    {
        pthread_mutex_lock(&gate);
        child_exit_wait_count++;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        if (child_exit_host_cancel && child_exit_wait_count == 1)
        {
            submit_host_cancel_batch(caller->pid);
        }
    }
    return ready;
}


/* Fill the bounded request queue from an unbound OS thread. A duplicate
 * of the real PID must remain accepted even when the queue is full. */
static void *submit_host_cancel(void *argument)
{
    int pid = *(int *)argument;
    int index;

    check(!dvm_runtime_cancel_guest_wait(pid) && dvm_runtime_request_guest_cancel(pid),
        "unbound host submits rather than touching guest state");
    for (index = 0; index < 63; index++)
    {
        check(dvm_runtime_request_guest_cancel(1000 + index), "bounded host request accepted");
    }
    check(dvm_runtime_request_guest_cancel(pid) && !dvm_runtime_request_guest_cancel(2000),
        "host requests coalesce and enforce their bound");
    return NULL;
}


/* Submit on another thread while the controller still owns NativeWorld. */
static void submit_host_cancel_batch(int pid)
{
    pthread_t thread;

    check(pid > 0 && !pthread_create(&thread, NULL, submit_host_cancel, &pid),
        "host cancellation thread starts");
    check(!pthread_join(thread, NULL), "host cancellation thread joins");
    puts("PASS submitted host cancellation batch");
}


static void submit_host_cancel_once(void)
{
    submit_host_cancel_batch(parked_pid);
    host_cancel_sent = 1;
}


int __real_dvm_runqueue_complete_channel_wait(Prog *process, char *cause);


/* Submit duplicates while delivery still owns the waiter's completion claim. */
static void *submit_during_host_cancel(void *argument)
{
    int pid = *(int *)argument;

    check(dvm_runtime_request_guest_cancel(pid) &&
        dvm_runtime_request_guest_cancel(1062) &&
        !dvm_runtime_request_guest_cancel(2000),
        "current and later requests stay visible while the full batch drains");
    return NULL;
}


int __wrap_dvm_runqueue_complete_channel_wait(Prog *process, char *cause)
{
    if ((host_cancel_mode || host_cancel_on_publish) && !host_cancel_delivery_seen &&
        process->state == Precv && process->pid == parked_pid)
    {
        pthread_t thread;
        int pid = process->pid;

        check(!pthread_create(&thread, NULL, submit_during_host_cancel, &pid),
            "concurrent duplicate request thread starts");
        check(!pthread_join(thread, NULL), "concurrent duplicate request thread joins");
        host_cancel_delivery_seen = 1;
        puts("PASS coalesced cancellation during delivery");
    }
    return __real_dvm_runqueue_complete_channel_wait(process, cause);
}


int __real_dvm_runqueue_claim(Prog *process);


/* A ready guest gets its turn while most of the full host queue still waits. */
int __wrap_dvm_runqueue_claim(Prog *process)
{
    if ((host_cancel_mode || host_cancel_on_publish) && host_cancel_delivery_seen == 1 &&
        process->pid == parked_pid)
    {
        int index;

        for (index = 0; index < 8; index++)
        {
            check(dvm_runtime_request_guest_cancel(3000 + index),
                "one bounded slice freed eight queue slots");
        }
        check(!dvm_runtime_request_guest_cancel(4000),
            "ready guest is claimed before a second cancellation slice");
        host_cancel_delivery_seen = 2;
        puts("PASS guest scheduled between cancellation slices");
    }
    return __real_dvm_runqueue_claim(process);
}


int __real_dvm_managed_workers_yield(int idle);


int __wrap_dvm_managed_workers_yield(int idle)
{
    if (host_cancel_no_idle_yield && parked_pid)
    {
        check(!idle && !host_cancel_delivery_seen,
            "controller must not idle before servicing queued cancellation");
        host_cancel_no_idle_yield = 0;
        puts("PASS cancellation pending at scheduler yield");
    }
    return __real_dvm_managed_workers_yield(idle);
}


/* Force a notification after park captured its generation, but before the
 * controller actually waits. Its old pending observation stays false. */
static int submit_after_park_snapshot(void)
{
    submit_host_cancel_once();
    return 0;
}


/* Drive both sides of the scheduler's no-lost-wake handshake. */
int __wrap_dvm_managed_workers_park(int (*pending)(void))
{
    if (host_cancel_mode && !host_cancel_sent)
    {
        if (host_cancel_mode == 36)
        {
            return __real_dvm_managed_workers_park(submit_after_park_snapshot);
        }
        submit_host_cancel_once();
    }
    return __real_dvm_managed_workers_park(pending);
}


/* Observe a genuine SEND, RECV or ALT registration before the coordinator
 * competes for NativeWorld. The publishing guest completes its turn first. */
void __wrap_dvm_runtime_channel_waiter_published(void)
{
    Prog *process;
    int cancel_pid = 0;

    if (parked_cancel_mode)
    {
        process = dvm_native_context_process(dvm_native_context_current());
        pthread_mutex_lock(&gate);
        if (parked_count < parked_cancel_mode && process &&
            (process->state == Psend || process->state == Precv || process->state == Palt))
        {
            if (!parked_count)
            {
                parked_pid = process->pid;
                if (host_cancel_on_publish)
                {
                    cancel_pid = parked_pid;
                }
            }
            parked_count++;
            pthread_cond_broadcast(&waiting);
        }
        pthread_mutex_unlock(&gate);
    }
    __real_dvm_runtime_channel_waiter_published();
    if (cancel_pid)
    {
        submit_host_cancel_batch(cancel_pid);
    }
}


/* Record the PID before entering the production pipe wait, then check
 * that cancellation leaves no bytes and retires the native wait record. */
FVM_Result __wrap_dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received)
{
    FVM_Result result;

    if (!stdin_cancel_mode || stdin_cancelled)
    {
        return __real_dvm_runtime_read_stdin(bytes, capacity, received);
    }
    stdin_wait_pid = ((Prog *)dvm_native_context_process(dvm_native_context_current()))->pid;
    result = __real_dvm_runtime_read_stdin(bytes, capacity, received);
    if (stdin_cancel_mode == 22 && !stdin_partial_received)
    {
        check(stdin_readiness_raced && result == FVM_OK && *received == 1,
            "initial pipe fragment arrives after stale readiness and before cancellation");
        stdin_partial_received = 1;
        return result;
    }
    check(result != FVM_OK && !*received && !dvm_runtime_cancel_guest_wait(stdin_wait_pid),
        "cancelled pipe read consumed no bytes and removed its record");
    stdin_cancelled = 1;
    puts("PASS cancelled stdin poll");
    return result;
}


/* Hold the first real file write until the second leaves the FIFO. */
FVM_Result __wrap_fvm_port_file_write(FVM_PortFile *file, const void *bytes, size_t size)
{
    if ((file_cancel_mode || child_exit_cancel_mode) && size == 1 && *(const char *)bytes == 'a')
    {
        pthread_mutex_lock(&gate);
        first_file_entered = 1;
        pthread_cond_broadcast(&waiting);
        while (!resume_mover)
        {
            pthread_cond_wait(&waiting, &gate);
        }
        pthread_mutex_unlock(&gate);
    }
    return __real_fvm_port_file_write(file, bytes, size);
}


/* Capture the queued guest identity before native release and verify that
 * it returns while the admitted write still owns the descriptor cursor. */
int __wrap_dvm_file_gate_try_release(DVM_FileGate *file_gate, DVM_FileTicket *ticket)
{
    int queued, result;

    pthread_mutex_lock(&gate);
    queued = file_cancel_mode && first_file_entered && !resume_mover && file_gate;
    if (queued)
    {
        Prog *process = dvm_native_context_process(dvm_native_context_current());
        file_wait_pid = process->pid;
    }
    pthread_mutex_unlock(&gate);
    result = __real_dvm_file_gate_try_release(file_gate, ticket);
    if (queued)
    {
        check(result == -2 && !dvm_runtime_cancel_guest_wait(file_wait_pid),
            "cancelled file wait restores its guest and retires its record");
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
        puts("PASS cancelled file admission");
    }
    return result;
}


/* The second guest has released NativeWorld and published its reservation. */
int __wrap_dvm_file_gate_enter(DVM_FileGate *file_gate, DVM_FileTicket *ticket)
{
    pthread_mutex_lock(&gate);
    if (directory_fault_mode && !resume_mover)
    {
        if (!directory_gate)
        {
            directory_gate = file_gate;
        }
        else
        {
            check(first_file_entered && directory_gate == file_gate, "second read queues on the same cursor");
            resume_mover = 1;
            pthread_cond_broadcast(&waiting);
        }
    }
    if (stdout_alias_mode && !resume_mover)
    {
        if (!aliased_output_gate)
        {
            aliased_output_gate = file_gate;
        }
        else
        {
            check(first_file_entered && aliased_output_gate == file_gate,
                "aliased stdout and stderr share the same FIFO");
            if (stdout_alias_mode == 27)
            {
                alt_wait_entered = 1;
            }
            else
            {
                resume_mover = 1;
            }
            pthread_cond_broadcast(&waiting);
        }
    }
    if (file_cancel_mode && first_file_entered && !resume_mover && file_wait_pid)
    {
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
    }
    pthread_mutex_unlock(&gate);
    {
        int admitted = __real_dvm_file_gate_enter(file_gate, ticket);
        if (admitted && stdout_cancel_mode && stdout_cancel_mode != 24 &&
            stdout_wait_pid && !stdout_cancelled)
        {
            pthread_mutex_lock(&gate);
            alt_wait_entered = 1;
            pthread_cond_broadcast(&waiting);
            pthread_mutex_unlock(&gate);
        }
        if (admitted && stdin_cancel_mode && stdin_wait_pid && !stdin_cancelled &&
            (stdin_cancel_mode != 22 || stdin_partial_received))
        {
            pthread_mutex_lock(&gate);
            alt_wait_entered = 1;
            pthread_cond_broadcast(&waiting);
            pthread_mutex_unlock(&gate);
        }
        return admitted;
    }
}


/* Record the guest PID before the timed condition wait. Verify cleanup
 * after targeted cancellation or a worker failure wakes the sleeper. */
int __wrap_dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    Prog *process;
    int result;

    if (!sleep_mode || milliseconds < 10000)
    {
        return __real_dvm_runtime_guest_sleep(milliseconds);
    }
    process = dvm_native_context_process(dvm_native_context_current());
    check(process != NULL, "sleep has a guest owner");
    sleeping_pid = process->pid;
    if (sleep_mode == 45)
    {
        submit_host_cancel_batch(sleeping_pid);
        verify_sleep_cancel_handoff = 1;
    }
    result = __real_dvm_runtime_guest_sleep(milliseconds);
    check(result == -1 && !dvm_runtime_cancel_guest_wait(sleeping_pid),
        "sleep wakes cancelled and removes its wait record");
    if (verify_sleep_cancel_handoff)
    {
        verify_sleep_cancel_handoff = 0;
        check(dvm_runtime_request_guest_cancel(2000) &&
            !dvm_runtime_request_guest_cancel(2001),
            "published sleep consumes its request and releases the full queue slot");
        puts("PASS native cancellation handoff");
    }
    puts("PASS cancelled native sleep");
    return result;
}


/* Release the companion guest only once the sleeper is about to park. */
int __wrap_fvm_port_sync_wait_ms(FVM_PortSync *sync, unsigned long milliseconds)
{
    if (sleep_mode && milliseconds > 10000)
    {
        pthread_mutex_lock(&gate);
        alt_wait_entered = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    return __real_fvm_port_sync_wait_ms(sync, milliseconds);
}


/* Inject a resource refusal at the audited admission boundary before any
 * managed withdrawal. Later operations use the actual scheduler. */
const char *__wrap_dvm_managed_workers_native_wait(void)
{
    check(!verify_sleep_cancel_handoff,
        "cancelled sleep must skip native executor admission");
    if (io_refusals)
    {
        io_refusals--;
        puts("PASS native I/O admission refusal");
        return "fixture executor unavailable";
    }
    return __real_dvm_managed_workers_native_wait();
}


/* Fail the first replacement thread creation, then allow the retry to
 * create a real worker before releasing the paused mover. */
FVM_PortThread *__wrap_fvm_port_thread_start(FVM_PortThreadFn fn, void *argument)
{
    FVM_PortThread *thread;

    if (!growth_armed)
    {
        return __real_fvm_port_thread_start(fn, argument);
    }
    growth_attempts++;
    if (growth_attempts == 1)
    {
        return NULL;
    }
    check(growth_attempts == 2, "exactly one replacement retry");
    thread = __real_fvm_port_thread_start(fn, argument);
    check(thread != NULL, "replacement thread starts after refusal");
    pthread_mutex_lock(&gate);
    resume_mover = 1;
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    return thread;
}


/* Run the same guest wait twice. Refusal must keep the guest attached
 * and remove its native wait record before the next attempt. */
static int wait_with_growth_retry(Channel *channel, DVM_NativeContext *owner, Prog *process)
{
    int expected = growth_attempts ? 1 : -2;
    int result;

    check(process && owner != first_owner, "growth retry belongs to the waiting guest");
    growth_armed = 1;
    result = channel ? __real_dvm_runtime_channel_wait_guest(channel) :
        __real_dvm_runtime_channel_wait_change();
    growth_armed = 0;
    check(result == expected && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process &&
        !dvm_runtime_cancel_guest_wait(process->pid), "growth attempt preserves continuation and ledger");
    puts("PASS channel wait growth attempt");
    return result;
}


/* Verify that the second guest reaches the production wait on another owner. */
int __wrap_dvm_runtime_channel_wait_guest(Channel *channel)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;
    if (isolate_waits)
    {
        return wait_with_other_guest(channel, owner, process);
    }
    if (thread_retry)
    {
        return wait_with_growth_retry(channel, owner, process);
    }
    check(!alt_bypass, "ALT must not wait when another branch can run");
    pthread_mutex_lock(&gate);
    check(first_owner && owner != first_owner && process && process != first_process &&
        !resume_mover, "second transfer owns a different guest and native context");
    pthread_mutex_unlock(&gate);
    check(!kprotect(try_buffer_finalization, channel) &&
        strstr(getup()->env->errstr, "channel buffer transfer already active") &&
        channel->send && channel->recv, "foreign finalizer preserves both queue owners");
    pthread_mutex_lock(&gate);
    if (owner == alt_owner)
    {
        puts("PASS ALT owns the waiting continuation");
    }
    waiting_pid = process->pid;
    if (worker_failure)
    {
        alt_wait_entered = 1;
    }
    else if (!pool_refusal)
    {
        resume_mover = 1;
    }
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    result = __real_dvm_runtime_channel_wait_guest(channel);
    check(result == (pool_refusal ? -2 : (cancel_wait || worker_failure ? -1 : 1)) && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process, "same continuation resumes");
    if (worker_failure || pool_refusal)
    {
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    if (target_cancel || pool_refusal)
    {
        check(!dvm_runtime_cancel_guest_wait(process->pid), "retired wait has no cancellation record");
    }
    puts("PASS foreign channel guest wait");
    return result;
}


/* Inspect both receive registrations under NativeWorld. This fixture owns
 * both guard channels through its frame and uses no unrelated receivers. */
static void check_retry_alt_queues(void)
{
    uchar *guard = (uchar *)DVM_R.s + offsetof(Alt, ac);
    Channel *channel;
    int index;

    for (index = 0; index < 2; index++)
    {
        memmove(&channel, guard + index*sizeof(Altc) + offsetof(Altc, c), sizeof(channel));
        check(channel && channel != H && channel->recv && !channel->recv->prog,
            "ALT retry has no stale receive registration");
    }
}


/* Verify ALT waits without leaving either guard registered, then let the
 * transfer finish before or during entry to the actual epoch wait. */
int __wrap_dvm_runtime_channel_wait_change(void)
{
    DVM_NativeContext *owner = dvm_native_context_current();
    Prog *process = dvm_native_context_process(owner);
    int result;

    if (thread_retry)
    {
        check(owner == alt_owner, "ALT owns the growth retry");
        check_retry_alt_queues();
        result = wait_with_growth_retry(NULL, owner, process);
        check_retry_alt_queues();
        puts("PASS ALT owns the waiting continuation");
        return result;
    }

    pthread_mutex_lock(&gate);
    check(owner == alt_owner && owner != first_owner && process &&
        process != first_process && !resume_mover, "distinct-channel ALT owns the wait");
    waiting_pid = process->pid;
    if (alternate_peer)
    {
        alt_wait_entered = 1;
    }
    else if (!pool_refusal)
    {
        resume_mover = 1;
    }
    pthread_cond_broadcast(&waiting);
    pthread_mutex_unlock(&gate);
    result = __real_dvm_runtime_channel_wait_change();
    check(result == (pool_refusal ? -2 : (cancel_wait ? -1 : 1)) && dvm_native_context_current() == owner &&
        dvm_native_context_process(owner) == process, "distinct-channel ALT resumes");
    if (pool_refusal)
    {
        pthread_mutex_lock(&gate);
        resume_mover = 1;
        pthread_cond_broadcast(&waiting);
        pthread_mutex_unlock(&gate);
    }
    if (target_cancel || pool_refusal)
    {
        check(!dvm_runtime_cancel_guest_wait(process->pid), "retired wait has no cancellation record");
    }
    puts("PASS foreign channel guest wait");
    puts("PASS ALT owns the waiting continuation");
    return result;
}
