/* Retained Dis interpreter/JIT with explicit native execution and GC owners. */
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
#include "file_port.h"
#include "file_gate.h"
#include "filesystem_sys.h"
#include "metadata_wait.h"
#include "open_wait.h"
#include "close_worker.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "shared_bytes.h"
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
static Prog *processes;                   /* Complete GC root/process list. */
static int nextpid;                       /* Monotonic process identity. */
static int live;                          /* Process quota accounting. */
static int gc_locked_poll;                /* Retained mutex polling for reference comparison. */
static int managed_workers = 1; /* Opt-in OS executors; shared heap remains exclusive. */
static unsigned long managed_workers_limit; /* Optional automatic pool resource cap; zero is uncapped. */
static int managed_limit_selected; /* Reject duplicate physical resource limits. */
static int managed_workers_selected; /* Reject duplicate executor options. */
static unsigned long progress_epoch; /* Shared acknowledged input generation, protected by the heap gate. */
static int gc_worker;                     /* Explicit production collector-worker admission. */
static char *module_directory;            /* Read-only module resource base; ordinary Sys paths retain caller cwd. */
extern int dontcompile;                   /* Production loader switch that prohibits forced JIT compilation. */
int cflag;                                /* Native leaf compilation switch consumed by the loader. */
int keepbroken;                           /* No debugger retention is implemented in this initial runner. */
Type *TFD;                                /* Descriptor owner for supported module/control input handles. */
Type *TFileIO;                            /* Unavailable file-to-channel host type. */
unsigned long dvm_runtime_instructions;   /* Dispatched instruction attempts across every Limbo process. */
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
                dvm_runtime_instructions,
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
    if (dvm_runtime_instruction_limit && dvm_runtime_instructions >= dvm_runtime_instruction_limit)
    {
        dvm_native_execution()->instruction_boundary_error = 1;
        error("instruction limit exceeded");
    }
    dvm_runtime_instructions++;
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

static FVM_PortSync *channel_queue_mutex; /* Lives for the hosted runtime. */


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


