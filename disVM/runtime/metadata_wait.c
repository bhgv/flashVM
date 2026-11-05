/* Metadata waits stage native paths/records and retain mapped descriptor roots. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "kernel.h"
#include "raise.h"
#include "metadata_wait.h"
#include "filesystem_sys.h"
#include "io_buffer.h"
#include "file_gate.h"
#include "runtime_gc.h"
#include "managed_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long paths;       /* Completed native path queries, under heap exclusion. */
static unsigned long descriptors; /* Completed native file/directory queries. */
static unsigned long batches;     /* Native directory read attempts, including EOF/errors. */
static unsigned long releases;    /* Queries that released the native heap lease. */
static unsigned int pins;         /* Explicit FD roots retained by this adapter. */
static size_t staging;            /* Current native path bytes, including terminators. */
static size_t peak_staging;       /* Largest simultaneous native path staging. */


/* Snapshot path before native release. path/output are borrowed ACTIVE inputs;
 * output is native C storage, never a mapped Dir. Allocation failure performs no
 * native call. All remaining throwing preparation precedes lease publication. */
FVM_Result dvm_metadata_path_info(const char *path, FVM_FileInfo *output)
{
    FVM_Result status;
    size_t length;
    char *snapshot;
    int released;
    if (!path || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    length = strlen(path);
    if (length == (size_t)-1)
    {
        return FVM_INVALID_ARGUMENT;
    }
    snapshot = malloc(length + 1);
    if (!snapshot)
    {
        return FVM_OUT_OF_MEMORY;
    }
    memcpy(snapshot, path, length + 1);
    staging += length + 1;
    if (staging > peak_staging)
    {
        peak_staging = staging;
    }
    released = dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        staging -= length + 1;
        free(snapshot);
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_port_path_info(snapshot, output);
    dvm_runtime_gc_native_enter(released);
    paths++;
    releases += released != 0;
    staging -= length + 1;
    free(snapshot);
    return status;
}


/* Only raw native handles, FIFO state and copied metadata are touched outside
 * the heap. FD is a checked mapped argument; its explicit root retains the slot
 * and gate across another actor's reference changes or GC while waiting. */
FVM_Result dvm_metadata_fd_info(Sys_FD *fd, int descriptor, FVM_FileInfo *output)
{
    FVM_PortFile *file = dvm_runtime_file(descriptor);
    FVM_PortDirectory *directory = dvm_runtime_directory(descriptor);
    DVM_FileGate *gate = dvm_runtime_file_gate(descriptor);
    FVM_Result status;
    DVM_FileTicket ticket;
    int released;
    if ((!file && !directory) || !gate || !output)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ptradd(D2H(fd));
    pins++;
    Setmark(D2H(fd));
    dvm_managed_workers_file_call();
    released = dvm_file_gate_try_release(gate, &ticket);
    if (released < 0)
    {
        ptrdel(D2H(fd));
        pins--;
        return FVM_OUT_OF_MEMORY;
    }
    status = file ? fvm_port_file_info(file, output) : fvm_port_directory_info(directory, output);
    if (!dvm_file_gate_leave(gate, &ticket))
    {
        fprintf(stderr, "disvm: native metadata admission lost\n");
        exit(1);
    }
    dvm_runtime_gc_native_enter(released);
    descriptors++;
    releases += released != 0;
    ptrdel(D2H(fd));
    pins--;
    return status;
}


/* Keep the logical cursor ticket through publication, without holding its
 * mutex. Later guests can queue while this owner restores NativeWorld. */
FVM_Result dvm_metadata_directory_read(Sys_FD *fd, int descriptor, FVM_FileInfo **entries,
    size_t *count, int (*publish)(void *), void *argument)
{
    FVM_PortDirectory *directory = dvm_runtime_directory(descriptor);
    DVM_FileGate *gate = dvm_runtime_file_gate(descriptor);
    FVM_Result status;
    DVM_FileTicket ticket;
    int released, published = 1;

    if (!directory || !gate || !entries || !count || !publish)
    {
        return FVM_INVALID_ARGUMENT;
    }
    ptradd(D2H(fd));
    pins++;
    Setmark(D2H(fd));
    dvm_managed_workers_file_call();
    released = dvm_file_gate_try_release(gate, &ticket);
    if (released < 0)
    {
        ptrdel(D2H(fd));
        pins--;
        return FVM_OUT_OF_MEMORY;
    }
    status = fvm_port_directory_read(directory, entries, count);
    dvm_runtime_gc_native_enter(released);
    batches++;
    releases += released != 0;
    if (status == FVM_OK && *count)
    {
        published = kprotect(publish, argument);
        if (!published)
        {
            fvm_port_directory_abort(directory);
        }
    }
    if (!dvm_file_gate_leave(gate, &ticket))
    {
        fprintf(stderr, "disvm: native directory admission lost\n");
        exit(1);
    }
    fvm_port_directory_entries_free(*entries);
    *entries = NULL;
    ptrdel(D2H(fd));
    pins--;
    if (!published)
    {
        krethrow();
    }
    return status;
}


/* Called with heap exclusion after all native continuations returned. No args,
 * allocation or managed root traversal; stderr is the diagnostic destination. */
void dvm_metadata_statistics(void)
{
    fprintf(stderr, "NATIVEMETADATA paths=%lu descriptors=%lu releases=%lu pins=%u staging=%lu peak=%lu batches=%lu\n",
        paths, descriptors, releases, pins, (unsigned long)staging, (unsigned long)peak_staging, batches);
}
