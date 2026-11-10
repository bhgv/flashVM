/* Retained Dis interpreter/JIT with explicit native execution and GC owners. */
#define _GNU_SOURCE
#include "disvm/refcount.h"
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "pool.h"
#include "raise.h"
#include "exception_host.h"
#include "native_jump.h"
#include "table_leases.h"
#include "owned.h"
#include "disvm/native_process.h"
#include "disvm/native_owner.h"
#include "kernel.h"
#include "vmchannel.h"
#include "file_port.h"
#include "file_gate.h"
#include "filesystem_sys.h"
#include "metadata_wait.h"
#include "open_wait.h"
#include "close_worker.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "root_registry.h"
#include "shared_bytes.h"
#include "shared_arithmetic.h"
#include "shared_pointer.h"
#include "shared_mapped.h"
#include "shared_access.h"
#include "shared_string_branch.h"
#include "runqueue.h"
#include "queue_probe.h"
#include "channel_sync.h"
#include "managed_owner_probe.h"
#include "managed_workers.h"
#include "process_environment.h"
#include "io_buffer.h"
#include "formatted_sys.h"
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/stat.h>
#include <unistd.h>
#include "disvm/clock_port.h"

#ifdef DVM_RUNTIME_EDITOR
#include "editor_view.h"
#include "editor_thread.h"
#include "thread_port.h"


/* Initialize, poll and release the UI channel boundary on this sole native owner. */
extern int dvm_editor_start(const char *directory);
extern int dvm_editor_poll(void);
extern int dvm_editor_attached(void);
extern void dvm_editor_stop(void);
#endif

#define ARENA_BYTES (64UL * 1024UL * 1024UL) /* Bounded experimental heap arena. */
#define PROCESS_MAX DVM_RUNTIME_PROCESS_MAX                      /* Maximum simultaneous Limbo processes. */

#include "heap_pool.h"

/* Initialize the resolver's owned native lock. No arguments/result; raises on failure. */
extern void dvm_srv_initialize(void);


/* Register retained Srv exports for this exclusive owner. No arguments or result. */
#ifdef DVM_RUNTIME_COMPILER
/* Register the optional compiler library builtin on the sole execution owner.
 * No parameters/result; registration failure raises before module loading. */
extern void dvm_compiler_initialize(void);
#endif
extern void srvmodinit(void);


/* Register retained Loader exports/types. No arguments/result; requires initialized heap. */
extern void loadermodinit(void);


/* Register hosted Math exports. No arguments/result; requires initialized heap and Sys. */
extern void mathmodinit(void);


/* Register hosted scalar hybrid tables after the VM allocator is initialized. */
extern void dvm_bigtable_initialize(void);
extern void dvm_processctl_initialize(void);
#ifdef DVM_RUNTIME_NATIVE_DEMO
/* Register the optional teaching module on this owner after heap initialization. */
extern void dvm_nativedemo_initialize(void);
#endif


/* Copy live scalar table/snapshot counts for explicit diagnostic statistics. */
extern void dvm_bigtable_owners(unsigned long *, unsigned long *);
extern ulong gcnruns;                     /* Collector scheduler passes maintained by the owning Dis core. */
extern ulong gcepochs;                    /* Completed collector epochs; distinct from interpreter/native dispatch. */

static Pool memory;                       /* Pool mutated by Dis or its stopped-world collector. */
Pool *heapmem = &memory;                  /* Pool consumed by the production RC/cycle collector. */
static DVM_MessageRouter *guest_messages; /* Dedicated guest endpoints, keyed by immutable PID. */
static int nextpid;                       /* Monotonic process identity. */
static int live;                          /* Atomic process quota accounting. */
static unsigned int resident_retirements; /* Dedicated exits still publishing completion. */
static int gc_locked_poll;                /* Retained mutex polling for reference comparison. */
static Prog *initial_guest; /* Controller-owned root guest in dedicated mode. */
static int native_spawn; /* One dedicated OS thread per spawned Prog. */
static int managed_workers = 1; /* Executor count before launch-profile selection. */
static unsigned long managed_workers_limit; /* Optional automatic pool resource cap; zero is uncapped. */
static int managed_limit_selected; /* Reject duplicate physical resource limits. */
static int managed_workers_selected; /* Reject duplicate executor options. */
static unsigned long progress_epoch; /* Shared acknowledged input generation, protected by the heap gate. */
static int gc_worker;                     /* Collector-worker admission selected at launch. */
static char *module_directory;            /* Read-only module resource base; ordinary Sys paths retain caller cwd. */
extern int dontcompile;                   /* Production loader switch that prohibits forced JIT compilation. */
int cflag;                                /* Native leaf compilation switch consumed by the loader. */
int keepbroken;                           /* No debugger retention is implemented in this initial runner. */
Type *TFD;                                /* Descriptor owner for supported module/control input handles. */
Type *TFileIO;                            /* Unavailable file-to-channel host type. */
unsigned long dvm_runtime_instructions;   /* Atomic instruction attempts across every Limbo process. */
static unsigned long native_instructions; /* Successfully executed native leaves, excluding rejected guards. */
unsigned long dvm_runtime_instruction_limit; /* Explicit total quota; zero retains the legacy slice policy. */
static int show_stats;                       /* Emit actual instruction counts on explicit request. */
static int scheduler_quantum = PQUANTA;      /* Instructions per scheduler turn; adjustable for GC/boundary testing. */
static int quantum_selected;  /* Reject duplicate quantum options rather than silently overriding them. */
static int inspect_reals;     /* Interpret the explicitly requested scalar range as binary64, not native words. */
static int inspect_requested; /* Explicit read-only post-execution module-data inspection. */
static size_t inspect_offset; /* Requested byte offset, aligned to the NC64 word width. */
static size_t inspect_count;  /* Requested native words, bounded to 4096 before execution. */
static uchar *inspect_data;   /* [GC:] Registered native MP root; only verified scalar cells are reported. */
static int test_control;      /* Explicit isolated-test halt capability; disabled for normal applications. */
static DVM_CloseJob *close_jobs[32]; /* Reserved native retirement nodes, under heap exclusion. */
static FVM_PortDirectory *directories[32]; /* Owned OS-port cursors; share the descriptor quota with files. */
static DVM_FileGate *file_gates[32];       /* Native per-slot FIFO, retained by rooted FD owners. */
static FVM_PortFile *files[32];            /* File slots 3..31 use FlashVM ports; standard 0..2 remain borrowed. */

/* Print copied instruction/native counters only for an explicit --stats request.
 * No parameters or VM mutation; terminal success/error/test-halt paths share this exact format. */
static void runtime_statistics(void)
{
    if (show_stats)
    {
        unsigned long tables;
        unsigned long keys;
        dvm_runtime_read_statistics();
        dvm_runtime_write_statistics();
        dvm_formatted_statistics();
        dvm_file_gate_statistics();
        dvm_metadata_statistics();
        dvm_open_statistics();
        dvm_close_statistics();
        dvm_runqueue_statistics();
        dvm_queue_probe_statistics();
        dvm_bigtable_owners(&tables, &keys);
        fprintf(stderr,
                "instructions=%lu native=%lu gc_runs=%lu gc_epochs=%lu tables_live=%lu keys_live=%lu\n",
                __atomic_load_n(&dvm_runtime_instructions, __ATOMIC_RELAXED),
                native_instructions,
                gcnruns,
                gcepochs,
                tables,
                keys);
    }
}

/* Resolve a diagnostic to decoded bytecode, including the caller of a failing native builtin.
 * Outputs borrow a loaded module path and copy a checked PC index; unknown locations use -1.
 * Integer span checks avoid subtracting a native/null program pointer from an unrelated PC. */
static void runtime_error_location(const char **path, long *pc)
{
    Modlink *module = DVM_R.M;
    Inst *instruction = DVM_R.PC;
    int boundary = dvm_native_execution()->instruction_boundary_error;
    *path = "(unknown)";
    *pc = -1;
    if (module == nil || module == H)
    {
        return;
    }
    if ((module->prog == nil || module->prog == H) && DVM_R.FP != nil && DVM_R.FP != H)
    {
        Frame *frame = (Frame *)DVM_R.FP;
        module = frame->mr;
        instruction = frame->lr;
        boundary = 0;
    }
    if (module != nil && module != H && module->m != nil)
    {
        uintptr base = (uintptr)module->prog;
        uintptr address = (uintptr)instruction;
        *path = module->m->path;
        if (module->prog != nil && module->prog != H && address >= base && (address - base) % sizeof(Inst) == 0 &&
            (address - base) / sizeof(Inst) <= (uintptr)module->m->nprog)
        {
            long index = (long)((address - base) / sizeof(Inst));
            *pc = index - (!boundary && index > 0);
        }
    }
}

/* Admit one complete instruction boundary on this native owner, before decode or C/native execution.
 * The optional total limit includes guarded fallbacks once and is never reset by UI/stdin progress.
 * A rejection leaves the current PC and registers unmodified and raises through the process guard. */
void dvm_runtime_step(void)
{
    unsigned long count = __atomic_load_n(&dvm_runtime_instructions, __ATOMIC_RELAXED);
    do
    {
        if (dvm_runtime_instruction_limit && count >= dvm_runtime_instruction_limit)
        {
            dvm_native_execution()->instruction_boundary_error = 1;
            error("instruction limit exceeded");
        }
    } while (!__atomic_compare_exchange_n(&dvm_runtime_instructions, &count, count + 1,
        0, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
}

/* Parse one nonnegative decimal extent into caller storage, without signs, bases or overflow.
 * text is borrowed argv memory. Returns zero without publishing partial results on invalid input. */
static int runtime_extent(const char *text, size_t *result)
{
    size_t value = 0;
    const unsigned char *cursor = (const unsigned char *)text;
    if (!*cursor)
    {
        return 0;
    }
    for (; *cursor; cursor++)
    {
        unsigned digit;
        if (*cursor < '0' || *cursor > '9')
        {
            return 0;
        }
        digit = *cursor - '0';
        if (value > ((size_t)INT_MAX - digit) / 10U)
        {
            return 0;
        }
        value = value * 10U + digit;
    }
    *result = value;
    return 1;
}

/* Retain the actual entry-instance data before its last process can release it.
 * process is borrowed on the sole VM owner. Check every requested NC64 cell against the type map;
 * other module cells may hold imports/strings, but a requested heap-reference cell is rejected. */
static void inspect_retain(Prog *process)
{
    Heap *heap;
    size_t index;
    uchar *data = process->R.MP;
    if (data == nil || data == H)
    {
        error("inspection requires entry module data");
    }
    heap = D2H(data);
    if (!heap->t || heap->t->np < 0 || heap->t->size < 0 || inspect_offset % 8U != 0 ||
        inspect_offset > (size_t)heap->t->size || inspect_count > ((size_t)heap->t->size - inspect_offset) / 8U)
    {
        error("inspection requires an aligned in-bounds range in pointer-free module data");
    }
    for (index = 0; index < inspect_count; index++)
    {
        size_t cell = inspect_offset / 8U + index;
        if (cell / 8U < (size_t)heap->t->np && (heap->t->map[cell / 8U] & (128U >> (cell % 8U))))
        {
            error("inspection requires an aligned in-bounds range in pointer-free module data");
        }
    }
    if (dvm_refcount_load_u(&heap->ref) == 0 || dvm_refcount_load_u(&heap->ref) == (ulong)-1)
    {
        error("inspection encountered an invalid module-data owner");
    }
    ptradd(heap); /* [GC:] Register before retaining; allocation failure transfers no extra reference. */
    if (dvm_refcount_retain_u(&heap->ref) != DVM_REFCOUNT_LIVE)
    {
        ptrdel(heap);
        error(exHeap);
    }
    Setmark(heap);
    inspect_data = data; /* [GC:] Native root keeps the mapped module graph alive until report/cleanup. */
}

/* Describe the current bound owner to exception diagnostics without a payload lock.
 * Input registers must be current; name is borrowed writable capacity storage. */
WORD dvm_exception_status(REG *registers, char *name, int capacity)
{
    const char *path;
    long pc;
    USED(registers);
    runtime_error_location(&path, &pc);
    if (capacity > 0)
    {
        snprint(name, capacity, "%s", path);
    }
    return (WORD)pc;
}

/* Retire the optional native inspection retain on the sole VM owner; repeated cleanup is harmless.
 * The active process may still own the same scalar data on an error path; its root is unaffected. */
static void inspect_release(void)
{
    if (inspect_data)
    {
        ptrdel(D2H(inspect_data)); /* [GC:] Remove exactly this native registration before retiring its retain. */
        destroy(inspect_data);
        inspect_data = nil;        /* [GC:] Retire exactly the extra native root, never a process-owned reference. */
    }
}

/* Report copied signed native words or binary64 after all processes have stopped, then release the retain.
 * No Dis stack/frame address is exposed. Returns zero on output failure; never changes module data. */
static int inspect_report(void)
{
    size_t index;
    int ok = 1;
    for (index = 0; index < inspect_count; index++)
    {
        int written; /* Host output status; either view copies bytes without aliasing typed storage. */
        size_t offset = inspect_offset + index * 8U;
        if (inspect_reals)
        {
            double value; /* NC64 REAL uses the host binary64 representation admitted by this runtime. */
            memcpy(&value, inspect_data + offset, sizeof(value));
            written = printf("REAL %zu %.17g\n", offset, value);
        }
        else
        {
            int64_t value;
            memcpy(&value, inspect_data + offset, sizeof(value));
            written = printf("DATA %zu %" PRId64 "\n", offset, value);
        }
        if (written < 0)
        {
            ok = 0;
            break;
        }
    }
    inspect_release();
    return ok;
}

/* Report allocator corruption and terminate instead of continuing with invalid metadata. */
static void allocator_fault(void *address, char *reason, ulong pc)
{
    USED(address);
    USED(pc);
    fprintf(stderr, "disvm allocator: %s\n", reason);
    exit(1);
}

void (*poolfault)(void *, char *, ulong) = allocator_fault; /* Production D2B guard hook. */

#define CANCEL_REQUESTS_PER_TURN 8
#define CANCEL_REQUEST_CAPACITY 64 /* Bounded host requests, containing numeric PIDs only. */

static int cancel_requests[CANCEL_REQUEST_CAPACITY]; /* FIFO under the channel mutex. */
static size_t cancel_request_count; /* Pending records under the channel mutex. */
static int cancel_requests_open; /* NativeWorld scheduler is live, under the channel mutex. */

static FVM_PortSync *channel_queue_mutex; /* Lives for the hosted runtime. */
static unsigned int channel_buffer_waiters; /* Protected by the queue mutex. */
static int native_waits_stopping; /* Shutdown predicate under the queue mutex. */
static DVM_FileGate *stdio_gates[3]; /* Output aliases share an admission queue. */
static uint64_t channel_generation; /* Queue/readiness epoch protected by the same mutex. */


typedef struct NativeGuestWait
{
    int pid; /* Copied from the current guest before releasing NativeWorld. */
    int cancelled; /* Protected by the queue mutex. */
    DVM_FileGate *file_gate; /* Borrowed through descriptor pin, or NULL. */
    int wake_fd; /* Owned by a pipe reader until it unlinks this record. */
    DVM_FileTicket *file_ticket; /* Live caller storage through queue admission. */
    struct NativeGuestWait *next; /* Linked only while this stack record is live. */
} NativeGuestWait;

static NativeGuestWait *native_guest_waits; /* Protected by the queue mutex. */

static int cancel_guest_wait(NativeGuestWait *wait);


/* Search only while holding the ledger lock. */
static size_t guest_cancel_request_index(int pid)
{
    size_t index;

    for (index = 0; index < cancel_request_count && cancel_requests[index] != pid; index++)
    {
    }
    return index;
}



/* Remove a request without changing FIFO order. The ledger is locked. */
static void remove_guest_cancel_request(size_t index)
{
    cancel_request_count--;
    memmove(cancel_requests + index, cancel_requests + index + 1,
        (cancel_request_count - index) * sizeof(*cancel_requests));
}


/* Publish the current guest identity before suspension, and return the
 * readiness epoch from the same critical section. */
static uint64_t native_guest_wait_begin(NativeGuestWait *wait, DVM_FileGate *gate, DVM_FileTicket *ticket, int wake_fd)
{
    Prog *process = dvm_native_context_process(dvm_native_context_current());
    uint64_t generation;
    size_t request;

    wait->pid = process ? process->pid : 0;
    wait->cancelled = 0;
    wait->file_gate = gate;
    wait->file_ticket = ticket;
    wait->wake_fd = wake_fd;
    fvm_port_sync_lock(channel_queue_mutex);
    wait->next = native_guest_waits;
    native_guest_waits = wait;
    request = guest_cancel_request_index(wait->pid);
    if (request < cancel_request_count)
    {
        /* Transfer the queued request to this exact continuation. */
        remove_guest_cancel_request(request);
        cancel_guest_wait(wait);
    }
    if (native_waits_stopping && gate && !wait->cancelled)
    {
        wait->cancelled = dvm_file_gate_cancel(gate, ticket);
    }
    generation = channel_generation;
    fvm_port_sync_unlock(channel_queue_mutex);
    return generation;
}


/* Skip native admission when cancellation has already won publication.
 * wait borrows a registered stack record. The ledger is released before
 * requesting a heap transition. Return -2 for early cancellation, otherwise
 * the audited release result; a later cancellation still wakes the waiter. */
static int native_guest_wait_try_leave(NativeGuestWait *wait)
{
    int cancelled;

    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = native_waits_stopping || wait->cancelled;
    fvm_port_sync_unlock(channel_queue_mutex);
    if (cancelled)
    {
        return -2;
    }
    return dvm_runtime_gc_native_try_leave();
}


/* Remove the record after restoring NativeWorld. Cancellation can still
 * win while the guest is reacquiring its continuation. */
static int native_guest_wait_end(NativeGuestWait *wait, int released)
{
    NativeGuestWait **link = &native_guest_waits;
    int completed;

    fvm_port_sync_lock(channel_queue_mutex);
    while (*link != wait)
    {
        link = &(*link)->next;
    }
    *link = wait->next;
    completed = native_waits_stopping || wait->cancelled ? -1 : released;
    fvm_port_sync_unlock(channel_queue_mutex);
    return completed;
}


/* With the ledger locked, cancel only operations still queued for I/O.
 * File gates never call into the ledger while holding their own mutex. */
static int cancel_guest_wait(NativeGuestWait *wait)
{
    if (wait->cancelled)
    {
        return 1;
    }
    if (wait->file_gate && !dvm_file_gate_cancel(wait->file_gate, wait->file_ticket) &&
        wait->wake_fd < 0)
    {
        return 0;
    }
    wait->cancelled = 1;
    if (wait->wake_fd >= 0)
    {
        int status;
        do
        {
            status = eventfd_write(wait->wake_fd, 1);
        } while (status < 0 && errno == EINTR);
        if (status < 0 && errno != EAGAIN)
        {
            abort();
        }
    }
    return 1;
}


static int cancel_scheduler_guest_wait(int pid);


/* Find and cancel a registered continuation while holding the ledger lock.
 * A negative result means the PID has no native wait. */
static int cancel_registered_guest_wait(int pid)
{
    NativeGuestWait *wait;

    for (wait = native_guest_waits; wait; wait = wait->next)
    {
        if (wait->pid == pid)
        {
            int cancelled = cancel_guest_wait(wait);
            fvm_port_sync_signal_all(channel_queue_mutex);
            return cancelled;
        }
    }
    return -1;
}


/* A NativeWorld owner can also cancel a scheduler-managed wait. */
int dvm_runtime_cancel_guest_wait(int pid)
{
    int cancelled;

    if (!channel_queue_mutex || pid <= 0)
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = cancel_registered_guest_wait(pid);
    fvm_port_sync_unlock(channel_queue_mutex);
    return cancelled < 0 ? cancel_scheduler_guest_wait(pid) : cancelled;
}


/* Request cancellation from an unbound host thread. Once a native wait is
 * registered, this request belongs to that continuation even if it cannot
 * be cancelled. Otherwise queue the PID for the scheduler or publication. */
int dvm_runtime_request_guest_cancel(int pid)
{
    int cancelled;

    if (!channel_queue_mutex || pid <= 0)
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancelled = cancel_registered_guest_wait(pid);
    if (cancelled >= 0)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return cancelled;
    }
    if (!cancel_requests_open || native_waits_stopping)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 0;
    }
    if (guest_cancel_request_index(pid) < cancel_request_count)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 1;
    }
    if (cancel_request_count == CANCEL_REQUEST_CAPACITY)
    {
        fvm_port_sync_unlock(channel_queue_mutex);
        return 0;
    }
    cancel_requests[cancel_request_count++] = pid;
    dvm_managed_workers_wake();
    fvm_port_sync_unlock(channel_queue_mutex);
    return 1;
}