/* Create arena/mutex privately; VM allocation errors occur outside pool lock. */
static void arena_init(void)
{
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


/* Remove current FIFO head without touching its separate process/GC root links.
 * state is the retained scheduler reason; invalid/empty head raises normally. */
Prog *delrun(int state)
{
    Prog *process = dvm_runqueue_pop(state);
    if (!process)
    {
        error("invalid runnable process removal");
    }
    return process;
}

/* Create one Limbo process; it is scheduled above one bound native OS owner. */
Prog *newprog(Prog *parent, Modlink *module)
{
    Prog *process;
    if (live == PROCESS_MAX || nextpid == INT_MAX)
    {
        error("process quota exceeded");
    }
    process = calloc(1, sizeof(DVM_RuntimeProcess));
    if (!process)
    {
        error(exNomem);
    }
    dvm_process_environment_init(process,parent);
    process->pid = ++nextpid;
    process->quanta = scheduler_quantum;
    process->xec = xec;
    process->exval = H;
    process->R.M = module;
    process->R.MP = module->MP;
    if (dvm_refcount_retain_u(&D2H(module)->ref) != DVM_REFCOUNT_LIVE)
    {
        free(process);
        error(exHeap);
    }
    process->next = processes;
    processes = process;
    live++;
    addrun(process);
    return process;
}

/* Reserve a live waiter before dequeue. NativeWorld keeps the Prog live
 * and excludes other executors until the claim is released. */
void dvm_runtime_channel_with_claim(Prog *peer, int (*match)(void *), void *request)
{
    DVM_RuntimeProcess *process = (DVM_RuntimeProcess *)peer;
    int completed;
    if (process->channel_claimed ||
        (peer->state != Psend && peer->state != Precv && peer->state != Palt))
    {
        error("channel waiter cannot be claimed");
    }
    if (!dvm_runqueue_pin(peer))
    {
        error("channel waiter cannot be pinned");
    }
    process->channel_claimed = &completed;
    completed = kprotect(match, request);
    process->channel_claimed = nil;
    dvm_runqueue_unpin(peer);
    if (!completed)
    {
        krethrow();
    }
}


/* Complete one detached wait under NativeWorld. Scheduler admission is the
 * state transition. Refusal leaves wait fields and any pending kill intact. */
void dvm_runtime_channel_wake(Prog *peer, char *cause)
{
    if (peer->state != Psend && peer->state != Precv && peer->state != Palt)
    {
        error("invalid channel waiter state");
    }
    addrun(peer);
    peer->ptr = nil;
    peer->chan = nil;
    if (cause)
    {
        peer->kill = cause;
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
 * but does not fail the sender if its payload was already transferred. */
void dvm_runtime_channel_finish_refill(Channel *channel, Prog *peer,
    int (*refill)(void *), void *request)
{
    char first_error[ERRMAX];
    int transferred = kprotect(refill, request);
    int released;
    if (!transferred)
    {
        memmove(first_error, owner_error(), sizeof(first_error));
    }
    released = kprotect(release_refill_channel, channel);
    if (transferred)
    {
        dvm_runtime_channel_wake(peer, nil);
    }
    else
    {
        dvm_runtime_channel_wake(peer, "channel transfer failed");
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
                process->channel_claimed = nil;
                dvm_runqueue_unpin(peer);
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


/* Detach under NativeWorld before a wake can cancel other registrations. */
void killcomm(Progq **queue)
{
    Progq *entry = *queue;
    char first_error[ERRMAX];
    int failed = 0;
    if (!entry)
    {
        return;
    }
    dvm_runtime_channel_close_preflight(queue);
    check_close_claims(entry);
    claim_close_waiters(entry, &failed);
    *queue = nil;
    drain_closed_queue(entry, &failed, first_error, &failed);
    if (failed)
    {
        error(first_error);
    }
}


/* Finalize both queue owners together. All checks precede detachment and
 * descriptor release. NativeWorld keeps the registered processes live. */
void dvm_runtime_channel_finalize(Channel *channel, int swept)
{
    Progq *send = channel->send;
    Progq *recv = channel->recv;
    Progq *send_tail = send ? dvm_runtime_channel_close_preflight(&channel->send) : nil;
    Progq *recv_tail = recv ? dvm_runtime_channel_close_preflight(&channel->recv) : nil;
    Array *buffer = channel->buf;
    Type *mapped = channel->mover == movtmp ? channel->mid.t : nil;
    char first_error[ERRMAX];
    int failed = 0;
    if (send_tail && send_tail == recv_tail)
    {
        error(exHeap);
    }
    check_close_claims(send);
    check_close_claims(recv);
    claim_close_waiters(send, &failed);
    claim_close_waiters(recv, &failed);
    channel->send = nil;
    channel->recv = nil;
    channel->mover = nil;
    channel->buf = H;
    drain_closed_queue(send, &failed, first_error, &failed);
    drain_closed_queue(recv, &failed, first_error, &failed);
    if (mapped)
    {
        freetype(mapped);
    }
    if (!swept && buffer != H &&
        !kprotect(release_refill_channel, buffer) && !failed)
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
    size_t ticket;
    FVM_Result status;
    if (fd < 0 || fd >= 32 || !files[fd] || size < 0)
    {
        return -1;
    }
    ticket = dvm_file_gate_reserve(file_gates[fd]);
    if (!dvm_file_gate_enter(file_gates[fd], ticket))
    {
        return -1;
    }
    status = fvm_port_file_read(files[fd], bytes, size, &received);
    if (!dvm_file_gate_leave(file_gates[fd], ticket))
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
    size_t ticket;
    FVM_Result status;
    if (fd < 0 || fd >= 32 || !files[fd])
    {
        return nil;
    }
    ticket = dvm_file_gate_reserve(file_gates[fd]);
    if (!dvm_file_gate_enter(file_gates[fd], ticket))
    {
        return nil;
    }
    status = fvm_port_file_size(files[fd], &available);
    if (!dvm_file_gate_leave(file_gates[fd], ticket))
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
    process = newprog(nil, link);
    process->R = initial.R;
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

/* Execute one complete managed turn under exclusive heap ownership. Both OS
 * executors call the SAME interpreter/JIT/error/retirement logic. Return1 for a
 * completed turn,0 for idle,-1 for a managed failure with its claim retired.
 * Registers and roots are published before returning; no UI pump runs here. */
static int runtime_dispatch_turn(void)
{
    DVM_NativeContext *context = dvm_native_context_current();
    Prog *process;
    Prog **slot;
    if (!dvm_runqueue_head())
    {
        return 0;
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
    process = dvm_runqueue_head();
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
        process->R = DVM_R;
        dvm_native_context_set_process(context, nil);
        dvm_bigtable_abort(process);
        if (owner_error()[0] && !dvm_native_execution()->instruction_boundary_error && handler(owner_error()))
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
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish exiting dispatch");
        }
        dvm_process_environment_release(process);
        delrun(Pexiting);
        for (slot = &processes; *slot && *slot != process; slot = &(*slot)->next)
        {
        }
        if (*slot)
        {
            *slot = process->next;
        }
        destroystack(&process->R);
        destroy(process->R.M);
        dvm_native_context_set_process(context, nil);
        up->prog = nil;
        destroy(process->exval);
        free(process->exstr);
        free(process);
        live--;
    }
    else
    {
        xec(process);
        if (!dvm_runqueue_finish(process))
        {
            error("cannot finish completed dispatch");
        }
        dvm_process_environment_release(process);
        dvm_native_context_set_process(context, nil);
        up->prog = nil;
        poperror();
        if (dvm_runqueue_head() == process)
        {
            addrun(delrun(Pready));
        }
    }
    if (processes && gc_worker && !dvm_runtime_gc_request())
    {
        error("native collector failed");
    }
    if (processes && !gc_worker)
    {
        rungc(processes);
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
    process = start_module(module);
    if (inspect_requested)
    {
        inspect_retain(process);
    }
    if (gc_worker && !dvm_runtime_gc_start(&processes,gc_locked_poll))
    {
        error("cannot start native collector");
    }
    if (managed_workers != 1 && !dvm_managed_workers_start(managed_workers ? managed_workers : 2, managed_workers == 0, managed_workers_limit, show_stats, runtime_dispatch_turn))
    {
        error("cannot start managed executor");
    }
    if (gc_worker && !dvm_close_worker_start())
    {
        error("cannot start native close worker");
    }
    while (dvm_runqueue_head() || (managed_workers != 1 && dvm_runqueue_inflight()) || (managed_workers == 0 && live)
#ifdef DVM_RUNTIME_EDITOR
           || (live && dvm_editor_attached())
#endif
    )
    {
        if (managed_workers == 0 && live && !dvm_runqueue_head() && !dvm_runqueue_inflight()
#ifdef DVM_RUNTIME_EDITOR
            && !dvm_editor_attached()
#endif
        )
        {
            if (show_stats && !quiescent_reported)
            {
                fprintf(stderr, "NATIVEPOOL_QUIESCENT processes=%d\n", live);
            }
            quiescent_reported = 1;
            if (!dvm_managed_workers_park())
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
        if (managed_workers == 1 && !dvm_runqueue_head())
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
            if (progress)
            {
                dvm_managed_workers_record();
            }
        }
        if (!dvm_managed_workers_yield(!dvm_runqueue_head()))
        {
            error("managed scheduler worker failed");
        }
    }
    if (live)
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
    if (!dvm_managed_workers_stop(show_stats))
    {
        result = 1;
    }
    if (!dvm_runtime_gc_stop(show_stats))
    {
        result = 1;
    }
    dvm_close_worker_stop();
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
#ifdef DVM_RUNTIME_EDITOR
    int threaded = 1;
    int threading_selected = 0;
#endif
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
        else if (strcmp(argv[1], "--gc-worker") == 0)
        {
            gc_worker = 1;
        }
        else if (strcmp(argv[1], "--gc-inline") == 0)
        {
            gc_worker = 0;
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
            puts("GC execution: --gc-worker (native collectors), --gc-inline (default reference), --gc-locked-poll (mutex reference polling)");
            puts("Memory experiment: --guarded-copies with --gc-worker guards copies, integer conversions/arithmetic and list projections; parallel heap stays disabled");
            puts("Diagnostic: --managed-owner-probe with --gc-worker checks managed roots across native owners");
            puts("Diagnostic: --native-queue-probe with --gc-worker checks native wait withdrawal on an extra OS owner");
            puts("Experimental NC64 cells; legacy .dis modules are rejected. Sys subset; native heap turns are exclusive.");
            puts("Managed execution: --managed-workers 1 (default) or a positive count / auto (requires --gc-worker); auto grows for native waits and retires idle helpers; --managed-workers-limit optionally caps resources. Heap stays serial.");
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
    if (managed_workers != 1 && !threaded)
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
    if (managed_workers != 1 && show_stats)
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
