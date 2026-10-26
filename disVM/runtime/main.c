/* Experimental sequential owner for the retained Dis interpreter and native leaves. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "pool.h"
#include "raise.h"
#include "disvm/native_process.h"
#include "file_port.h"
#include <stdio.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>

#ifdef DVM_RUNTIME_EDITOR
#include "editor_view.h"
#include "thread_port.h"


/* Initialize, poll and release the UI channel boundary on this sole native owner. */
extern int dvm_editor_start(void);
extern int dvm_editor_poll(void);
extern int dvm_editor_attached(void);
extern void dvm_editor_stop(void);
#endif

#define ARENA_BYTES (64UL * 1024UL * 1024UL) /* Bounded experimental heap arena. */
#define PROCESS_MAX 128                      /* Maximum simultaneous Limbo processes. */


/* Private allocator owner; the arena has stable metadata for production GC walks. */
struct Pool
{
    Bhdr *arena; /* Owned bounded arena, including collector block headers. */
};


/* Initialize the resolver's owned native lock. No arguments/result; raises on failure. */
extern void dvm_srv_initialize(void);


/* Register retained Srv exports for this exclusive owner. No arguments or result. */
extern void srvmodinit(void);


/* Register retained Loader exports/types. No arguments/result; requires initialized heap. */
extern void loadermodinit(void);


/* Register hosted Math exports. No arguments/result; requires initialized heap and Sys. */
extern void mathmodinit(void);

/* Register hosted scalar hybrid tables after the VM allocator is initialized. */
extern void dvm_bigtable_initialize(void);

static Pool memory;                       /* Sole pool owned by the bound runtime thread. */
Pool *heapmem = &memory;                  /* Pool consumed by the production RC/cycle collector. */
static Prog *ready;                       /* FIFO head for runnable Limbo processes. */
static Prog *tail;                        /* FIFO tail. */
static Prog *processes;                   /* Complete GC root/process list. */
static int nextpid;                       /* Monotonic process identity. */
static unsigned long execution_slices;    /* Busy-work budget since the last actual standard-input progress. */
static int live;                          /* Process quota accounting. */
static char failure[256];                 /* Current native error copied before unwinding. */
static char *module_directory;            /* Read-only module resource base; ordinary Sys paths retain caller cwd. */
static Osenv environment;                 /* Error string environment for this runtime owner. */
extern int dontcompile;                   /* Production loader switch that prohibits forced JIT compilation. */
int cflag;                                /* Native leaf compilation switch consumed by the loader. */
int keepbroken;                           /* No debugger retention is implemented in this initial runner. */
Type *TFD;                                /* Descriptor owner for supported module/control input handles. */
Type *TFileIO;                            /* Unavailable file-to-channel host type. */
unsigned long dvm_runtime_instructions;   /* Dispatched instruction attempts across every Limbo process. */
static unsigned long native_instructions; /* Successfully executed native leaves, excluding rejected guards. */
unsigned long
    dvm_runtime_instruction_limit;     /* Optional total dispatch limit; zero leaves the existing policy unchanged. */
static int instruction_boundary_error; /* A budget rejection happens before fetching/advancing the current PC. */
static int show_stats;                 /* Emit actual instruction counts on explicit request. */
static int inspect_reals;       /* Interpret the explicitly requested scalar range as binary64, not native words. */
static int inspect_requested;   /* Explicit read-only post-execution module-data inspection. */
static size_t inspect_offset;   /* Requested byte offset, aligned to the NC64 word width. */
static size_t inspect_count;    /* Requested native words, bounded to 4096 before execution. */
static uchar *inspect_data;     /* [GC:] Retained scalar-only entry-instance MP until report/cleanup. */
static int test_control;        /* Explicit isolated-test halt capability; disabled for normal applications. */
static FVM_PortFile *files[32]; /* File slots 3..31 use FlashVM ports; standard 0..2 remain borrowed. */


/* Print copied instruction/native counters only for an explicit --stats request.
 * No parameters or VM mutation; terminal success/error/test-halt paths share this exact format. */
static void runtime_statistics(void)
{
    if (show_stats)
    {
        fprintf(stderr, "instructions=%lu native=%lu\n", dvm_runtime_instructions, native_instructions);
    }
}


/* Admit one complete instruction boundary on this native owner, before decode or C/native execution.
 * The optional total limit includes guarded fallbacks once and is never reset by UI/stdin progress.
 * A rejection leaves the current PC and registers unmodified and raises through the process guard. */