/* Check the request queue without taking the scheduler mutex. */
static int guest_cancel_pending(void)
{
    int pending;

    fvm_port_sync_lock(channel_queue_mutex);
    pending = cancel_request_count != 0;
    fvm_port_sync_unlock(channel_queue_mutex);
    return pending;
}


/* At the final idle boundary, either keep the controller alive to drain
 * accepted requests or close admission before releasing the ledger lock. */
static int guest_cancel_pending_or_close(void)
{
    int pending;

    /* A dedicated owner can have work before its guest becomes runnable. */
    if (native_spawn)
    {
        if (__atomic_load_n(&resident_retirements, __ATOMIC_ACQUIRE))
        {
            return 1;
        }
        for (Prog *guest = dvm_runtime_process_roots(); guest; guest = guest->next)
        {
            DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
            if (__atomic_load_n(&process->startup_pending, __ATOMIC_ACQUIRE) || (process->messages && (
                dvm_messages_pending(process->messages, 0, 0) ||
                dvm_messages_pending(process->messages, 1, 0) ||
                dvm_managed_workers_reply_in_transit(guest))))
            {
                return 1;
            }
        }
    }
    /* Read the source of each handoff before the destination it publishes. */
    if (native_spawn && (dvm_runqueue_dispatch_pending() ||
        __atomic_load_n(&resident_retirements, __ATOMIC_ACQUIRE) ||
        dvm_runqueue_inflight() || dvm_runqueue_ready()))
    {
        return 1;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    pending = cancel_request_count != 0;
    if (!pending)
    {
        cancel_requests_open = 0;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return pending;
}


#define DVM_CHILD_EXIT_WAIT (Pbroken + 2) /* Hosted scheduler state distinct from Bigtable waits. */


/* EN: Route asynchronous host cancellation to the owner of a supported wait.
 * RU: Передать асинхронную отмену хоста владельцу поддерживаемого ожидания. */
static int post_host_wait_cancel(int pid)
{
    if (!native_spawn)
    {
        return 0;
    }
    Prog *peer;
    for (peer = dvm_runtime_process_roots(); peer && peer->pid != pid; peer = peer->next)
    {
    }
    if (!peer || (peer->state != Psend && peer->state != Precv && peer->state != Palt &&
        peer->state != DVM_CHILD_EXIT_WAIT))
    {
        return 0;
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    if (process->channel_claimed || dvm_runqueue_dispatch_owner(peer))
    {
        return 0;
    }
    int posted = dvm_messages_send_once(guest_messages, (uintptr_t)pid, &process->channel_cancel);
    if (posted == 1)
    {
        dvm_managed_workers_wake_guest(peer);
    }
    return posted != 0;
}


/* Limit cancellation work before the next guest turn. Keep each PID
 * visible until its delivery finishes so concurrent submissions coalesce. */
static int dispatch_guest_cancellations(void)
{
    size_t remaining;
    int worked;

    fvm_port_sync_lock(channel_queue_mutex);
    remaining = cancel_request_count < CANCEL_REQUESTS_PER_TURN ?
        cancel_request_count : CANCEL_REQUESTS_PER_TURN;
    worked = remaining != 0;
    while (remaining-- && cancel_request_count)
    {
        int pid = cancel_requests[0];

        fvm_port_sync_unlock(channel_queue_mutex);
        if (!post_host_wait_cancel(pid))
        {
            dvm_runtime_cancel_guest_wait(pid);
        }
        fvm_port_sync_lock(channel_queue_mutex);
        if (cancel_request_count)
        {
            /* Only shutdown can clear the queue while this owner delivers. */
            remove_guest_cancel_request(0);
        }
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return worked;
}


/* Admit an audited file operation, with the guest cancellable only until
 * it wins the cursor. On refusal restore the original heap ownership. */
int dvm_file_gate_try_release(DVM_FileGate *gate, DVM_FileTicket *ticket)
{
    NativeGuestWait wait;
    int released, admitted;

    dvm_file_gate_reserve(gate, ticket);
    if (!gate)
    {
        return dvm_runtime_gc_native_try_leave();
    }
    native_guest_wait_begin(&wait, gate, ticket, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released < 0)
    {
        dvm_file_gate_cancel(gate, ticket);
        return native_guest_wait_end(&wait, 0) < 0 ? -2 : -1;
    }
    admitted = released ? dvm_file_gate_enter(gate, ticket) : dvm_file_gate_enter_now(gate, ticket);
    native_guest_wait_end(&wait, released);
    if (!admitted)
    {
        dvm_runtime_gc_native_enter(released);
        return released ? -2 : -1;
    }
    return released;
}


/* Wait for the requested pipe readiness or its eventfd cancellation. */
static int wait_stdio_pipe(NativeGuestWait *wait, int wake_fd, int descriptor, short events, int timeout)
{
    struct pollfd descriptors[2] = {{descriptor, events, 0}, {wake_fd, POLLIN, 0}};
    int ready, available;

    do
    {
        ready = poll(descriptors, 2, timeout);
    } while (ready < 0 && errno == EINTR);
    if (ready <= 0 || descriptors[1].revents ||
        (events == POLLOUT && (descriptors[0].revents & (POLLERR | POLLHUP))) ||
        !(descriptors[0].revents & (events == POLLIN ? POLLIN | POLLHUP : POLLOUT)))
    {
        return 0;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    available = !wait->cancelled;
    fvm_port_sync_unlock(channel_queue_mutex);
    return available;
}


/* Decide whether an uncommitted nonblocking pipe transfer should repoll.
 * failure is the saved syscall errno. An interrupt does not establish that
 * a protected caller would block; EAGAIN does for a zero-timeout caller. */
static int retry_stdio_splice(int failure, int timeout)
{
    return failure == EINTR || (failure == EAGAIN && timeout != 0);
}


/* Read a borrowed FIFO through a private pipe without changing its flags.
 * The ledger serializes cancellation against the nonblocking consumption
 * attempt. Advisory readiness alone does not commit the read. */
static FVM_Result read_stdio_pipe(NativeGuestWait *wait, int wake_fd, int descriptor,
    void *bytes, size_t capacity, size_t *received, int timeout)
{
    int staging[2];
    ssize_t count = -1;
    FVM_Result status = FVM_ERROR;

    if (pipe2(staging, O_CLOEXEC | O_NONBLOCK))
    {
        return FVM_OUT_OF_MEMORY;
    }
    while (wait_stdio_pipe(wait, wake_fd, descriptor, POLLIN, timeout))
    {
        int retry;

        fvm_port_sync_lock(channel_queue_mutex);
        if (wait->cancelled)
        {
            fvm_port_sync_unlock(channel_queue_mutex);
            break;
        }
        count = splice(descriptor, NULL, staging[1], NULL, capacity, SPLICE_F_NONBLOCK);
        retry = count < 0 && retry_stdio_splice(errno, timeout);
        if (count >= 0)
        {
            wait->wake_fd = -1;
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        if (!retry)
        {
            break;
        }
    }
    if (count >= 0)
    {
        size_t copied = 0;

        while (copied < (size_t)count)
        {
            ssize_t fragment = read(staging[0], (uchar *)bytes + copied, (size_t)count - copied);
            if (fragment < 0 && errno == EINTR)
            {
                continue;
            }
            if (fragment <= 0)
            {
                break;
            }
            copied += (size_t)fragment;
        }
        *received = copied;
        if (copied == (size_t)count)
        {
            status = FVM_OK;
        }
    }
    close(staging[0]);
    close(staging[1]);
    return status;
}


/* Feed bounded chunks through a private staging pipe. SPLICE_F_NONBLOCK
 * avoids blocking after an advisory POLLOUT even if another writer wins
 * the capacity. Only bytes that reach the borrowed pipe count as written. */
static FVM_Result write_stdio_pipe(NativeGuestWait *wait, int wake_fd, int descriptor,
    const void *bytes, size_t capacity, size_t *written, int timeout)
{
    const uchar *source = bytes;
    int staging[2];
    size_t staged = 0;
    ssize_t count;

    if (pipe2(staging, O_CLOEXEC | O_NONBLOCK))
    {
        return FVM_OUT_OF_MEMORY;
    }
    while (*written < capacity)
    {
        if (!staged)
        {
            size_t chunk = capacity - *written;
            if (chunk > PIPE_BUF)
            {
                chunk = PIPE_BUF;
            }
            do
            {
                count = write(staging[1], source + *written, chunk);
            } while (count < 0 && errno == EINTR);
            if (count <= 0)
            {
                break;
            }
            staged = (size_t)count;
        }
        if (!wait_stdio_pipe(wait, wake_fd, descriptor, POLLOUT, timeout))
        {
            break;
        }
        count = splice(staging[0], NULL, descriptor, NULL, staged, SPLICE_F_NONBLOCK);
        if (count < 0 && retry_stdio_splice(errno, timeout))
        {
            continue;
        }
        if (count <= 0)
        {
            break;
        }
        staged -= (size_t)count;
        *written += (size_t)count;
    }
    close(staging[0]);
    close(staging[1]);
    return *written || !capacity ? FVM_OK : FVM_ERROR;
}


/* Borrow one gate for each pipe direction. stdout and stderr may refer to
 * the same pipe, so share their gate when the native identities match. */
static DVM_FileGate *stdio_pipe_gate(int descriptor, const struct stat *metadata)
{
    DVM_FileGate *gate;
    struct stat other_metadata;
    int other = 3 - descriptor;
    int shared = descriptor != 0 && !fstat(other, &other_metadata) &&
        S_ISFIFO(other_metadata.st_mode) && metadata->st_dev == other_metadata.st_dev &&
        metadata->st_ino == other_metadata.st_ino;

    fvm_port_sync_lock(channel_queue_mutex);
    gate = stdio_gates[descriptor];
    if (!gate && shared)
    {
        gate = stdio_gates[other];
    }
    if (!gate)
    {
        gate = dvm_file_gate_create();
    }
    stdio_gates[descriptor] = gate;
    if (shared)
    {
        stdio_gates[other] = gate;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return gate;
}


/* A pipe has a cancellable readiness phase. The borrowed descriptor is never
 * reconfigured. Return to the original heap owner on every path. */
static FVM_Result runtime_stdio_transfer(int descriptor, void *input, const void *output,
    size_t capacity, size_t *transferred, int *released_out)
{
    struct stat metadata;
    NativeGuestWait wait;
    DVM_FileTicket ticket;
    DVM_FileGate *gate;
    FVM_Result status = FVM_ERROR;
    int wake_fd, released;

    *released_out = 0;
    *transferred = 0;
    if (!capacity || fstat(descriptor, &metadata) || !S_ISFIFO(metadata.st_mode))
    {
        released = dvm_runtime_gc_native_try_leave();
        if (released < 0)
        {
            return FVM_OUT_OF_MEMORY;
        }
        status = input ? fvm_port_stdio_read(descriptor, input, capacity, transferred) :
            fvm_port_stdio_write(descriptor, output, capacity, transferred);
        dvm_runtime_gc_native_enter(released);
        *released_out = released;
        return status;
    }
    gate = stdio_pipe_gate(descriptor, &metadata);
    if (!gate)
    {
        return FVM_OUT_OF_MEMORY;
    }
    wake_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wake_fd < 0)
    {
        return FVM_OUT_OF_MEMORY;
    }
    dvm_file_gate_reserve(gate, &ticket);
    native_guest_wait_begin(&wait, gate, &ticket, wake_fd);
    released = native_guest_wait_try_leave(&wait);
    if (released < 0)
    {
        dvm_file_gate_cancel(gate, &ticket);
        status = released == -2 ? FVM_ERROR : FVM_OUT_OF_MEMORY;
    }
    else
    {
        int timeout = !released && gc_worker ? 0 : -1;
        int admitted = released ? dvm_file_gate_enter(gate, &ticket) :
            dvm_file_gate_enter_now(gate, &ticket);

        if (admitted)
        {
            if (input)
            {
                status = read_stdio_pipe(&wait, wake_fd, descriptor, input, capacity, transferred, timeout);
            }
            else
            {
                status = write_stdio_pipe(&wait, wake_fd, descriptor, output, capacity, transferred, timeout);
            }
            if (!dvm_file_gate_leave(gate, &ticket))
            {
                abort();
            }
        }
        dvm_runtime_gc_native_enter(released);
        *released_out = released;
    }
    native_guest_wait_end(&wait, released);
    close(wake_fd);
    return status;
}


/* Read staged stdin bytes and return after restoring the guest continuation. */
FVM_Result dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received)
{
    int released;

    return runtime_stdio_transfer(0, bytes, NULL, capacity, received, &released);
}


/* Write staged stdout/stderr bytes. Return any committed prefix, even if a
 * later pipe fragment is cancelled. released reports a completed heap lease. */
FVM_Result dvm_runtime_write_stdio(int descriptor, const void *bytes, size_t capacity,
    size_t *written, int *released)
{
    return runtime_stdio_transfer(descriptor, NULL, bytes, capacity, written, released);
}


/* Serialize short queue operations and unlock before propagating a VM error. */
void dvm_runtime_channel_queue_guard(int (*operation)(void *), void *argument)
{
    int completed;
    fvm_port_sync_lock(channel_queue_mutex);
    completed = kprotect(operation, argument);
    fvm_port_sync_unlock(channel_queue_mutex);
    if (!completed)
    {
        krethrow();
    }
}


extern int dvm_runtime_channel_buffer_busy(Channel *channel);


/* Wait on the native reservation ledger only. The caller keeps the channel
 * identity live and must have no bound VM context. Idle is an observation,
 * so the caller must still revalidate when it later acquires a reservation. */
static int wait_channel_buffer_idle(Channel *channel, NativeGuestWait *wait)
{
    int completed;
    fvm_port_sync_lock(channel_queue_mutex);
    while (!native_waits_stopping && (!wait || !wait->cancelled) &&
        dvm_runtime_channel_buffer_busy(channel))
    {
        channel_buffer_waiters++;
        fvm_port_sync_wait(channel_queue_mutex);
        channel_buffer_waiters--;
    }
    completed = !native_waits_stopping;
    fvm_port_sync_unlock(channel_queue_mutex);
    return completed;
}


/* Unbound native callers use only the reservation ledger. */
int dvm_runtime_channel_wait_idle(Channel *channel)
{
    if (!channel || channel == H || dvm_native_context_current())
    {
        return 0;
    }
    return wait_channel_buffer_idle(channel, NULL);
}


/* The caller holds a rooted channel and has ruled out a mover dependency.
 * Native leave withdraws the current Prog until the same owner resumes it. */
int dvm_runtime_channel_wait_guest(Channel *channel)
{
    NativeGuestWait wait;
    int released;

    native_guest_wait_begin(&wait, NULL, NULL, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released > 0)
    {
        wait_channel_buffer_idle(channel, &wait);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released < 0 ? -2 : released);
}


/* Wait for queue publication or reservation completion without managed
 * access. Capture the epoch before releasing NativeWorld to avoid losing a
 * wakeup. The caller has withdrawn ALT registrations and will retry. */
int dvm_runtime_channel_wait_change(void)
{
    NativeGuestWait wait;
    uint64_t generation = native_guest_wait_begin(&wait, NULL, NULL, -1);
    int released = native_guest_wait_try_leave(&wait);

    if (released > 0)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        channel_buffer_waiters++;
        while (!native_waits_stopping && !wait.cancelled && generation == channel_generation)
        {
            fvm_port_sync_wait(channel_queue_mutex);
        }
        channel_buffer_waiters--;
        fvm_port_sync_unlock(channel_queue_mutex);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released < 0 ? -2 : released);
}


/* Wait for a monotonic duration on the native cancellation ledger. The
 * scalar duration is private and all VM state stays in the saved frame. */
int dvm_runtime_guest_sleep(unsigned long milliseconds)
{
    NativeGuestWait wait;
    uint64_t started, now, elapsed;
    int released, completed = milliseconds == 0;

    native_guest_wait_begin(&wait, NULL, NULL, -1);
    released = native_guest_wait_try_leave(&wait);
    if (released > 0)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        if (milliseconds && dvm_host_clock_ms(&started))
        {
            now = started;
            while (!native_waits_stopping && !wait.cancelled)
            {
                elapsed = now - started;
                if (elapsed > milliseconds)
                {
                    completed = 1;
                    break;
                }
                if (!fvm_port_sync_wait_ms(channel_queue_mutex, milliseconds - elapsed + 1) ||
                    !dvm_host_clock_ms(&now))
                {
                    break;
                }
            }
        }
        fvm_port_sync_unlock(channel_queue_mutex);
        dvm_runtime_gc_native_enter(released);
    }
    return native_guest_wait_end(&wait, released ? (completed ? 1 : -1) : 0);
}


/* Translate a guest wait result after its native continuation is restored.
 * A cancelled wait has a distinct guest-visible exception. */
void dvm_runtime_channel_require_wait(int outcome)
{
    if (outcome == -2)
    {
        error("channel wait needs an executor");
    }
    if (outcome < 0)
    {
        error("channel wait cancelled");
    }
    if (!outcome)
    {
        error("channel buffer transfer already active");
    }
}


/* Advance readiness after queue publication or reservation removal, under the mutex. */
void dvm_runtime_channel_buffer_notify(void)
{
    channel_generation++;
    if (channel_buffer_waiters)
    {
        fvm_port_sync_signal_all(channel_queue_mutex);
    }
}


/* Notify selectors once a guest waiter is committed. Temporary ALT queue
 * registrations are invisible to other selectors under NativeWorld. */
void dvm_runtime_channel_waiter_published(void)
{
    if (!dvm_runtime_guarded_copies)
    {
        return;
    }
    fvm_port_sync_lock(channel_queue_mutex);
    dvm_runtime_channel_buffer_notify();
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Cancel native guest waits before joining workers. A suspended guest
 * still reacquires NativeWorld and restores its continuation before unwind. */
void dvm_runtime_cancel_native_waits(void)
{
    if (channel_queue_mutex)
    {
        fvm_port_sync_lock(channel_queue_mutex);
        native_waits_stopping = 1;
        cancel_requests_open = 0;
        cancel_request_count = 0;
        for (NativeGuestWait *wait = native_guest_waits; wait; wait = wait->next)
        {
            cancel_guest_wait(wait);
        }
        fvm_port_sync_signal_all(channel_queue_mutex);
        fvm_port_sync_unlock(channel_queue_mutex);
    }
}


/* Create arena/mutex privately; VM allocation errors occur outside pool lock. */
static void arena_init(void)
{
    native_waits_stopping = 0;
    channel_queue_mutex = fvm_port_sync_create();
    if (!channel_queue_mutex || !dvm_heap_pool_init(&memory, ARENA_BYTES))
    {
        error(exNomem);
    }
}

/* Allocate ordinary host metadata; the VM heap uses its separate bounded arena. */
void *mallocz(ulong size, int clear)
{
    void *value = malloc(size);
    if (value && clear)
    {
        memset(value, 0, size);
    }
    return value;
}

/* Return the bound native owner's error buffer. Missing environment is a fatal
 * host protocol violation; no shared fallback may hide a foreign owner access.
 * Collector errors must never overwrite mutator text. No allocation/blocking. */
static char *owner_error(void)
{
    if (up && up->env && up->env->errstr)
    {
        return up->env->errstr;
    }
    fprintf(stderr, "disvm: error without a bound native environment\n");
    exit(1);
}


/* Preserve the current host error and unwind through the retained Proc error frames. */
void error(char *message)
{
    if (message != owner_error())
    {
        snprintf(owner_error(), ERRMAX, "%s", message);
    }
    nexterror();
}

/* Propagate the active error to the next enclosing native owner frame. */
void nexterror(void)
{
    if (!up || up->nerr <= 0)
    {
        fprintf(stderr, "disvm: %s\n", owner_error());
        exit(1);
    }
    if (!dvm_native_error_unwind())
    {
        fprintf(stderr, "disvm: native owner cleanup failed before error transfer\n");
        abort();
    }
    dvm_native_error_jump(up->estack[up->nerr], 1);
}

/* Run fallible construction with the real Proc jump-frame discipline. */
int kprotect(int (*callback)(void *), void *argument)
{
    int result;
    if (waserror())
    {
        return 0;
    }
    result = callback(argument);
    poperror();
    return result;
}

/* Re-raise after production cleanup. */
void krethrow(void)
{
    nexterror();
}

/* Save a native error for the retained module loader. */
void kwerrstr(char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    vsnprint(owner_error(), ERRMAX, format, arguments);
    va_end(arguments);
}

/* Preserve the exclusive VM owner across synchronous host work. */
void release(void)
{
}

/* Return from synchronous host work on the same exclusive VM owner. */
void acquire(void)
{
}

/* Borrow this bound native owner's typed active Prog, or nil without an owner.
 * No process identity is inferred from the shared runnable head. Heap access
 * still requires the current exclusive turn; native waiters cannot execute. */
Prog *currun(void)
{
    return up ? up->prog : nil;
}


/* Append a non-leased runtime Prog under exclusive heap ownership. process is
 * borrowed; malformed/duplicate admission raises before modifying queue roots. */
void addrun(Prog *process)
{
    if (!dvm_runqueue_append(process))
    {
        error("invalid runnable process admission");
    }
}


/* Withdraw the TLS guest for a wait or exit without changing its GC root links.
 * state is the retained scheduler reason. A foreign head raises before removal. */
Prog *delrun(int state)
{
    Prog *process = dvm_runqueue_pop_expected(currun(), state);
    if (!process)
    {
        error("invalid runnable process removal");
    }
    return process;
}

/* EN: Dispose guest's endpoint once no producer can borrow the guest.
 * RU: Освободить endpoint гостя после ухода всех отправителей. */
static void drop_guest_messages(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (process->messages)
    {
        dvm_messages_stop(process->messages);
        while (dvm_messages_take(process->messages, 0, 0))
        {
            /* The request nodes are embedded in the guest. */
        }
        while (dvm_messages_take(process->messages, 1, 0))
        {
            /* Embedded replies are discarded on guest teardown. */
        }
        if (!dvm_messages_unregister(guest_messages, process->messages))
        {
            abort();
        }
        process->messages = NULL;
    }
}


/* EN: Initialize private process storage from an owner-local image.
 * RU: Инициализировать приватный Prog из локального образа владельца. */
static void initialize_process(Prog *process, Prog *parent, Modlink *module, const REG *initial)
{
    dvm_process_environment_init(process, parent);
    ((DVM_RuntimeProcess *)process)->parent_pid = parent ? parent->pid : 0;
    process->quanta = scheduler_quantum;
    process->xec = xec;
    process->exval = H;
    if (initial)
    {
        process->R = *initial;
    }
    else
    {
        process->R.M = module;
        process->R.MP = module->MP;
    }
}


/* EN: Reserve identity and initialize the private native message endpoint.
 * RU: Зарезервировать PID и подготовить приватный нативный ящик сообщений. */
int dvm_runtime_prepare_resident(Prog *process)
{
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
    if (__atomic_load_n(&live, __ATOMIC_RELAXED) == PROCESS_MAX || nextpid == INT_MAX)
    {
        resident->startup_error = "process quota exceeded";
        return 0;
    }
    process->pid = ++nextpid;
    if (native_spawn)
    {
        resident->channel_close.kind = DVM_PROG_MESSAGE_CHANNEL_CLOSE;
        resident->channel_exit.kind = DVM_PROG_MESSAGE_CHANNEL_EXIT;
        resident->channel_cancel.kind = DVM_PROG_MESSAGE_CHANNEL_CANCEL;
        resident->cancel_wait.kind = DVM_PROG_MESSAGE_CANCEL_WAIT;
        resident->cancel_wait.sender = (uintptr_t)resident->parent_pid;
        resident->cancel_wait.request = (uint64_t)process->pid;
        resident->messages = dvm_messages_register(guest_messages, (uintptr_t)process->pid);
        if (!resident->messages)
        {
            resident->startup_error = exNomem;
            return 0;
        }
    }
    __atomic_fetch_add(&live, 1, __ATOMIC_RELAXED);
    return 1;
}


/* EN: Register a prepared resident and start its owner. Consumes prepared storage.
 * RU: Зарегистрировать готовый процесс и запустить его владельца. Поглощает prepared. */
static Prog *create_process(Prog *parent, Modlink *module, const REG *initial, Prog *prepared)
{
    Prog *process = prepared;
    if (!process)
    {
        process = calloc(1, sizeof(DVM_RuntimeProcess));
        if (!process)
        {
            error(exNomem);
        }
        initialize_process(process, parent, module, initial);
    }
    if (dvm_refcount_retain_u(&D2H(module)->ref) != DVM_REFCOUNT_LIVE)
    {
        free(process);
        error(exHeap);
    }
    DVM_RuntimeProcess *resident = (DVM_RuntimeProcess *)process;
    if (native_spawn && parent)
    {
        __atomic_store_n(&resident->startup_pending, 1, __ATOMIC_RELEASE);
        process->state = Pready;
        /* Until registrar publication, the caller's frame still owns the arguments. */
        Heap *module_root = D2H(module);
        if (!dvm_runtime_root_add(module_root))
        {
            free(process);
            destroy(module);
            error(exHeap);
        }
        int started = dvm_managed_workers_start_guest(process);
        if (!dvm_runtime_root_remove(module_root))
        {
            abort();
        }
        if (!started)
        {
            const char *failure = resident->startup_error;
            if (resident->messages)
            {
                __atomic_fetch_sub(&live, 1, __ATOMIC_RELAXED);
            }
            drop_guest_messages(process);
            free(process);
            destroy(module);
            error((char *)(failure ? failure : "cannot create native guest thread"));
        }
    }
    else
    {
        if (!dvm_runtime_prepare_resident(process))
        {
            const char *failure = resident->startup_error;
            free(process);
            destroy(module);
            error((char *)failure);
        }
        dvm_runtime_process_root_add(process);
        addrun(process);
    }
    return process;
}


/* EN: Retained entry for callers without a prepared register image.
 * RU: Вход для вызывающих без подготовленного образа регистров. */
Prog *newprog(Prog *parent, Modlink *module)
{
    return create_process(parent, module, NULL, NULL);
}


/* EN: Native preparation ledger for a child whose source frame is still owned.
 * RU: Журнал подготовки потомка, исходный кадр ещё во владении создателя. */
typedef struct SpawnPreparation
{
    Prog *parent;   /* Current resident, whose source argument frame stays rooted. */
    Modlink *module; /* Rooted for the duration of private preparation. */
    Inst *entry;     /* Initial program counter captured by the spawn opcode. */
    Prog *initial;  /* Private stack and initial registers, freed by spawn on failure. */
    Prog *storage;  /* Private Prog allocation, transferred only after preparation. */
} SpawnPreparation;


/* EN: Allocate and fill only the child stack and its private process storage.
 * RU: Выделить и заполнить только приватный стек и Prog потомка. */
static int prepare_spawn_storage(void *argument)
{
    SpawnPreparation *request = argument;
    request->storage = calloc(1, sizeof(DVM_RuntimeProcess));
    if (!request->storage)
    {
        error(exNomem);
    }
    request->initial->R.M = request->module;
    request->initial->R.MP = request->module->MP;
    request->initial->R.PC = request->entry;
    newstack(request->initial);
    initialize_process(request->storage, request->parent, request->module, &request->initial->R);
    return 1;
}


/* EN: Prepare native storage outside exclusive admission, then publish the child.
 * RU: Подготовить нативную память вне эксклюзива, затем опубликовать потомка. */
void dvm_runtime_spawn_prepared(Prog *parent, Modlink *module, Inst *entry, Prog *initial)
{
    SpawnPreparation request = {parent, module, entry, initial, NULL};
    if (native_spawn)
    {
        Heap *module_root = D2H(module);
        if (!dvm_runtime_root_add(module_root))
        {
            error(exHeap);
        }
        int shared = dvm_runtime_gc_shared_try_enter();
        if (shared <= 0)
        {
            if (!dvm_runtime_root_remove(module_root))
            {
                abort();
            }
            error("cannot prepare native guest outside exclusive admission");
        }
        int prepared = kprotect(prepare_spawn_storage, &request);
        dvm_runtime_gc_shared_finish();
        if (!dvm_runtime_root_remove(module_root))
        {
            abort();
        }
        if (!prepared)
        {
            free(request.storage);
            krethrow();
        }
    }
    else if (!kprotect(prepare_spawn_storage, &request))
    {
        free(request.storage);
        krethrow();
    }
    (void)create_process(parent, module, NULL, request.storage);
}


typedef struct ChannelClaim
{
    Prog *peer;
    int (*validate)(void *); /* Optional queue-head check under the queue mutex. */
    int acquired;            /* Waiter was pinned and claimed. */
    void *request;
} ChannelClaim;


/* Acquire the completion hold while a scheduler or queue owner is live. */
static int acquire_channel_claim(void *argument)
{
    ChannelClaim *claim = argument;
    Prog *peer = claim->peer;
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    if (claim->validate && !claim->validate(claim->request))
    {
        return 1;
    }
    if (process->channel_claimed ||
        (peer->state != Psend && peer->state != Precv && peer->state != Palt))
    {
        if (native_spawn && claim->validate)
        {
            return 1;
        }
        error("channel waiter cannot be claimed");
    }
    if (!dvm_runqueue_pin(peer))
    {
        error("channel waiter cannot be pinned");
    }
    process->channel_claimed = claim;
    claim->acquired = 1;
    return 1;
}


/* Release the exact completion claim under the same mutex as acquisition.
 * peer remains pinned until after the claim is cleared. A mismatched identity
 * indicates a broken ownership invariant and cannot be recovered in the VM. */
static void release_channel_claim(Prog *peer, const void *owner)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    int runnable;
    fvm_port_sync_lock(channel_queue_mutex);
    if (process->channel_claimed != owner)
    {
        abort();
    }
    runnable = process->queued;
    dvm_runqueue_unpin(peer);
    process->channel_claimed = nil;
    if (runnable)
    {
        dvm_managed_workers_wake_guest(peer);
    }
    fvm_port_sync_unlock(channel_queue_mutex);
}


/* Finish outside the mutex. Cancellation and wake can enter the queue guard. */
static void finish_channel_claim(ChannelClaim *claim, int (*match)(void *))
{
    Prog *peer = claim->peer;
    int completed = kprotect(match, claim->request);
    release_channel_claim(peer, claim);
    if (!completed)
    {
        krethrow();
    }
}


/* External cancellation borrows the scheduler-owned waiter under NativeWorld. */
void dvm_runtime_channel_with_claim(Prog *peer, int (*match)(void *), void *request)
{
    ChannelClaim claim = {peer, nil, 0, request};
    dvm_runtime_channel_queue_guard(acquire_channel_claim, &claim);
    finish_channel_claim(&claim, match);
}


/* Check the queue head and pin it in the same mutex acquisition. */
int dvm_runtime_channel_with_queue_claim(Prog *peer, int (*validate)(void *),
    int (*match)(void *), void *request)
{
    ChannelClaim claim = {peer, validate, 0, request};
    dvm_runtime_channel_queue_guard(acquire_channel_claim, &claim);
    if (claim.acquired)
    {
        finish_channel_claim(&claim, match);
    }
    return claim.acquired;
}


/* Complete one detached wait under NativeWorld. Scheduler admission is the
 * state transition. Refusal leaves wait fields and any pending kill intact. */
void dvm_runtime_channel_wake(Prog *peer, char *cause)
{
    if (peer->state != Psend && peer->state != Precv && peer->state != Palt)
    {
        error("invalid channel waiter state");
    }
    if (!dvm_runqueue_complete_channel_wait(peer, cause))
    {
        error("invalid runnable process admission");
    }
}


extern void dvm_runtime_alt_preflight(Alt *alt);


/* Remove this guest's registrations under its completion claim. A pending
 * exception survives cancellation; a detached waiter is made runnable once. */
static int cancel_parked_channel_wait(void *argument)
{
    Prog *peer = argument;

    if (peer->state == Palt)
    {
        dvm_runtime_alt_preflight(peer->R.s);
        altdone(peer->R.s, peer, nil, -1);
    }
    else
    {
        Channel *channel = peer->chan;
        Progq **queue;
        Progq *entry;

        if (!channel || channel == H)
        {
            error(exHeap);
        }
        queue = peer->state == Psend ? &channel->send : &channel->recv;
        vmchanend(queue);
        for (entry = *queue; entry && entry->prog != peer; entry = entry->next)
        {
        }
        if (!entry)
        {
            error("channel waiter is missing its registration");
        }
        cqdelp(queue, peer);
    }
    dvm_runtime_channel_wake(peer, peer->kill ? nil : "channel wait cancelled");
    return 1;
}


/* Claim one scheduler-owned blocked guest under the active managed turn. */
static int cancel_parked_guest(Prog *peer)
{
    if (!peer || (peer->state != Psend && peer->state != Precv && peer->state != Palt) ||
        ((DVM_RuntimeProcess *)peer)->channel_claimed || dvm_runqueue_dispatch_owner(peer))
    {
        return 0;
    }
    dvm_runtime_channel_with_claim(peer, cancel_parked_channel_wait, peer);
    return 1;
}


static int cancel_child_exit_wait(Prog *process);


/* Cancel a scheduler-managed wait by PID. The caller must
 * own NativeWorld. An unbound, suspended, or already completed guest is left
 * alone. The public entry checks the native ledger before this fallback. */
static int cancel_scheduler_guest_wait(int pid)
{
    DVM_HeapOwner *owner = dvm_native_owner_current();
    DVM_HeapOwnerStats state;
    Prog *peer;

    if (pid <= 0 || !dvm_native_owner_world(owner) ||
        !dvm_native_owner_statistics(owner, &state) || state.phase != DVM_HEAP_ACTIVE)
    {
        return 0;
    }
    for (peer = dvm_runtime_process_roots(); peer && peer->pid != pid; peer = peer->next)
    {
    }
    return peer && !dvm_runqueue_dispatch_owner(peer) &&
        (cancel_parked_guest(peer) || dvm_bigtable_cancel_wait(peer) || cancel_child_exit_wait(peer));
}


/* Borrow a direct child while the caller owns the managed turn. */
static Prog *find_child(Prog *caller, int pid)
{
    Prog *child;

    for (child = dvm_runtime_process_roots(); child && child->pid != pid; child = child->next)
    {
    }
    return child && ((DVM_RuntimeProcess *)child)->parent_pid == caller->pid ? child : nil;
}


/* Authorize the hosted builtin against immutable spawning identity. */
int dvm_runtime_cancel_child_wait(Prog *caller, int pid)
{
    Prog *child = find_child(caller, pid);

    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    if (process->cancel_ready_pid == pid)
    {
        process->cancel_ready_pid = 0;
        return process->cancel_ready_result;
    }
    if (!child)
    {
        return Processctl_Denied;
    }
    return cancel_parked_guest(child) || dvm_bigtable_cancel_wait(child) ||
        cancel_child_exit_wait(child) || dvm_runtime_cancel_guest_wait(pid) ?
        Processctl_Cancelled : Processctl_NotWaiting;
}


/* EN: Post a cancellation for the child's supported wait and park caller.
 * RU: Отправить отмену поддерживаемого ожидания потомка и припарковать caller. */
int dvm_runtime_prepare_cancelwait(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    Prog *child = find_child(caller, pid);
    if (!native_spawn || process->cancel_ready_pid == pid || !child ||
        (child->state != Psend && child->state != Precv && child->state != Palt &&
         child->state != DVM_CHILD_EXIT_WAIT))
    {
        return 1;
    }
    DVM_RuntimeProcess *target = (DVM_RuntimeProcess *)child;
    if (target->channel_claimed)
    {
        return 1;
    }
    DVM_OwnerMessage *request = &target->cancel_wait;
    process->exit_wait_pid = pid;
    process->cancel_waiting = 1;
    delrun(DVM_CHILD_EXIT_WAIT);
    /* The child may already be polling its mailbox. Publish the caller's
     * reply destination and wait state before making the request visible. */
    if (!dvm_messages_send_once(guest_messages, (uintptr_t)pid, request))
    {
        process->exit_wait_pid = 0;
        process->cancel_waiting = 0;
        addrun(caller);
        error("cannot post wait cancellation");
    }
    DVM_R.PC--;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    dvm_managed_workers_wake_guest(child);
    return 0;
}


/* EN: Publish the outcome after taking the request off the old wait.
 * RU: Опубликовать исход после снятия запроса со старого ожидания. */
void dvm_runtime_wait_request_finished(Prog *guest, DVM_OwnerMessage *message)
{
    if (message->kind != DVM_PROG_MESSAGE_CANCEL_WAIT)
    {
        return;
    }
    Prog *parent;
    for (parent = dvm_runtime_process_roots(); parent && (uintptr_t)parent->pid != message->sender; parent = parent->next)
    {
    }
    if (!parent || parent->state != DVM_CHILD_EXIT_WAIT)
    {
        return;
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)parent;
    if (!process->cancel_waiting || process->exit_wait_pid != guest->pid)
    {
        return;
    }
    DVM_OwnerMessage *reply = &process->cancel_result;
    reply->sender = (uintptr_t)guest->pid;
    reply->request = message->request;
    reply->kind = DVM_PROG_MESSAGE_CANCEL_RESULT;
    reply->value = ((DVM_RuntimeProcess *)guest)->cancel_delivering ?
        Processctl_Cancelled : Processctl_NotWaiting;
    if (!dvm_messages_send(guest_messages, (uintptr_t)parent->pid, 1, reply))
    {
        abort();
    }
    dvm_managed_workers_wake_guest(parent);
}


/* Requeue an exiting parent without disturbing the child's exit request. */
static int wake_child_exit_wait(Prog *process)
{
    if (process->state != DVM_CHILD_EXIT_WAIT)
    {
        return 0;
    }
    ((DVM_RuntimeProcess *)process)->exit_wait_pid = 0;
    ((DVM_RuntimeProcess *)process)->cancel_waiting = 0;
    addrun(process);
    return 1;
}


/* Remove a parent's interest without undoing the already requested exit. */
static int detach_child_exit_wait(Prog *process)
{
    Prog *child;

    if (process->state != DVM_CHILD_EXIT_WAIT)
    {
        return 0;
    }
    child = find_child(process, ((DVM_RuntimeProcess *)process)->exit_wait_pid);
    if (child && !((DVM_RuntimeProcess *)process)->cancel_waiting)
    {
        ((DVM_RuntimeProcess *)child)->exit_notify_parent = 0;
    }
    return wake_child_exit_wait(process);
}


/* Attribute cancellation to the original MCALL, whose PC was rewound
 * before parking. The normal exception path owns prepared-frame cleanup. */
static int cancel_child_exit_wait(Prog *process)
{
    if (native_spawn)
    {
        Prog *resident = dvm_managed_workers_guest();
        if (process != (resident ? resident : initial_guest))
        {
            return 0;
        }
    }
    if ((process->flags & Pkilled) || !detach_child_exit_wait(process))
    {
        return 0;
    }
    if (native_spawn && dvm_native_execution()->resident_registers_valid)
    {
        DVM_R.PC++;
        process->R.PC = DVM_R.PC;
    }
    else
    {
        process->R.PC++;
    }
    if (!process->kill)
    {
        process->kill = "child exit wait cancelled";
    }
    return 1;
}


/* Return a blocked child to the scheduler for its ordinary exit turn. */
static int wake_for_termination(Prog *child)
{
    return cancel_parked_guest(child) || dvm_bigtable_wake_for_exit(child) ||
        detach_child_exit_wait(child);
}


/* EN: Post termination to a dedicated child parked on a channel.
 * RU: Передать завершение выделенному потомку, ожидающему канал. */
static int post_channel_exit(Prog *child)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)child;

    if (!native_spawn ||
        (child->state != Psend && child->state != Precv && child->state != Palt) ||
        process->channel_claimed || dvm_runqueue_dispatch_owner(child))
    {
        return 0;
    }
    child->flags |= Pkilled;
    if (dvm_messages_send_once(guest_messages, (uintptr_t)child->pid, &process->channel_exit) == 1)
    {
        dvm_managed_workers_wake_guest(child);
    }
    return 1;
}


/* Schedule termination for a runnable or supported blocked child. Never
 * retire a native stack on a foreign worker. The child unwinds it first. */
static int request_child_exit(Prog *child)
{
    if (!child)
    {
        return Processctl_Denied;
    }
    if (child->state == Prelease)
    {
        child->flags |= Pkilled;
        dvm_runtime_cancel_guest_wait(child->pid);
        return Processctl_Requested;
    }
    if (post_channel_exit(child))
    {
        return Processctl_Requested;
    }
    if (child->state == Pready || wake_for_termination(child))
    {
        child->flags |= Pkilled;
        return Processctl_Requested;
    }
    return Processctl_NotWaiting;
}


/* Resolve the direct child under the caller's managed turn before requesting exit. */
int dvm_runtime_kill_child(Prog *caller, int pid)
{
    return request_child_exit(find_child(caller, pid));
}


/* Request termination before committing MCALL. A successful request parks
 * the parent on its prepared frame until the child's exit publishes completion. */
int dvm_runtime_prepare_killwait(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;
    Prog *child;

    if (process->exit_ready_pid == pid)
    {
        return 1;
    }
    child = find_child(caller, pid);
    if (request_child_exit(child) != Processctl_Requested)
    {
        return 1;
    }
    ((DVM_RuntimeProcess *)child)->exit_notify_parent = 1;
    process->exit_wait_pid = pid;
    DVM_R.PC--;
    DVM_R.IC = 1;
    DVM_R.t = 1;
    delrun(DVM_CHILD_EXIT_WAIT);
    return 0;
}


/* Match an exit completion by exact child identity, then consume it. */
int dvm_runtime_killwait_result(Prog *caller, int pid)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)caller;

    if (process->exit_ready_pid == pid)
    {
        process->exit_ready_pid = 0;
        return Processctl_Retired;
    }
    return find_child(caller, pid) ? Processctl_NotWaiting : Processctl_Denied;
}


