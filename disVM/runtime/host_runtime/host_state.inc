/* Copyright (c) 2026 bhgv
 * EN: Host declarations, immutable configuration and initial diagnostics; included once by main.c.
 * RU: Объявления host, неизменная конфигурация и исходная диагностика; однократно подключается из main.c.
 */
/* Copyright (c) 2026 bhgv
 * EN: Native Dis runtime, permanent resident ownership and synchronized shared services.
 * RU: Нативная Dis VM, постоянные владельцы Prog и синхронизированные общие службы.
 */
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
#include "file_slots.h"
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
#include "host_cancellations.h"
#include "channel_match.h"
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
#define PROCESS_MAX DVM_RUNTIME_PROCESS_MAX  /* Maximum simultaneous Limbo processes. */

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
static unsigned int resident_wait_deliveries; /* Mailbox requests being applied by residents. */
static unsigned int resident_retirements;     /* Dedicated exits still publishing completion. */
static int gc_locked_poll;                    /* Retained mutex polling for reference comparison. */
static __thread Prog *initial_guest;          /* This thread's controller fallback; peer residents never borrow it. */
static int native_spawn;                      /* One dedicated OS thread per spawned Prog. */
static int managed_workers_selected;          /* Reject duplicate executor options. */
static unsigned long progress_epoch;          /* Shared acknowledged input generation, protected by the heap gate. */
static int gc_worker;                         /* Collector-worker admission selected at launch. */
static char *module_directory;               /* Read-only module resource base; ordinary Sys paths retain caller cwd. */
extern int dontcompile;                      /* Production loader switch that prohibits forced JIT compilation. */
int cflag;                                   /* Native leaf compilation switch consumed by the loader. */
int keepbroken;                              /* No debugger retention is implemented in this initial runner. */
Type *TFD;                                   /* Descriptor owner for supported module/control input handles. */
Type *TFileIO;                               /* Unavailable file-to-channel host type. */
unsigned long dvm_runtime_instructions;      /* Atomic instruction attempts across every Limbo process. */
static unsigned long native_instructions;    /* Atomic accepted native callbacks, excluding rejected guards. */
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
                __atomic_load_n(&native_instructions, __ATOMIC_RELAXED),
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
    } while (!__atomic_compare_exchange_n(
        &dvm_runtime_instructions, &count, count + 1, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
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