void dvm_runtime_step(void)
{
    if (dvm_runtime_instruction_limit && dvm_runtime_instructions >= dvm_runtime_instruction_limit)
    {
        instruction_boundary_error = 1;
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
 * process is borrowed on the sole VM owner. Only pointer-free, bounded NC64 words are admitted;
 * pointer-bearing descriptors are rejected rather than exposing native heap addresses. */
static void inspect_retain(Prog *process)
{
    Heap *heap;
    uchar *data = process->R.MP;
    if (data == nil || data == H)
    {
        error("inspection requires entry module data");
    }
    heap = D2H(data);
    if (!heap->t || heap->t->np != 0 || heap->t->size < 0 || inspect_offset % 8U != 0 ||
        inspect_offset > (size_t)heap->t->size || inspect_count > ((size_t)heap->t->size - inspect_offset) / 8U)
    {
        error("inspection requires an aligned in-bounds range in pointer-free module data");
    }
    if (heap->ref == 0 || heap->ref == (ulong)-1)
    {
        error("inspection encountered an invalid module-data owner");
    }
    heap->ref++;
    Setmark(heap);
    inspect_data = data; /* [GC:] One explicit native retain, no heap pointers in the retained scalar payload. */
}


/* Retire the optional native inspection retain on the sole VM owner; repeated cleanup is harmless.
 * The active process may still own the same scalar data on an error path; its root is unaffected. */
static void inspect_release(void)
{
    if (inspect_data)
    {
        destroy(inspect_data);
        inspect_data = nil; /* [GC:] Retire exactly the extra native root, never a process-owned reference. */
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


/* Create a bounded arena with stable headers for the retained cycle collector. */
static void arena_init(void)
{
    Bhdr *block;
    ulong header = (sizeof(Bhdr) + 15UL) & ~15UL;
    memory.arena = calloc(1, ARENA_BYTES);
    if (!memory.arena)
    {
        error(exNomem);
    }
    memory.arena->magic = MAGIC_E;
    memory.arena->size = header;
    memory.arena->csize = ARENA_BYTES;
    memory.arena->clink = nil;
    block = B2NB(memory.arena);
    block->magic = MAGIC_F;
    block->size = ARENA_BYTES - header;
}


/* First-fit allocation preserves block addresses and GC traversal metadata. */
void *poolalloc(Pool *pool, ulong size)
{
    Bhdr *block;
    Bhdr *remainder;
    ulong need;
    if (size > ARENA_BYTES - BHDRSIZE - 16)
    {
        return nil;
    }
    need = (size + BHDRSIZE + 15UL) & ~15UL;
    for (block = B2NB(pool->arena); block < B2LIMIT(pool->arena); block = B2NB(block))
    {
        if (block->magic != MAGIC_F || block->size < need)
        {
            continue;
        }
        if (block->size - need >= sizeof(Bhdr) + 16)
        {
            remainder = (Bhdr *)((uchar *)block + need);
            remainder->magic = MAGIC_F;
            remainder->size = block->size - need;
            block->size = need;
        }
        block->magic = MAGIC_A;
        return B2D(block);
    }
    return nil;
}


/* Free one heap block without moving headers observed by the cycle collector. */
void poolfree(Pool *pool, void *value)
{
    Bhdr *block;
    USED(pool);
    D2B(block, value);
    block->magic = MAGIC_F;
}


/* Return the stable arena chain consumed by production rungc. */
Bhdr *poolchain(Pool *pool)
{
    return pool->arena;
}


/* Report usable payload bytes for production heap bounds checks. */
ulong poolmsize(Pool *pool, void *value)
{
    Bhdr *block;
    USED(pool);
    D2B(block, value);
    return block->size - BHDRSIZE;
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


/* Preserve the current host error and unwind through the retained Proc error frames. */
void error(char *message)
{
    if (message != failure)
    {
        snprintf(failure, sizeof(failure), "%s", message);
    }
    nexterror();
}


/* Propagate the active error to the next enclosing native owner frame. */
void nexterror(void)
{
    if (!up || up->nerr <= 0)
    {
        fprintf(stderr, "disvm: %s\n", failure);
        exit(1);
    }
    siglongjmp(up->estack[--up->nerr], 1);
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
    vsnprint(failure, sizeof(failure), format, arguments);
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


/* Locate the current runnable process for production channel operations. */
Prog *currun(void)
{
    return ready;
}


/* Append a runnable process to the scheduler FIFO. */
void addrun(Prog *process)
{
    process->state = Pready;
    process->link = nil;
    if (tail)
    {
        tail->link = process;
    }
    else
    {
        ready = process;
    }
    tail = process;
}


/* Block/remove the FIFO head while retaining it in the GC root list. */
Prog *delrun(int state)
{
    Prog *process = ready;
    ready = process->link;
    if (!ready)
    {
        tail = nil;
    }
    process->link = nil;
    process->state = state;
    return process;
}


/* Create one Limbo process; it is scheduled above one bound native OS owner. */
Prog *newprog(Prog *parent, Modlink *module)
{
    Prog *process;
    USED(parent);
    if (live == PROCESS_MAX)
    {
        error("process quota exceeded");
    }
    process = calloc(1, sizeof(Prog));
    if (!process)
    {
        error(exNomem);
    }
    process->pid = ++nextpid;
    process->quanta = PQUANTA;
    process->xec = xec;
    process->exval = H;
    process->R.M = module;
    process->R.MP = module->MP;
    D2H(module)->ref++;
    process->next = processes;
    processes = process;
    live++;
    addrun(process);
    return process;
}


/* Wake channel peers with an explicit hangup, using the production queue representation. */
void killcomm(Progq **queue)
{
    Progq *entry;
    Prog *process;
    while ((entry = *queue) != nil)
    {
        *queue = entry->next;
        process = entry->prog;
        free(entry);
        if (!process)
        {
            return;
        }
        process->ptr = nil;
        switch (process->state)
        {
        case Psend:
        case Precv:
            process->kill = "channel hangup";
            addrun(process);
            break;
        case Palt:
            altgone(process);
            break;
        case Prelease:
            error("foreign C channel waiter is not supported by this host");
            break;
        default:
            break;
        }
    }
}


/* Reuse production formatting and publish the actual stdout write result. */
void Sys_print(void *frame)
{
    F_Sys_print *call = frame;
    char text[4096];
    int count = xprint(currun(), frame, &call->vargs, call->s, text, sizeof(text));
    WORD result;
    if (count < 0 || count >= sizeof(text) - UTFmax - 2)
    {
        error("experimental stdout message limit");
    }
    result = fwrite(text, 1, count, stdout);
    fflush(stdout);
    memmove(call->ret, &result, sizeof(result));
}


/* Sys string-only functions are provided by retained runt.c; FD services are unavailable. */
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


/* Open module input through the shared FlashVM file implementation. */
static int open_file(const char *path, const char *mode)
{
    int index;
    for (index = 3; index < 32; index++)
    {
        if (!files[index])
        {
            if (fvm_port_file_open(path, mode, &files[index]) == FVM_OK)
            {
                return index;
            }
            kwerrstr("cannot open file: %s", path);
            return -1;
        }
    }
    kwerrstr("file handle quota exceeded");
    return -1;
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
                                  "styxservers.dis"}; /* Bundled module allowlist. */
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
    if (fd < 0 || fd >= 32 || !files[fd] || size < 0)
    {
        return -1;
    }
    if (fvm_port_file_read(files[fd], bytes, size, &received) != FVM_OK)
    {
        return -1;
    }
    return received;
}


/* Close exactly the module handle owned by the loader. */
int kclose(int fd)
{
    if (fd < 0 || fd >= 32 || !files[fd])
    {
        return -1;
    }
    fvm_port_file_close(files[fd]);
    files[fd] = nil;
    return 0;
}


/* Determine module extent without reading beyond its framed contents. */
Dir *kdirfstat(int fd)
{
    Dir *metadata;
    size_t available;
    if (fd < 0 || fd >= 32 || !files[fd])
    {
        return nil;
    }
    if (fvm_port_file_size(files[fd], &available) != FVM_OK || available >= 8UL * 1024UL * 1024UL)
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
    return seprint(start, end, "%s", failure);
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
    D2H(link)->ref--;
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


/* Execute a bounded experimental application using real decoding, JIT and channels.
 * argc/argv select the NC64 module and runtime options. Returns zero on completion,
 * one for execution/input failure, or two for an invalid command/ABI configuration. */
int main(int argc, char **argv)
{
    DVM_NativeContext *context;
    Module *module;
    Prog *process;
    Prog **slot;
    char *entry;
    cflag = 1;
    while (argc > 1 && argv[1][0] == '-')
    {
#ifdef DVM_RUNTIME_EDITOR
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
            puts("DisVM runtime " DVM_RUNTIME_VERSION ": experimental NC64, magic 819264, Linux AMD64");
            return 0;
        }
        else if (strcmp(argv[1], "--help") == 0)
        {
            puts(
                "usage: disvm [--jit|--no-jit] [--stats] [--max-instructions COUNT] [--test-control] [--inspect-words OFFSET COUNT | --inspect-reals OFFSET COUNT] native-cell.dis");
#ifdef DVM_RUNTIME_EDITOR
            puts(
                "editor options: [--font TTF] [--line-numbers 0|1] (bundled Noto Sans Mono, default 11) [--document UTF8 | --new PATH] [--events FRAMES | --window-events FRAMES | --clipboard-events FRAMES] [--capture PPM]");
#endif
            puts("Experimental NC64 cells; legacy .dis modules are rejected. Sys subset, one native owner.");
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
    dontcompile = !cflag;
    if (sizeof(WORD) != 8 || sizeof(void *) != 8 || IBY2WD != 8 || offsetof(Alt, ac) != 16 ||
        offsetof(F_Sys_print, s) != 64)
    {
        fprintf(stderr, "native-cell compiler/runtime frame contract mismatch\n");
        return 2;
    }
    context = dvm_native_process_create(nil);
    if (!context || !dvm_native_context_bind(context))
    {
        return 1;
    }
    up->env = &environment;
    environment.errstr = failure;
    if (waserror())
    {
        inspect_release();
        fprintf(stderr, "disvm: %s\n", failure);
        runtime_statistics();
        return 1;
    }
    arena_init();
    opinit();
    sysmodinit();
    loadermodinit();
    mathmodinit();
    dvm_bigtable_initialize();
    dvm_srv_initialize();
    srvmodinit();
    entry = entry_resource(argv[1]);
#ifdef DVM_RUNTIME_EDITOR
    if (!dvm_editor_default_font(module_directory) || !dvm_editor_start())
    {
        dvm_editor_stop();
        error("cannot initialize graphical editor");
    }
#endif
    module = load(entry);
    if (!module)
    {
        error(failure);
    }
    process = start_module(module);
    if (inspect_requested)
    {
        inspect_retain(process);
    }
    while (ready
#ifdef DVM_RUNTIME_EDITOR
           || (live && dvm_editor_attached())
#endif
    )
    {
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
                execution_slices = 0;
            }
        }
        if (!ready)
        {
            fvm_port_thread_sleep_us(1000);
            continue;
        }
#endif
        if (++execution_slices > 100000)
        {
            error("execution budget exceeded");
        }
        process = ready;
        if (!dvm_native_context_set_process(context, process))
        {
            error("cannot publish active Dis process in TLS owner");
        }
        up->prog = process;
        if (waserror())
        {
            process->R = DVM_R;
            if (failure[0])
            {
                fprintf(stderr,
                        "Dis process %d module %s pc %ld: %s\n",
                        process->pid,
                        DVM_R.M->m->path,
                        (long)(DVM_R.PC - DVM_R.M->prog - (instruction_boundary_error ? 0 : 1)),
                        failure);
                inspect_release(); /* [GC:] The per-process guard terminates here without reaching the outer guard. */
                runtime_statistics();
                return 1;
            }
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
            free(process);
            live--;
        }
        else
        {
            xec(process);
            poperror();
            if (ready == process)
            {
                addrun(delrun(Pready));
            }
        }
        if (processes)
        {
            rungc(processes);
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
        runtime_statistics();
        return 1;
    }
    runtime_statistics();
    puts("PASS Dis application completed");
    return 0;
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
static char *file_path(String *name)
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
    F_Sys_open *call = frame;
    Sys_FD *old;
    Sys_FD *result;
    int descriptor;
    char *path = file_path(call->s);
    if (!path)
    {
        Sys_FD *empty = H;
        memmove(&old, call->ret, sizeof(old));
        memmove(call->ret, &empty, sizeof(empty));
        destroy(old);
        return;
    }
    if (call->s == H)
    {
        error(exNilref);
    }
    if (test_control && strcmp(path, "/dev/sysctl") == 0 && call->mode == 1)
    {
        descriptor = -2;
    }
    else
    {
        descriptor = -1;
        if (call->mode == OREAD)
        {
            descriptor = open_file(path, "r");
        }
        else
        {
            kwerrstr("unsupported file open mode");
        }
    }
    result = H;
    if (descriptor >= 0 || descriptor == -2)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = descriptor;
    }
    memmove(&old, call->ret, sizeof(old));
    memmove(call->ret, &result, sizeof(result));
    destroy(old);
}


/* Validate one live native descriptor/byte-array request; input storage stays caller-owned. */
static int io_span(Sys_FD *descriptor, Array *bytes, WORD count);


/* Write to actual native files/standard output, or consume the explicit test halt capability. */
void Sys_write(void *frame)
{
    F_Sys_write *call = frame;
    if (test_control && call->fd != H && call->fd->fd == -2 && call->buf != H && call->n == 4 && call->buf->len >= 4 &&
        memcmp(call->buf->data, "halt", 4) == 0)
    {
        fflush(stdout);
        inspect_release(); /* [GC:] Explicit test halt also retires any captured scalar data before exit. */
        runtime_statistics();
        exit(0);
    }
    {
        WORD result = 0;
        int count;
        size_t written;
        if (call->buf != H && call->n >= 0)
        {
            count = io_span(call->fd, call->buf, call->n);
            if (count < 0)
            {
                result = -1;
            }
            else if (call->fd->fd == 1 || call->fd->fd == 2)
            {
                result = fvm_port_stdio_write((int)call->fd->fd, call->buf->data, count, &written) == FVM_OK
                             ? (WORD)written
                             : -1;
            }
            else if (call->fd->fd >= 3 && files[call->fd->fd])
            {
                result = fvm_port_file_write(files[call->fd->fd], call->buf->data, count) == FVM_OK ? count : -1;
            }
            else
            {
                result = -1;
            }
        }
        memmove(call->ret, &result, sizeof(result));
    }
}


/* Count only a successfully accepted native leaf; guarded rejection retains the C handler. */
int dvm_runtime_leaf(Jit *jit, int operation, REG *registers)
{
    int accepted;
    /* Classic native table leaves read four-byte fields; NC64 uses its checked C adapters. */
    if (operation == ICASE || operation == ICASEL || operation == IGOTO)
    {
        return 0;
    }
    accepted = jit->entry[operation](registers);
    if (accepted)
    {
        native_instructions++;
    }
    return accepted;
}


/* Validate a typed byte-array I/O and keep its native count within the C port range.
 * descriptor/bytes borrow a live FD and mutable array; count is clamped to its extent.
 * Returns the bounded byte count, or -1 for invalid array metadata. */
static int io_span(Sys_FD *descriptor, Array *bytes, WORD count)
{
    if (descriptor == H || descriptor == nil || D2H(descriptor)->t != TFD)
    {
        error(exNilref);
    }
    if (descriptor->fd < 0 || descriptor->fd >= 32)
    {
        kwerrstr("invalid native file descriptor");
        return -1;
    }
    if (bytes == nil || D2H(bytes)->t != &Tarray || bytes->t == nil || bytes->t == H || bytes->t->size != 1 ||
        bytes->t->np != 0 || bytes->len < 0 || bytes->len > 0x7fffffff || (bytes->len && !bytes->data))
    {
        kwerrstr("invalid byte-array read span");
        return -1;
    }
    if (count > bytes->len)
    {
        count = bytes->len;
    }
    return count;
}


/* Read one genuine fragment through the shared FlashVM file port.
 * frame supplies a borrowed descriptor/array; result is the byte count or -1. */
void Sys_read(void *frame)
{
    F_Sys_read *call = frame;
    int count;
    size_t received;
    WORD result = 0;
    if (call->buf != H && call->n >= 0)
    {
        count = io_span(call->fd, call->buf, call->n);
        if (count < 0)
        {
            result = -1;
        }
        else if (call->fd->fd == 0)
        {
            result = fvm_port_stdio_read(0, call->buf->data, count, &received) == FVM_OK ? (WORD)received : -1;
            if (result > 0)
            {
                execution_slices = 0;
            }
        }
        else
        {
            result = count == 0 ? 0 : kread(call->fd->fd, call->buf->data, count);
        }
    }
    memmove(call->ret, &result, sizeof(result));
}


/* Read until the requested extent, EOF or an actual port error.
 * frame is borrowed; partial reads return their exact count and leave trailing bytes unread. */
void Sys_readn(void *frame)
{
    F_Sys_readn *call = frame;
    int count;
    WORD result = 0;
    long fragment;
    size_t received;
    if (call->buf == H || call->n < 0)
    {
        memmove(call->ret, &result, sizeof(result));
        return;
    }
    count = io_span(call->fd, call->buf, call->n);
    if (count < 0)
    {
        result = -1;
        memmove(call->ret, &result, sizeof(result));
        return;
    }
    while (result < count)
    {
        if (call->fd->fd == 0)
        {
            fragment = fvm_port_stdio_read(0, call->buf->data + result, count - result, &received) == FVM_OK
                           ? (long)received
                           : -1;
            if (fragment > 0)
            {
                execution_slices = 0;
            }
        }
        else
        {
            fragment = kread(call->fd->fd, call->buf->data + result, count - result);
        }
        if (fragment <= 0)
        {
            if (!result)
            {
                result = fragment;
            }
            break;
        }
        result += fragment;
    }
    memmove(call->ret, &result, sizeof(result));
}


/* Publish a new descriptor while preserving the previous result owner until construction completes. */
static void publish_descriptor(void *slot, Sys_FD *result)
{
    Sys_FD *old;
    memmove(&old, slot, sizeof(old));
    memmove(slot, &result, sizeof(result));
    destroy(old);
}


/* Borrow standard descriptors 0/1/2 without taking ownership of their OS handles. */
void Sys_fildes(void *frame)
{
    F_Sys_fildes *call = frame;
    Sys_FD *result = H;
    if (call->fd >= 0 && call->fd <= 2)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = call->fd;
    }
    else
    {
        kwerrstr("only standard descriptors 0, 1 and 2 can be borrowed");
    }
    publish_descriptor(call->ret, result);
}


/* Create/truncate a file only after validating flags and allocating its result owner.
 * The supported permission value is 0666 filtered by the platform umask. */
void Sys_create(void *frame)
{
    F_Sys_create *call = frame;
    Sys_FD *result = H;
    int descriptor;
    char *path = file_path(call->s);
    if (!path)
    {
        publish_descriptor(call->ret, H);
        return;
    }
    if (call->s == H || call->s == nil)
    {
        error(exNilref);
    }
    if (call->mode == 1 && call->perm == 0666)
    {
        result = H2D(Sys_FD *, heapz(TFD));
        result->fd = -1;
        descriptor = open_file(path, "w");
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
        kwerrstr("create supports OWRITE and permission 0666 only");
    }
    publish_descriptor(call->ret, result);
}


/* Position an ordinary native file; standard streams and test capabilities fail.
 * frame borrows a typed FD and signed big offset. The big result is the new
 * position, or -1 with a native error. Checked narrowing precedes the port call. */
void Sys_seek(void *frame)
{
    F_Sys_seek *call = frame;
    LONG result = -1;
    long position;
    if (call->fd == H || call->fd == nil || D2H(call->fd)->t != TFD)
    {
        error(exNilref);
    }
    if (call->fd->fd < 3 || call->fd->fd >= 32 || !files[call->fd->fd])
    {
        kwerrstr("seek requires an ordinary live file");
    }
    else if (call->start < FVM_FILE_SEEK_START || call->start > FVM_FILE_SEEK_END || call->off < LONG_MIN ||
             call->off > LONG_MAX)
    {
        kwerrstr("invalid seek origin or native offset range");
    }
    else if (fvm_port_file_seek(files[call->fd->fd], (long)call->off, (int)call->start, &position) != FVM_OK)
    {
        kwerrstr("file seek failed");
    }
    else
    {
        result = position;
    }
    memmove(call->ret, &result, sizeof(result));
}


/* Format and write an actual FD message through the shared file/stdio port.
 * frame borrows a typed descriptor and variadic VM slots. Returns actual bytes
 * written, or -1 on an I/O failure; formatting shares the bounded print contract. */
void Sys_fprint(void *frame)
{
    F_Sys_fprint *call = frame;
    char bytes[4096];
    size_t written;
    WORD result = -1;
    int count;
    if (call->fd == H || call->fd == nil || D2H(call->fd)->t != TFD)
    {
        error(exNilref);
    }
    count = xprint(currun(), frame, &call->vargs, call->s, bytes, sizeof(bytes));
    if (count < 0 || count >= sizeof(bytes) - UTFmax - 2)
    {
        error("experimental formatted FD message limit");
    }
    if (call->fd->fd == 1 || call->fd->fd == 2)
    {
        if (fvm_port_stdio_write((int)call->fd->fd, bytes, count, &written) == FVM_OK)
        {
            result = written;
        }
    }
    else if (call->fd->fd >= 3 && call->fd->fd < 32 && files[call->fd->fd])
    {
        if (fvm_port_file_write(files[call->fd->fd], bytes, count) == FVM_OK)
        {
            result = count;
        }
    }
    if (result < 0)
    {
        kwerrstr("formatted descriptor write failed");
    }
    memmove(call->ret, &result, sizeof(result));
}