/* Publish retirement after stack, module and process storage are released.
 * The numeric parent identity remains valid even if the parent already exited. */
static void publish_child_exit(int parent_pid, int child_pid)
{
    Prog *parent;

    if (!parent_pid)
    {
        return;
    }
    for (parent = dvm_runtime_process_roots(); parent && parent->pid != parent_pid; parent = parent->next)
    {
    }
    if (parent && parent->state == DVM_CHILD_EXIT_WAIT &&
        ((DVM_RuntimeProcess *)parent)->exit_wait_pid == child_pid)
    {
        DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)parent;
        if (process->messages)
        {
            DVM_OwnerMessage *reply = &process->child_retired;
            reply->sender = (uintptr_t)child_pid;
            reply->request = (uint64_t)child_pid;
            reply->kind = DVM_PROG_MESSAGE_CHILD_RETIRED;
            if (!dvm_messages_send(guest_messages, (uintptr_t)parent_pid, 1, reply))
            {
                abort();
            }
            dvm_managed_workers_wake_guest(parent);
        }
        else
        {
            process->exit_ready_pid = child_pid;
            wake_child_exit_wait(parent);
        }
    }
}


/* End a requested guest only when its own instruction and native cleanup
 * are complete. The process guard then takes the ordinary exit path. */
void dvm_runtime_termination_checkpoint(Prog *process)
{
    if ((process->flags & Pkilled) && (native_spawn ? process->state == Pready : dvm_runqueue_head() == process))
    {
        error("");
    }
}


/* Consume the temporary channel owner inside a cleanup checkpoint. */
static int release_refill_channel(void *argument)
{
    destroy(argument);
    return 1;
}


/* Finish a buffered rendezvous under NativeWorld after ALT cancellation.
 * A refill error is primary. A later release error still reaches the receiver
 * but the sender succeeds once its payload is published. */
void dvm_runtime_channel_finish_refill(Channel *channel, Prog *peer,
    int (*refill)(void *), void *request, const int *committed)
{
    char first_error[ERRMAX];
    int transferred = kprotect(refill, request);
    int released;
    if (!transferred)
    {
        memmove(first_error, owner_error(), sizeof(first_error));
    }
    released = kprotect(release_refill_channel, channel);
    dvm_runtime_channel_wake(peer, transferred || *committed ? nil : "channel transfer failed");
    if (!transferred)
    {
        error(first_error);
    }
    if (!released)
    {
        krethrow();
    }
}


extern void dvm_runtime_altgone_claimed(Prog *peer);


/* Wake one detached waiter while the scheduler still owns its Prog. */
static int wake_closed_channel(void *argument)
{
    Prog *process = argument;
    switch (process->state)
    {
    case Psend:
    case Precv:
        dvm_runtime_channel_wake(process, "channel hangup");
        break;
    case Palt:
        dvm_runtime_altgone_claimed(process);
        break;
    case Prelease:
        error("foreign C channel waiter is not supported by this host");
        break;
    default:
        /* A previous registration may already have woken the same ALT. */
        break;
    }
    return 1;
}


extern Progq *dvm_runtime_channel_close_preflight(Progq **queue);


/* Test whether this queue entry participates in hosted channel completion. */
static int hosted_channel_waiter(Prog *peer)
{
    return peer && (peer->state == Psend || peer->state == Precv || peer->state == Palt);
}


/* Refuse another owner's claim before changing either queue. */
static void check_close_claims(Progq *entry)
{
    for (; entry; entry = entry->next)
    {
        Prog *peer = entry->prog;
        if (hosted_channel_waiter(peer) && (((DVM_RuntimeProcess *)peer)->channel_claimed ||
            dvm_runqueue_hold_count(peer) == INT_MAX || dvm_runqueue_hold_count(peer) < 0))
        {
            error("channel waiter cannot be claimed");
        }
    }
}


/* Reserve all eligible peers with one identity, including duplicate ALT nodes. */
static void claim_close_waiters(Progq *entry, const void *owner)
{
    for (; entry; entry = entry->next)
    {
        Prog *peer = entry->prog;
        if (hosted_channel_waiter(peer) && !((DVM_RuntimeProcess *)peer)->channel_claimed)
        {
            /* The complete preflight checked capacity before any mutation. */
            if (!dvm_runqueue_pin(peer))
            {
                abort();
            }
            ((DVM_RuntimeProcess *)peer)->channel_claimed = owner;
        }
    }
}


/* Drain an exclusively detached chain, recording the first wake failure.
 * Only the first registration consumes the claim, even on a wake error. */
static void drain_closed_queue(Progq *entry, const void *owner, char *first_error, int *failed)
{
    while (entry)
    {
        Progq *next = entry->next;
        Prog *peer = entry->prog;
        DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
        int claimed = hosted_channel_waiter(peer) && process->channel_claimed == owner;
        free(entry);
        entry = next;
        if (claimed || (peer && peer->state == Prelease))
        {
            int completed = kprotect(wake_closed_channel, peer);
            if (claimed)
            {
                release_channel_claim(peer, owner);
            }
            if (!completed && !*failed)
            {
                memmove(first_error, owner_error(), ERRMAX);
                *failed = 1;
            }
        }
        if (peer)
        {
            dvm_runqueue_unpin(peer);
        }
    }
}


typedef struct ClosedQueues
{
    Channel *channel; /* Whole-channel finalization only. */
    Progq **send_owner;
    Progq **recv_owner; /* NULL for the single-queue entry point. */
    Progq *send;
    Progq *recv;
    Array *buffer; /* Transferred buffer owner after successful detach. */
    Type *mapped; /* Transferred mover descriptor owner, if present. */
} ClosedQueues;


/* Validate and detach both owners while the queue mutex is held.
 * Wakeups run later and may themselves remove sibling registrations. */
static int detach_closed_queues(void *argument)
{
    ClosedQueues *queues = argument;
    Progq *send = *queues->send_owner;
    Progq *recv = queues->recv_owner ? *queues->recv_owner : nil;
    Progq *send_tail = send ? dvm_runtime_channel_close_preflight(queues->send_owner) : nil;
    Progq *recv_tail = recv ? dvm_runtime_channel_close_preflight(queues->recv_owner) : nil;
    if (queues->channel && dvm_runtime_channel_buffer_busy(queues->channel))
    {
        error("channel buffer transfer already active");
    }
    if (send_tail && send_tail == recv_tail)
    {
        error(exHeap);
    }
    check_close_claims(send);
    check_close_claims(recv);
    claim_close_waiters(send, queues);
    claim_close_waiters(recv, queues);
    queues->send = send;
    queues->recv = recv;
    *queues->send_owner = nil;
    if (queues->recv_owner)
    {
        *queues->recv_owner = nil;
    }
    if (queues->channel)
    {
        Channel *channel = queues->channel;
        queues->buffer = channel->buf;
        queues->mapped = channel->mover == movtmp ? channel->mid.t : nil;
        channel->buf = H;
        channel->mover = nil;
    }
    return 1;
}


/* Detach under the queue mutex and wake after unlocking. */
void killcomm(Progq **queue)
{
    ClosedQueues queues = {nil, queue, nil, nil, nil, H, nil};
    char first_error[ERRMAX];
    int failed = 0;
    dvm_runtime_channel_queue_guard(detach_closed_queues, &queues);
    drain_closed_queue(queues.send, &queues, first_error, &failed);
    if (failed)
    {
        error(first_error);
    }
}


/* Caller holds the channel queue mutex. Empty queues may own sentinels. */
static int channel_quiet(Channel *channel)
{
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    return !channel->aux && (!send || (!send->prog && !send->next && send != recv)) &&
        (!recv || (!recv->prog && !recv->next)) && !dvm_runtime_channel_buffer_busy(channel);
}


int dvm_runtime_channel_is_quiet(Channel *channel)
{
    fvm_port_sync_lock(channel_queue_mutex);
    int quiet = channel_quiet(channel);
    fvm_port_sync_unlock(channel_queue_mutex);
    return quiet;
}


/* EN: Post a close message to owners of the waits registered on channel.
 * Return zero for waiters outside dedicated mode or transport-owned channels.
 * RU: Передать закрытие владельцам ожиданий на channel. Вернуть ноль для
 * ожиданий вне выделенного режима или каналов под управлением транспорта. */
int dvm_runtime_channel_post_close(Channel *channel)
{
    int posted = 0;
    fvm_port_sync_lock(channel_queue_mutex);
    if (native_spawn && !channel->aux)
    {
        Progq *queues[] = {channel->send, channel->recv};
        int supported = 1;
        for (size_t index = 0; supported && index < sizeof(queues) / sizeof(*queues); index++)
        {
            for (Progq *entry = queues[index]; entry; entry = entry->next)
            {
                Prog *peer = entry->prog;
                if (peer && !hosted_channel_waiter(peer))
                {
                    supported = 0;
                    break;
                }
            }
        }
        for (size_t index = 0; supported && index < sizeof(queues) / sizeof(*queues); index++)
        {
            for (Progq *entry = queues[index]; entry; entry = entry->next)
            {
                Prog *peer = entry->prog;
                if (peer)
                {
                    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
                    if (dvm_messages_send_once(guest_messages, (uintptr_t)peer->pid,
                        &process->channel_close) == 1)
                    {
                        dvm_managed_workers_wake_guest(peer);
                    }
                    posted = 1;
                }
            }
        }
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    return posted;
}


/* Try the callback-free last-owner path after the reader epoch. */
int dvm_runtime_channel_claim_quiet(Heap *heap, Array *buffer, Type **mapped, int already_claimed)
{
    Channel *channel = H2D(Channel *, heap);
    fvm_port_sync_lock(channel_queue_mutex);
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    int claimed = channel->buf == buffer && channel_quiet(channel) &&
        (already_claimed ? dvm_refcount_load_u(&heap->ref) == 0 :
            dvm_refcount_claim_last_u(&heap->ref));
    if (claimed)
    {
        *mapped = channel->mover == movtmp ? channel->mid.t : nil;
        channel->send = nil;
        channel->recv = nil;
        channel->buf = H;
    }
    fvm_port_sync_unlock(channel_queue_mutex);
    if (claimed)
    {
        free(send);
        free(recv);
    }
    return claimed;
}


/* Finalize both queue owners together. NativeWorld keeps the channel and
 * registered processes live through resource release. */
void dvm_runtime_channel_finalize(Channel *channel, int swept)
{
    ClosedQueues queues = {channel, &channel->send, &channel->recv, nil, nil, H, nil};
    char first_error[ERRMAX];
    int failed = 0;
    dvm_runtime_channel_queue_guard(detach_closed_queues, &queues);
    drain_closed_queue(queues.send, &queues, first_error, &failed);
    drain_closed_queue(queues.recv, &queues, first_error, &failed);
    if (queues.mapped)
    {
        freetype(queues.mapped);
    }
    if (!swept && queues.buffer != H &&
        !kprotect(release_refill_channel, queues.buffer) && !failed)
    {
        memmove(first_error, owner_error(), ERRMAX);
        failed = 1;
    }
    if (failed)
    {
        error(first_error);
    }
}


/* Retained runt.c provides string services; hosted FD services use platform ports. */
static void free_fd(Heap *heap, int swept)
{
    Sys_FD *fd = H2D(Sys_FD *, heap);
    USED(swept);
    if (fd->fd >= 3 && fd->fd < 32)
    {
        kclose(fd->fd);
    }
}

/* Initialize the actual descriptor-owner finalizer used by the supported input path. */
void sysinit(void)
{
    if (TFD)
    {
        return;
    }
    TFD = dtype(free_fd, sizeof(Sys_FD), nil, 0);
    if (!TFD)
    {
        error(exNomem);
    }
}

/* Accept only ordinary Dis; dynamic native libraries and signatures are unavailable. */
int dynldable(int fd)
{
    USED(fd);
    return 0;
}

/* Do not require signatures for local experimental module files. */
int mustbesigned(char *path, uchar *bytes, ulong length, Dir *metadata)
{
    USED(path);
    USED(bytes);
    USED(length);
    USED(metadata);
    return 0;
}

/* Signed payload verification is explicitly unsupported. */
int verifysigner(uchar *signature, int size, uchar *bytes, ulong length)
{
    USED(bytes);
    USED(length);
    USED(signature);
    USED(size);
    return 0;
}

/* Reserve a descriptor under heap exclusion before constructing an unpublished
 * native handle. path is borrowed UTF-8, mode an immutable r/w/wx literal.
 * asynchronous admits audited Sys calls; loader calls retain their protected
 * synchronous boundary. Return a published slot or -1 with a logical error.
 * A non-NULL gate reserves the slot even before any handle exists. */
static int open_file_mode(const char *path, const char *mode, int asynchronous)
{
    int index;
    for (index = 3; index < 32; index++)
    {
        if (!files[index] && !directories[index] && !file_gates[index])
        {
            FVM_PortFile *file = NULL;
            FVM_PortDirectory *directory = NULL;
            FVM_Result status;
            file_gates[index] = dvm_file_gate_create();
            if (!file_gates[index])
            {
                kwerrstr("cannot allocate native file gate");
                return -1;
            }
            close_jobs[index] = dvm_close_prepare();
            if (!close_jobs[index])
            {
                dvm_file_gate_destroy(file_gates[index]);
                file_gates[index] = NULL;
                kwerrstr("cannot reserve native handle retirement");
                return -1;
            }
            status = dvm_open_wait(path, mode, asynchronous, &file, &directory);
            if (status == FVM_OK)
            {
                files[index] = file;
                directories[index] = directory;
                return index;
            }
            dvm_file_gate_destroy(file_gates[index]);
            file_gates[index] = NULL;
            dvm_close_discard(close_jobs[index]);
            close_jobs[index] = NULL;
            /* string2c storage may have been reused by another executor during
             * release. Do not inspect the caller's original path here. */
            kwerrstr("cannot open native file or directory (status %d)", status);
            return -1;
        }
    }
    kwerrstr("file handle quota exceeded");
    return -1;
}


/* Loader-only native input: borrowed path/mode, synchronous protected lifetime.
 * Return the owned descriptor slot or -1; the caller closes it with kclose. */
static int open_file(const char *path, const char *mode)
{
    return open_file_mode(path, mode, 0);
}

/* Map only the bundled library names from Inferno resource paths to package siblings.
 * path is borrowed. Returns the relative library name or the unchanged native path;
 * ordinary Sys file paths never pass through this module-resource mapping. */
static const char *bundled_module_path(const char *path)
{
    static const char *names[] = {"bufio.dis",
                                  "string.dis",
                                  "arg.dis",
                                  "readdir.dis",
                                  "debug.dis",
                                  "dis.dis",
                                  "styx.dis",
                                  "styxservers.dis",
                                  "regex.dis"}; /* Bundled module allowlist. */
    size_t index;
    if (strncmp(path, "/dis/lib/", 9) != 0)
    {
        return path;
    }
    for (index = 0; index < sizeof(names) / sizeof(names[0]); index++)
    {
        if (strcmp(path + 9, names[index]) == 0)
        {
            return path + 9;
        }
    }
    return path;
}

/* Open read-only module input; the full VM word is checked before this native call. */
int kopen(char *path, int mode)
{
    const char *resource = bundled_module_path(path);
    if (mode != OREAD)
    {
        kwerrstr("unsupported file open mode");
        return -1;
    }
    {
        char resolved[4096];
        size_t base;
        size_t length = strlen(resource);
        if (resource[0] == '/')
        {
            return open_file(resource, "r");
        }
        base = strlen(module_directory);
        if (base + length + 2 > sizeof(resolved))
        {
            kwerrstr("module resource path is too long");
            return -1;
        }
        memcpy(resolved, module_directory, base);
        resolved[base] = '/';
        memcpy(resolved + base + 1, resource, length + 1);
        return open_file(resolved, "r");
    }
}

/* Read one module fragment from its borrowed file slot. */
long kread(int fd, void *bytes, long size)
{
    size_t received;
    DVM_FileTicket ticket;
    FVM_Result status;
    if (fd < 0 || fd >= 32 || !files[fd] || size < 0)
    {
        return -1;
    }
    dvm_file_gate_reserve(file_gates[fd], &ticket);
    if (!dvm_file_gate_enter_now(file_gates[fd], &ticket))
    {
        return -1;
    }
    status = fvm_port_file_read(files[fd], bytes, size, &received);
    if (!dvm_file_gate_leave(file_gates[fd], &ticket))
    {
        fprintf(stderr, "disvm: loader file admission lost\n");
        exit(1);
    }
    if (status != FVM_OK)
    {
        return -1;
    }
    return received;
}

/* Borrow a live native file handle on the Dis owner. descriptor is a validated
 * runtime slot. The caller keeps its managed FD rooted until native I/O ends;
 * only then may GC close this same handle. Null rejects non-file slots. */
FVM_PortFile *dvm_runtime_file(int descriptor)
{
    return descriptor >= 3 && descriptor < 32 ? files[descriptor] : NULL;
}


/* Borrow only under the heap gate while a mapped FD/private loader slot pins
 * the paired native handle. No managed references cross into gate storage. */
DVM_FileGate *dvm_runtime_file_gate(int descriptor)
{
    return descriptor >= 3 && descriptor < 32 ? file_gates[descriptor] : NULL;
}


/* Record successful stdin progress after managed reentry, never from a worker
 * or inactive heap interval. No parameters/result, allocation or blocking. */
void dvm_runtime_input_progress(void)
{
    progress_epoch++;
    dvm_native_execution()->idle_slices = 0;
    dvm_native_execution()->progress_epoch = progress_epoch;
}


/* Close exactly the module handle owned by the loader. */
int kclose(int fd)
{
    if (fd < 3 || fd >= 32 || (!files[fd] && !directories[fd]))
    {
        return -1;
    }
    if (!dvm_file_gate_destroy(file_gates[fd]))
    {
        fprintf(stderr, "disvm: closing a file with pending native operations\n");
        exit(1);
    }
    file_gates[fd] = NULL;
    {
        FVM_PortFile *file = files[fd];
        FVM_PortDirectory *directory = directories[fd];
        DVM_CloseJob *job = close_jobs[fd];
        files[fd] = nil;
        directories[fd] = nil;
        close_jobs[fd] = NULL;
        dvm_close_submit(job, file, directory);
    }
    return 0;
}

/* Determine module extent without reading beyond its framed contents. */
Dir *kdirfstat(int fd)
{
    Dir *metadata;
    size_t available;
    DVM_FileTicket ticket;
    FVM_Result status;
    if (fd < 0 || fd >= 32 || !files[fd])
    {
        return nil;
    }
    dvm_file_gate_reserve(file_gates[fd], &ticket);
    if (!dvm_file_gate_enter_now(file_gates[fd], &ticket))
    {
        return nil;
    }
    status = fvm_port_file_size(files[fd], &available);
    if (!dvm_file_gate_leave(file_gates[fd], &ticket))
    {
        fprintf(stderr, "disvm: loader metadata admission lost\n");
        exit(1);
    }
    if (status != FVM_OK || available >= 8UL * 1024UL * 1024UL)
    {
        return nil;
    }
    metadata = calloc(1, sizeof(Dir));
    if (metadata)
    {
        metadata->length = available;
    }
    return metadata;
}

/* Module dynamic seeks are not required by ordinary Dis input. */
vlong kseek(int fd, vlong position, int origin)
{
    USED(fd);
    USED(position);
    USED(origin);
    error("native dynamic module seek unsupported");
    return -1;
}

/* Return a diagnostic for the interpreter's %r conversion. */
char *syserr(char *start, char *end, Prog *process)
{
    USED(process);
    return seprint(start, end, "%s", owner_error());
}

/* Reserve the complete native-cell entry frame before publishing its process. */
static Prog *start_module(Module *module)
{
    Import empty;
    Modlink *link;
    Prog initial;
    Prog *process;
    Frame *frame;
    memset(&initial, 0, sizeof(initial));
    memset(&empty, 0, sizeof(empty));
    if (!module->entryt || module->entryt->size < sizeof(Frame))
    {
        error("incompatible legacy module frame; native-cell module required");
    }
    link = linkmod(module, &empty, 1);
    frame = calloc(1, module->entryt->size);
    if (!frame)
    {
        error(exNomem);
    }
    frame->t = module->entryt;
    initial.R.M = link;
    initial.R.MP = link->MP;
    initial.R.PC = module->entry;
    DVM_R.s = &frame;
    newstack(&initial);
    initmem(module->entryt, initial.R.FP);
    process = create_process(nil, link, &initial.R, NULL);
    if (dvm_refcount_release_u(&D2H(link)->ref) != DVM_REFCOUNT_LIVE)
    {
        error(exHeap);
    }
    free(frame);
    return process;
}

/* Bind relative imports to the entry module's directory without changing caller cwd.
 * path is borrowed argv storage. Returns its basename for the initial logical module load. */
static char *entry_resource(char *path)
{
    char *separator = strrchr(path, '/');
    size_t length;
    if (strlen(path) >= 4096)
    {
        error("entry module path is too long");
    }
    if (!separator)
    {
        module_directory = strdup(".");
        if (!module_directory)
        {
            error(exNomem);
        }
        return path;
    }
    length = separator - path;
    if (!length)
    {
        length = 1;
    }
    module_directory = malloc(length + 1);
    if (!module_directory)
    {
        error(exNomem);
    }
    memcpy(module_directory, path, length);
    module_directory[length] = 0;
    return separator + 1;
}

/* EN: Let the dedicated owner complete guest's posted channel wait request.
 * RU: Завершить ожидание guest по сообщению на его потоке. */
static int consume_channel_message(Prog *guest)
{
    if (!hosted_channel_waiter(guest))
    {
        return 0;
    }
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process->messages || !dvm_messages_pending(process->messages, 0, 0) ||
        process->channel_claimed || dvm_runqueue_dispatch_owner(guest))
    {
        return 0;
    }
    if (!guest->kill && dvm_messages_pending(process->messages, 0, DVM_PROG_MESSAGE_CHANNEL_CLOSE))
    {
        guest->kill = "channel hangup";
    }
    process->cancel_delivering = 1;
    int cancelled = cancel_parked_guest(guest);
    process->cancel_delivering = 0;
    return cancelled;
}


/* EN: Consume a cancellation or retirement reply before dispatching the parent.
 * RU: Принять ответ об отмене или завершении до запуска родителя. */
static int consume_child_reply(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process || !process->messages)
    {
        return 0;
    }
    DVM_OwnerMessage *reply = dvm_managed_workers_take_reply(guest);
    if (!reply)
    {
        reply = dvm_messages_take(process->messages, 1, 0);
    }
    if (!reply || guest->state != DVM_CHILD_EXIT_WAIT ||
        reply->request != (uint64_t)process->exit_wait_pid)
    {
        return 0;
    }
    if (process->cancel_waiting && reply->kind == DVM_PROG_MESSAGE_CANCEL_RESULT)
    {
        process->cancel_ready_pid = (int)reply->request;
        process->cancel_ready_result = (int)reply->value;
    }
    else if (!process->cancel_waiting && reply->kind == DVM_PROG_MESSAGE_CHILD_RETIRED)
    {
        process->exit_ready_pid = (int)reply->request;
    }
    else
    {
        return 0;
    }
    return wake_child_exit_wait(guest);
}


/* EN: Finish process-wait cancellation on this resident's own executor.
 * RU: Завершить отмену ожидания процесса на его собственном исполнителе. */
static int consume_process_wait_requests(Prog *guest)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)guest;
    if (!process || !process->messages ||
        (guest->state != DVM_CHILD_EXIT_WAIT && guest->state != Pready))
    {
        return 0;
    }
    int cancelled = 0;
    DVM_OwnerMessage *message;
    while ((message = dvm_messages_take(process->messages, 0, 0)))
    {
        if (message->kind == DVM_PROG_MESSAGE_CANCEL_WAIT ||
            message->kind == DVM_PROG_MESSAGE_CHANNEL_CANCEL)
        {
            process->cancel_delivering = cancel_child_exit_wait(guest);
            cancelled |= process->cancel_delivering;
            dvm_runtime_wait_request_finished(guest, message);
            process->cancel_delivering = 0;
        }
    }
    return cancelled;
}


/* Execute one complete managed turn under exclusive heap ownership. Both OS
 * executors call the same interpreter/JIT/error/retirement logic. Return a
 * managed turn status, or -1 after retiring the claim on failure.
 * Registers and roots are published before returning; no UI pump runs here. */
static int runtime_dispatch_turn(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *process;
    int cancelled = gc_worker && dispatch_guest_cancellations();

    int reload_registers = cancelled;

    if (native_spawn)
    {
        Prog *guest = dvm_managed_workers_guest();
        guest = guest ? guest : initial_guest;
        cancelled |= consume_channel_message(guest);
        cancelled |= consume_child_reply(guest);
        dvm_managed_workers_finish_reply(guest);
        cancelled |= consume_process_wait_requests(guest);
        fvm_port_sync_lock(channel_queue_mutex);
        process = dvm_runqueue_select_guest(guest);
        fvm_port_sync_unlock(channel_queue_mutex);
    }
    else
    {
        process = dvm_runqueue_head();
    }
    if (reload_registers)
    {
        dvm_native_execution()->resident_registers_valid = 0;
    }
    if (!process)
    {
        return cancelled ? DVM_MANAGED_TURN_MAINTENANCE : DVM_MANAGED_TURN_IDLE;
    }
    if (dvm_native_execution()->progress_epoch != progress_epoch)
    {
        dvm_native_execution()->idle_slices = 0;
        dvm_native_execution()->progress_epoch = progress_epoch;
    }
    /* An explicit instruction budget must remain independent of quantum.
     * Otherwise quantum=1 silently stops at 100000 despite a larger limit. */
    if (!dvm_runtime_instruction_limit && ++dvm_native_execution()->idle_slices > 100000)
    {
        fprintf(stderr,"disvm: execution budget exceeded\n");
        return -1;
    }
    if (!dvm_runqueue_claim(process))
    {
        error("cannot publish active Dis process in TLS owner");
    }
    up->prog = process;
    if (!dvm_process_environment_bind(process))
    {
        error("invalid managed process environment");
    }
    dvm_native_execution()->instruction_boundary_error = 0;
    if (waserror())
    {
        dvm_native_execution()->resident_registers_valid = 0;
        dvm_native_execution()->arithmetic_retry_opcode = 0;
        ((DVM_RuntimeProcess *)process)->arithmetic_retries = 0;
        process->R = DVM_R;
        dvm_native_context_set_process(context, nil);
        dvm_bigtable_abort(process);
        if ((process->flags & Pkilled) && (native_spawn ? process->state == Pready : dvm_runqueue_head() == process))
        {
            owner_error()[0] = 0;
        }
        int handled = 0;
        if (owner_error()[0] && !dvm_native_execution()->instruction_boundary_error)
        {
            if (!dvm_native_context_set_process(context, process))
            {
                abort();
            }
            handled = handler(owner_error());
            if (!dvm_native_context_set_process(context, nil))
            {
                abort();
            }
        }
        if (handled)
        {
            owner_error()[0] = 0;
            if (!dvm_runqueue_finish(process))
            {
                error("cannot finish handled dispatch");
            }
            dvm_process_environment_release(process);
            up->prog = nil;
            return 1;
        }
        if (owner_error()[0])
        {
            const char *path;
            long pc;
            runtime_error_location(&path, &pc);
            fprintf(stderr, "Dis process %d module %s pc %ld: %s\n", process->pid, path, pc, owner_error());
            if (!dvm_runqueue_finish(process))
            {
                error("cannot finish failed dispatch");
            }
            dvm_process_environment_release(process);
            inspect_release(); /* [GC:] The per-process guard terminates here without reaching the outer guard. */
            up->prog = nil;
            return -1;
        }
        if (dvm_runqueue_owner(process) || ((DVM_RuntimeProcess *)process)->channel_claimed ||
            !dvm_runqueue_begin_retire(process))
        {
            error("cannot retire a process with a live native or channel claim");
        }
        if (native_spawn)
        {
            __atomic_fetch_add(&resident_retirements, 1, __ATOMIC_RELEASE);
        }
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish exiting dispatch");
        }
        dvm_process_environment_release(process);
        delrun(Pexiting);
        if (!dvm_runtime_process_root_remove(process))
        {
            error("retiring process lost its GC root");
        }
        /* Withdraw this root set before running any frame or module destructor. */
        dvm_native_context_set_process(context, nil);
        destroystack(&process->R);
        destroy(process->R.M);
        up->prog = nil;
        destroy(process->exval);
        free(process->exstr);
        {
            DVM_RuntimeProcess *retired = (DVM_RuntimeProcess *)process;
            int parent_pid = retired->exit_notify_parent ? retired->parent_pid : 0;
            int child_pid = process->pid;
            if (process == initial_guest)
            {
                initial_guest = NULL;
            }
            if (native_spawn)
            {
                dvm_managed_workers_guest_retired(child_pid);
            }
            drop_guest_messages(process);
            free(process);
            __atomic_fetch_sub(&live, 1, __ATOMIC_RELAXED);
            publish_child_exit(parent_pid, child_pid);
            if (native_spawn)
            {
                __atomic_fetch_sub(&resident_retirements, 1, __ATOMIC_RELEASE);
                dvm_managed_workers_wake();
            }
        }
    }
    else
    {
        if (process->flags & Pkilled)
        {
            /* A resident with live registers already owns its unwind state. */
            if (!dvm_native_execution()->resident_registers_valid)
            {
                DVM_R = process->R;
            }
            dvm_runtime_termination_checkpoint(process);
        }
        xec(process);
        dvm_runtime_termination_checkpoint(process);
        /* Child-wait replies and cancellations are applied by the TLS owner. */
        dvm_native_execution()->resident_registers_valid =
            native_spawn &&
            (dvm_managed_workers_guest() == process ||
             (process == initial_guest && process->state == DVM_CHILD_EXIT_WAIT)) &&
            (process->state == Pready || hosted_channel_waiter(process) ||
             process->state == DVM_CHILD_EXIT_WAIT);
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish completed dispatch");
        }
        dvm_process_environment_release(process);
        dvm_native_context_set_process(context, nil);
        up->prog = nil;
        poperror();
        if ((process->flags & Pkilled) && (native_spawn ? process->state != Pready : dvm_runqueue_head() != process))
        {
            wake_for_termination(process);
        }
        if (!native_spawn && dvm_runqueue_head() == process)
        {
            if (!dvm_runqueue_pop_expected(process, Pready))
            {
                error("invalid runnable process rotation");
            }
            addrun(process);
        }
    }
    if (dvm_runtime_process_roots() && gc_worker && !dvm_runtime_gc_request())
    {
        error("native collector failed");
    }
    if (dvm_runtime_process_roots() && !gc_worker)
    {
        rungc(dvm_runtime_process_roots());
    }
    return 1;
}


/* Execute the complete Dis runtime exclusively on its bound native owner.
 * argument is a borrowed module path valid until join/return. No UI heap access
 * occurs here in threaded mode. Channel movers stay on this mutator; optional
 * native GC obtains exclusive heap access through the world rendezvous. */
static int runtime_execute_inner(void *argument)
{
    int quiescent_reported = 0; /* One diagnostic per all-channel-wait episode. */
    DVM_NativeContext *context;
    Module *module;
    Prog *process;
    char *entry;
    context = dvm_native_process_create(nil);
    if (!context || !dvm_native_context_bind(context))
    {
        return 1;
    }
    if (pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL))
    {
        fprintf(stderr, "cannot initialize cooperative VM cancellation discipline\n");
        return 1;
    }
    up->env = &up->defenv;
    up->env->errstr = up->env->errbuf1;
    up->env->syserrstr = up->env->errbuf0;
    if (waserror())
    {
        inspect_release();
        fprintf(stderr, "disvm: %s\n", owner_error());
        return 1;
    }
    arena_init();
    opinit();
    sysmodinit();
    dvm_filesystem_initialize();
    loadermodinit();
    mathmodinit();
    dvm_bigtable_initialize();
    dvm_processctl_initialize();
#ifdef DVM_RUNTIME_COMPILER
    dvm_compiler_initialize();
#endif
#ifdef DVM_RUNTIME_NATIVE_DEMO
    dvm_nativedemo_initialize();
#endif
    excinit();
    dvm_srv_initialize();
    srvmodinit();
    entry = entry_resource((char *)argument);
#ifdef DVM_RUNTIME_EDITOR
    if (!dvm_editor_start(module_directory))
    {
        dvm_editor_stop();
        error("cannot initialize graphical editor");
    }
#endif
    module = load(entry);
    if (!module)
    {
        error(owner_error());
    }
    if (native_spawn)
    {
        guest_messages = dvm_messages_create();
        if (!guest_messages)
        {
            error(exNomem);
        }
        dvm_runqueue_enable_direct();
    }
    process = start_module(module);
    initial_guest = process;
    if (inspect_requested)
    {
        inspect_retain(process);
    }
    if (gc_worker && !dvm_runtime_gc_start(gc_locked_poll))
    {
        error("cannot start native collector");
    }
    if ((native_spawn || managed_workers != 1) && !dvm_managed_workers_start(
        native_spawn ? 1 : (managed_workers ? managed_workers : 2), !native_spawn && managed_workers == 0,
        managed_workers_limit, show_stats, runtime_dispatch_turn,
        dvm_runtime_cancel_native_waits))
    {
        error("cannot start managed executor");
    }
    if (gc_worker && !dvm_close_worker_start())
    {
        error("cannot start native close worker");
    }
    fvm_port_sync_lock(channel_queue_mutex);
    cancel_requests_open = gc_worker && !native_waits_stopping;
    fvm_port_sync_unlock(channel_queue_mutex);
    while (dvm_runqueue_ready() || (native_spawn && dvm_runqueue_dispatch_pending()) ||
           ((native_spawn || managed_workers != 1) && dvm_runqueue_inflight()) || (managed_workers == 0 && __atomic_load_n(&live, __ATOMIC_RELAXED))
#ifdef DVM_RUNTIME_EDITOR
           || (__atomic_load_n(&live, __ATOMIC_RELAXED) && dvm_editor_attached())
#endif
           || guest_cancel_pending_or_close())
    {
        if (managed_workers == 0 && __atomic_load_n(&live, __ATOMIC_RELAXED) && !dvm_runqueue_head() && !dvm_runqueue_inflight()
            && !guest_cancel_pending()
#ifdef DVM_RUNTIME_EDITOR
            && !dvm_editor_attached()
#endif
        )
        {
            if (show_stats && !quiescent_reported)
            {
                fprintf(stderr, "NATIVEPOOL_QUIESCENT processes=%d\n", __atomic_load_n(&live, __ATOMIC_RELAXED));
            }
            quiescent_reported = 1;
            if (!dvm_managed_workers_park(guest_cancel_pending))
            {
                error("automatic executor park failed");
            }
            continue;
        }
        quiescent_reported = 0;
        if (gc_worker && !dvm_native_context_poll())
        {
            error("native collector safepoint failed");
        }
#ifdef DVM_RUNTIME_EDITOR
        if (dvm_editor_attached())
        {
            int progress = dvm_editor_poll();
            if (progress < 0)
            {
                error("graphical editor transport/presentation failure");
            }
            if (progress)
            {
                dvm_runtime_input_progress();
            }
        }
        if (!native_spawn && managed_workers == 1 && !dvm_runqueue_head() && !guest_cancel_pending())
        {
            fvm_port_thread_sleep_us(1000);
            continue;
        }
#endif
        {
            int progress = runtime_dispatch_turn();
            if (progress < 0)
            {
                poperror(); /* The dispatch retired its own guard; do not retain this returning outer jump frame. */
                return 1;
            }
            if (progress == DVM_MANAGED_TURN_GUEST)
            {
                dvm_managed_workers_record();
            }
#ifndef DVM_RUNTIME_EDITOR
            if (native_spawn && progress == DVM_MANAGED_TURN_IDLE &&
                (dvm_runqueue_ready() || dvm_runqueue_inflight() || dvm_runqueue_dispatch_pending()))
            {
                if (!dvm_managed_workers_park(guest_cancel_pending))
                {
                    error("dedicated guest wait failed");
                }
                continue;
            }
#endif
        }
        if (!dvm_managed_workers_yield(!native_spawn && !dvm_runqueue_head() && !guest_cancel_pending()))
        {
            error("managed scheduler worker failed");
        }
    }
    if (__atomic_load_n(&live, __ATOMIC_RELAXED))
    {
        error("deadlock: processes remain blocked on channels");
    }
#ifdef DVM_RUNTIME_EDITOR
    dvm_editor_stop();
#endif
    poperror();
    if (inspect_requested && !inspect_report())
    {
        fprintf(stderr, "disvm: inspection output failed\n");
        return 1;
    }
    return 0;
}


/* Join the collector on every managed normal/error return, before the editor
 * launcher retires mutator TLS. argument is borrowed through this call. */
static int runtime_execute(void *argument)
{
    int result = runtime_execute_inner(argument);
    dvm_runtime_cancel_native_waits();
    if (!dvm_managed_workers_stop(show_stats))
    {
        result = 1;
    }
    if (stdio_gates[1] == stdio_gates[2])
    {
        stdio_gates[2] = NULL;
    }
    for (int descriptor = 0; descriptor < 3; descriptor++)
    {
        if (!dvm_file_gate_destroy(stdio_gates[descriptor]))
        {
            fprintf(stderr, "disvm: standard pipe gate retains a native operation\n");
            result = 1;
        }
        else
        {
            stdio_gates[descriptor] = NULL;
        }
    }
    if (!dvm_runtime_gc_stop(show_stats))
    {
        result = 1;
    }
    dvm_close_worker_stop();
    if (guest_messages)
    {
        for (Prog *guest = dvm_runtime_process_roots(); guest; guest = guest->next)
        {
            drop_guest_messages(guest);
        }
        dvm_messages_destroy(guest_messages);
        guest_messages = NULL;
    }
    runtime_statistics();
    dvm_queue_probe_finish();
    if (!result)
    {
        puts("PASS Dis application completed");
    }
    return result;
}


/* Parse options on main before publishing immutable launch arguments to an OS worker. */
int main(int argc, char **argv)
{
    struct sigaction pipe_action = {0};
    int gc_selected = 0;
#ifdef DVM_RUNTIME_EDITOR
    int threaded = 1;
    int threading_selected = 0;
#endif
    /* This executable handles broken pipes as I/O errors on every worker,
     * including a reader disappearing between POLLOUT and splice. */
    pipe_action.sa_handler = SIG_IGN;
    sigemptyset(&pipe_action.sa_mask);
    if (sigaction(SIGPIPE, &pipe_action, NULL))
    {
        perror("disvm: cannot configure broken-pipe handling");
        return 1;
    }
    /* Native pipe writes bypass stdio buffering, so keep host diagnostics
     * in stream order with guest output. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);
    cflag = 1;
    while (argc > 1 && argv[1][0] == '-')
    {
#ifdef DVM_RUNTIME_EDITOR
        if (!strcmp(argv[1],"--threaded-ui") || !strcmp(argv[1],"--sequential-ui"))
        {
            if (threading_selected)
            {
                fprintf(stderr,"Select UI execution mode only once\n");
                return 2;
            }
            threaded = !strcmp(argv[1],"--threaded-ui");
            threading_selected = 1;
            argv++;
            argc--;
            continue;
        }
        if (argc > 2 && dvm_editor_option(argv[1], argv[2]))
        {
            argv += 2;
            argc -= 2;
            continue;
        }
#endif
        if (strcmp(argv[1], "--stats") == 0)
        {
            show_stats = 1;
        }
        else if (strcmp(argv[1], "--max-instructions") == 0)
        {
            size_t limit;
            if (dvm_runtime_instruction_limit || argc < 3 || !runtime_extent(argv[2], &limit) || !limit)
            {
                fprintf(stderr, "--max-instructions requires one decimal value in 1..2147483647\n");
                return 2;
            }
            dvm_runtime_instruction_limit = (unsigned long)limit;
            argv += 2;
            argc -= 2;
            continue;
        }
        else if (strcmp(argv[1], "--quantum") == 0)
        {
            size_t quantum;
            if (quantum_selected || argc < 3 || !runtime_extent(argv[2], &quantum) || !quantum || quantum > 65536)
            {
                fprintf(stderr, "--quantum requires one value in 1..65536 and cannot be repeated\n");
                return 2;
            }
            scheduler_quantum = (int)quantum;
            quantum_selected = 1;
            argv += 2;
            argc -= 2;
            continue;
        }
        else if (strcmp(argv[1], "--inspect-words") == 0 || strcmp(argv[1], "--inspect-reals") == 0)
        {
            if (inspect_requested || argc < 4 || !runtime_extent(argv[2], &inspect_offset) ||
                !runtime_extent(argv[3], &inspect_count) || inspect_count < 1 || inspect_count > 4096 ||
                inspect_offset % 8U != 0)
            {
                fprintf(
                    stderr,
                    "%s requires one aligned decimal byte offset and 1..4096 cells; inspection options cannot be combined\n",
                    argv[1]);
                return 2;
            }
            inspect_reals = strcmp(argv[1], "--inspect-reals") == 0;
            inspect_requested = 1;
            argv += 3;
            argc -= 3;
            continue;
        }
        else if (strcmp(argv[1], "--test-control") == 0)
        {
            test_control = 1;
        }
        else if (strcmp(argv[1], "--gc-locked-poll") == 0)
        {
            gc_locked_poll = 1;
        }
        else if (strcmp(argv[1], "--managed-owner-probe") == 0)
        {
            dvm_managed_owner_probe_requested = 1;
        }
        else if (strcmp(argv[1], "--native-queue-probe") == 0)
        {
            if (!dvm_queue_probe_enable())
            {
                fprintf(stderr,"cannot initialize native queue probe\n");
                return 2;
            }
        }
        else if (strcmp(argv[1], "--native-spawn") == 0)
        {
            native_spawn = 1;
        }
        else if (strcmp(argv[1], "--managed-workers") == 0)
        {
            size_t count = 0;
            if (managed_workers_selected || argc < 3 ||
                (strcmp(argv[2], "auto") && (!runtime_extent(argv[2], &count) || count < 1)))
            {
                fprintf(stderr, "--managed-workers requires one positive count or auto\n");
                return 2;
            }
            managed_workers = (int)count;
            managed_workers_selected = 1;
            argv++;
            argc--;
        }
        else if (strcmp(argv[1], "--managed-workers-limit") == 0)
        {
            size_t limit;
            if (managed_limit_selected || argc < 3 || !runtime_extent(argv[2], &limit) || limit < 2)
            {
                fprintf(stderr, "--managed-workers-limit requires one count >=2\n");
                return 2;
            }
            managed_workers_limit = (unsigned long)limit;
            managed_limit_selected = 1;
            argv++;
            argc--;
        }
        else if (strcmp(argv[1], "--guarded-copies") == 0)
        {
            if (dvm_runtime_guarded_copies)
            {
                fprintf(stderr, "--guarded-copies may be selected only once\n");
                return 2;
            }
            dvm_runtime_guarded_copies = 1;
        }
        else if (strcmp(argv[1], "--parallel-add") == 0 || strcmp(argv[1], "--parallel-integer") == 0)
        {
            if (dvm_runtime_parallel_add)
            {
                fprintf(stderr, "parallel arithmetic mode may be selected only once\n");
                return 2;
            }
            dvm_runtime_parallel_add = strcmp(argv[1], "--parallel-add") == 0 ? 1 : 2;
        }
        else if (strcmp(argv[1], "--parallel-real") == 0)
        {
            dvm_runtime_parallel_real = 1;
        }
        else if (strcmp(argv[1], "--parallel-movp") == 0)
        {
            dvm_runtime_parallel_movp = 1;
        }
        else if (strcmp(argv[1], "--parallel-byte-copies") == 0)
        {
            dvm_runtime_parallel_byte_copies = 1;
        }
        else if (strcmp(argv[1], "--parallel-channel-buffer") == 0)
        {
            dvm_runtime_parallel_channel_buffer = 1;
        }
        else if (strcmp(argv[1], "--parallel-movmp") == 0)
        {
            dvm_runtime_parallel_movmp = 1;
        }
        else if (strcmp(argv[1], "--parallel-slicela") == 0)
        {
            dvm_runtime_parallel_slicela = 1;
        }
        else if (strcmp(argv[1], "--parallel-string") == 0)
        {
            dvm_runtime_parallel_string = 1;
        }
        else if (strcmp(argv[1], "--gc-idle-cycles") == 0)
        {
            dvm_runtime_idle_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-no-cycles") == 0)
        {
            dvm_runtime_disable_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-event-cycles") == 0)
        {
            dvm_runtime_event_cycles = 1;
        }
        else if (strcmp(argv[1], "--gc-worker") == 0)
        {
            gc_worker = 1;
            gc_selected = 1;
        }
        else if (strcmp(argv[1], "--gc-inline") == 0)
        {
            gc_worker = 0;
            gc_selected = 1;
        }
        else if (strcmp(argv[1], "--no-jit") == 0)
        {
            cflag = 0;
        }
        else if (strcmp(argv[1], "--jit") == 0)
        {
            cflag = 1;
        }
        else if (strcmp(argv[1], "--version") == 0)
        {
            puts("DisVM runtime " DVM_RUNTIME_VERSION ": experimental NC64, magic 819264, Linux " DVM_RUNTIME_BACKEND);
            return 0;
        }
        else if (strcmp(argv[1], "--help") == 0)
        {
            puts(
                "usage: disvm [--jit|--no-jit] [--stats] [--quantum COUNT] [--max-instructions COUNT] [--test-control] [--inspect-words OFFSET COUNT | --inspect-reals OFFSET COUNT] native-cell.dis");
#ifdef DVM_RUNTIME_EDITOR
            puts(
                "editor options: [--font TTF] [--line-numbers 0|1] (bundled Noto Sans Mono, default 11) [--document UTF8 | --new PATH] [--events FRAMES | --window-events FRAMES | --clipboard-events FRAMES] [--capture PPM]");
#endif
            puts("GC execution: --gc-worker (native collectors), --gc-inline (single-worker reference), --gc-locked-poll (mutex reference polling)");
#if defined(__x86_64__) && !defined(DVM_RUNTIME_EDITOR)
            puts("Default console profile: one native thread per spawn, with native GC.");
#endif
            puts("Explicit GC and guarded/parallel switches select a custom profile.");
            puts("--native-spawn starts one dedicated OS thread per spawned guest with a guarded parallel profile where supported.");
            puts("--gc-idle-cycles runs cycle collection only when no Dis heap owners are active.");
            puts("--gc-no-cycles keeps Fast GC active and disables cycle collection. Unreachable cycles can accumulate.");
            puts("--gc-event-cycles coalesces cycle scans after managed owner loss.");
            puts("Diagnostic: --managed-owner-probe with --gc-worker checks managed roots across native owners");
            puts("Diagnostic: --native-queue-probe with --gc-worker checks native wait withdrawal on an extra OS owner");
            puts("Experimental NC64 cells; legacy .dis modules are rejected. Sys subset; unaudited heap operations use exclusive turns.");
            puts("Managed execution: --managed-workers 1 or a positive count / auto (multiple workers require --gc-worker); --managed-workers-limit caps auto.");
#ifdef DVM_RUNTIME_EDITOR
            puts("UI execution: --threaded-ui (default: UI main + Dis OS worker), --sequential-ui (reference)");
#endif
            return 0;
        }
        else
        {
            fprintf(stderr, "unknown runtime option: %s\n", argv[1]);
            return 2;
        }
        argv++;
        argc--;
    }
    if (argc != 2)
    {
        fprintf(stderr, "usage: disvm [--jit|--no-jit] native-cell.dis\n");
        return 2;
    }
#if defined(__x86_64__) && !defined(DVM_RUNTIME_EDITOR)
    /* The console defaults to dedicated native guests. Explicit diagnostic
     * profiles still select their own runtime settings. */
    if (!native_spawn && !gc_selected && (!managed_workers_selected || managed_workers != 1) &&
        !dvm_runtime_guarded_copies && !dvm_runtime_parallel_add &&
        !dvm_runtime_parallel_real && !dvm_runtime_parallel_string &&
        !dvm_runtime_parallel_movp && !dvm_runtime_parallel_movmp &&
        !dvm_runtime_parallel_slicela && !dvm_runtime_parallel_channel_buffer &&
        !dvm_runtime_parallel_byte_copies && !dvm_managed_owner_probe_requested &&
        !dvm_queue_probe_requested)
    {
        if (!managed_workers_selected)
        {
            native_spawn = 1;
        }
        else
        {
            gc_worker = 1;
            dvm_runtime_guarded_copies = 1;
            dvm_runtime_parallel_movp = 1;
            dvm_runtime_parallel_movmp = 1;
            dvm_runtime_parallel_slicela = 1;
            dvm_runtime_parallel_channel_buffer = 1;
            dvm_runtime_parallel_byte_copies = 1;
        }
    }
#else
    USED(gc_selected);
#endif
    if (native_spawn)
    {
        if (managed_workers_selected || managed_limit_selected || (gc_selected && !gc_worker))
        {
            fprintf(stderr, "--native-spawn requires native GC and selects its own executor lifecycle\n");
            return 2;
        }
        gc_worker = 1;
        if (
#if !defined(__x86_64__)
            !cflag &&
#endif
            !dvm_runtime_guarded_copies && !dvm_runtime_parallel_add &&
            !dvm_runtime_parallel_real && !dvm_runtime_parallel_string &&
            !dvm_runtime_parallel_movp && !dvm_runtime_parallel_movmp &&
            !dvm_runtime_parallel_slicela && !dvm_runtime_parallel_channel_buffer &&
            !dvm_runtime_parallel_byte_copies)
        {
            dvm_runtime_guarded_copies = 1;
            dvm_runtime_parallel_add = 2;
            dvm_runtime_parallel_real = 1;
            dvm_runtime_parallel_string = 1;
            dvm_runtime_parallel_movp = 1;
            dvm_runtime_parallel_movmp = 1;
            dvm_runtime_parallel_slicela = 1;
            dvm_runtime_parallel_channel_buffer = 1;
            dvm_runtime_parallel_byte_copies = 1;
        }
        dvm_runtime_shared_integer = dvm_runtime_parallel_add != 0;
        dvm_runtime_guest_unbounded = !quantum_selected;
    }
    if (managed_limit_selected && managed_workers != 0)
    {
        fprintf(stderr, "--managed-workers-limit requires --managed-workers auto\n");
        return 2;
    }
#if !defined(__x86_64__)
    if (dvm_runtime_guarded_copies && cflag)
    {
        fprintf(stderr, "--guarded-copies with --jit requires AMD64; use --no-jit on this target\n");
        return 2;
    }
#endif
    if ((dvm_runtime_parallel_add || dvm_runtime_parallel_real || dvm_runtime_parallel_string) &&
        (!dvm_runtime_guarded_copies || (managed_workers == 1 && !native_spawn)))
    {
        fprintf(stderr, "parallel scalar modes require --guarded-copies and multiple managed workers\n");
        return 2;
    }
    if (dvm_runtime_parallel_movp && (!dvm_runtime_guarded_copies || (managed_workers == 1 && !native_spawn)))
    {
        fprintf(stderr, "--parallel-movp requires --guarded-copies and multiple managed workers\n");
        return 2;
    }
    if ((dvm_runtime_parallel_slicela || dvm_runtime_parallel_movmp || dvm_runtime_parallel_channel_buffer ||
         dvm_runtime_parallel_byte_copies) && (!dvm_runtime_guarded_copies || (managed_workers == 1 && !native_spawn)))
    {
        fprintf(stderr, "parallel mapped copies require --guarded-copies and multiple managed workers\n");
        return 2;
    }
    /* The implicit native profile never asks active guests to park for Cycles.
     * Explicit --gc-worker retains the stopped reference path for diagnostics. */
    if (gc_worker && !gc_selected && !dvm_runtime_disable_cycles)
    {
        dvm_runtime_idle_cycles = 1;
        dvm_runtime_event_cycles = 1;
    }
    if (dvm_runtime_idle_cycles && (!gc_worker || dvm_runtime_disable_cycles))
    {
        fprintf(stderr, "--gc-idle-cycles requires native GC and conflicts with --gc-no-cycles\n");
        return 1;
    }
    if (dvm_runtime_disable_cycles && (!gc_worker || dvm_runtime_event_cycles))
    {
        fprintf(stderr, "--gc-no-cycles requires native GC and conflicts with --gc-event-cycles\n");
        return 1;
    }
    if (dvm_runtime_event_cycles && !gc_worker)
    {
        fprintf(stderr, "--gc-event-cycles requires native GC\n");
        return 2;
    }
    if (dvm_runtime_guarded_copies && !gc_worker)
    {
        fprintf(stderr, "--guarded-copies requires --gc-worker\n");
        return 2;
    }
    if (managed_workers != 1 && !gc_worker)
    {
        fprintf(stderr,"--managed-workers >1 requires --gc-worker\n");
        return 2;
    }
#ifdef DVM_RUNTIME_EDITOR
    if ((native_spawn || managed_workers != 1) && !threaded)
    {
        fprintf(stderr,"--managed-workers >1 requires --threaded-ui for the graphical host\n");
        return 2;
    }
#endif
    if (dvm_managed_owner_probe_requested && !gc_worker)
    {
        fprintf(stderr, "--managed-owner-probe requires --gc-worker\n");
        return 2;
    }
    if (dvm_queue_probe_requested && !gc_worker)
    {
        fprintf(stderr,"--native-queue-probe requires --gc-worker\n");
        dvm_queue_probe_finish();
        return 2;
    }
    if ((native_spawn || managed_workers != 1) && show_stats)
    {
        dvm_runqueue_trace_waits();
    }
    dontcompile = !cflag;
    if (sizeof(WORD) != 8 || sizeof(void *) != 8 || IBY2WD != 8 || offsetof(Alt, ac) != 16 ||
        offsetof(F_Sys_print, s) != 64)
    {
        fprintf(stderr, "native-cell compiler/runtime frame contract mismatch\n");
        return 2;
    }
#ifdef DVM_RUNTIME_EDITOR
    if (threaded)
    {
        return dvm_editor_thread_launch(runtime_execute,argv[1]);
    }
#endif
    return runtime_execute(argv[1]);
}

/* Preserve formatted VM errors before unwinding their current owner. */
void errorf(char *format, ...)
{
    char text[256];
    va_list arguments;
    va_start(arguments, format);
    vsnprint(text, sizeof(text), format, arguments);
    va_end(arguments);
    error(text);
}

/* Dynamic native code is excluded; ordinary Dis never enters this boundary. */
Module *newdyncode(int fd, char *path, Dir *metadata)
{
    USED(fd);
    USED(path);
    USED(metadata);
    error("native library loading unsupported");
    return nil;
}

/* Reject native data attachment rather than manufacturing a successful import. */
void newdyndata(Modlink *module)
{
    USED(module);
    error("native data loading unsupported");
}

/* There are no native dynamic-code allocations in this host. */
void freedyncode(Module *module)
{
    USED(module);
}

/* There are no native dynamic-data allocations in this host. */
void freedyndata(Modlink *module)
{
    USED(module);
}

/* Ask the host allocator about metadata storage; VM blocks use poolmsize instead. */
ulong msize(void *value)
{
    extern size_t malloc_usable_size(void *);
    return malloc_usable_size(value);
}

/* Validate language path contents before converting to a native NUL-terminated name.
 * name is borrowed; returns the UTF-8 path or NULL with a visible error. */
char *dvm_runtime_file_path(String *name)
{
    int index;
    if (name == H || name == nil)
    {
        error(exNilref);
    }
    if (name->len == (-0x7fffffff - 1))
    {
        error(exBounds);
    }
    if (name->len >= 0)
    {
        if (memchr(name->Sascii, 0, name->len))
        {
            kwerrstr("file paths cannot contain NUL");
            return nil;
        }
    }
    else
    {
        for (index = 0; index < -name->len; index++)
        {
            if (name->Srune[index] == 0)
            {
                kwerrstr("file paths cannot contain NUL");
                return nil;
            }
        }
    }
    return string2c(name);
}

/* Open ordinary read-only input, or the explicitly enabled isolated-test control node. */
void Sys_open(void *frame)
{
    F_Sys_open local;
    F_Sys_open *call = &local;
    Sys_FD *old;
    Sys_FD *result;
    int descriptor;
    char *path;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_open, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    path = dvm_runtime_file_path(call->s);
    if (!path)
    {
        Sys_FD *empty = H;
        memmove(call->ret, &empty, sizeof(empty));
        destroy(old);
        return;
    }
    if (call->s == H)
    {
        error(exNilref);
    }
    result = H2D(Sys_FD *, heapz(TFD));
    result->fd = -1;
    if (test_control && strcmp(path, "/dev/sysctl") == 0 && call->mode == 1)
    {
        descriptor = -2;
    }
    else
    {
        descriptor = -1;
        if (call->mode == OREAD)
        {
            /* [GC:] The unpublished FD is a C-held root across native waiting. */
            ptradd(D2H(result));
            Setmark(D2H(result));
            descriptor = open_file_mode(path, "r", 1);
            ptrdel(D2H(result));
        }
        else
        {
            kwerrstr("unsupported file open mode");
        }
    }
    if (descriptor >= 0 || descriptor == -2)
    {
        result->fd = descriptor;
    }
    else
    {
        destroy(result);
        result = H;
    }
    memmove(call->ret, &result, sizeof(result));
    destroy(old);
}

/* Write to actual native files/standard output, or consume the explicit test halt capability. */
void Sys_write(void *frame)
{
    F_Sys_write local;
    F_Sys_write *call = &local;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_write, ret), sizeof(*call->ret));
    if (test_control && call->fd != H && call->fd != nil && call->fd->fd == -2 && call->buf != H && call->buf != nil &&
        call->n == 4 && call->buf->len >= 4 && memcmp(call->buf->data, "halt", 4) == 0)
    {
        fflush(stdout);
        inspect_release(); /* [GC:] Explicit test halt also retires any captured scalar data before exit. */
        runtime_statistics();
        exit(0);
    }
    {
        WORD result = dvm_runtime_write(call->fd,call->buf,call->n);
        memmove(call->ret,&result,sizeof(result));
    }
}

/* Share the NC64 leaf policy with complete JIT tails and its standalone check. */
#include "native_leaf.c"

/* Publish a new descriptor while preserving the previous result owner until construction completes. */
static void publish_descriptor(void *slot, Sys_FD *result, Sys_FD *old)
{
    memmove(slot, &result, sizeof(result));
    destroy(old);
}

/* Borrow standard descriptors 0/1/2 without taking ownership of their OS handles. */
void Sys_fildes(void *frame)
{
    F_Sys_fildes local;
    F_Sys_fildes *call = &local;
    Sys_FD *result = H;
    Sys_FD *old;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_fildes, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    if (call->fd >= 0 && call->fd <= 2)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = call->fd;
    }
    else
    {
        kwerrstr("only standard descriptors 0, 1 and 2 can be borrowed");
    }
    publish_descriptor(call->ret, result, old);
}

/* Create/truncate a file only after validating flags and allocating its result owner.
 * The supported permission value is 0666 filtered by the platform umask. */
void Sys_create(void *frame)
{
    F_Sys_create local;
    F_Sys_create *call = &local;
    Sys_FD *result = H;
    Sys_FD *old;
    int descriptor;
    char *path;
    dvm_sys_capture(frame, call, sizeof(*call), offsetof(F_Sys_create, ret), sizeof(*call->ret));
    old = dvm_sys_owner(call->ret);
    path = dvm_runtime_file_path(call->s);
    if (!path)
    {
        publish_descriptor(call->ret, H, old);
        return;
    }
    if (call->s == H || call->s == nil)
    {
        error(exNilref);
    }
    if ((call->mode == Sys_OWRITE || call->mode == (Sys_OWRITE | Sys_OEXCL)) && call->perm == 0666)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = -1;
        /* [GC:] Preserve the unpublished FD while other owners collect. */
        ptradd(D2H(result));
        Setmark(D2H(result));
        descriptor = open_file_mode(path, (call->mode & Sys_OEXCL) ? "wx" : "w", 1);
        ptrdel(D2H(result));
        if (descriptor < 0)
        {
            destroy(result);
            result = H;
        }
        else
        {
            result->fd = descriptor;
        }
    }
    else
    {
        kwerrstr("create supports OWRITE, optional OEXCL and permission 0666 only");
    }
    publish_descriptor(call->ret, result, old);
}

/* Borrow a directory cursor only for the current serial owner call. */
FVM_PortDirectory *dvm_runtime_directory(int descriptor)
{
    return descriptor >= 3 && descriptor < 32 ? directories[descriptor] : nil;
}
