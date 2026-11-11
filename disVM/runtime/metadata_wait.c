/* Copyright (c) 2026 bhgv
 * EN: Native I/O with rooted arguments and mutex-protected shared accounting.
 * RU: Нативный I/O с укоренёнными аргументами и общим учётом под мьютексом.
 */
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
#include "channel_sync.h"
#include "managed_workers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long paths;       /* Completed native path queries, under the registry mutex. */
static unsigned long descriptors; /* Completed native file/directory queries; updates under registry mutex. */
static unsigned long batches;     /* Native directory read attempts, including EOF/errors; updates under registry mutex. */
static unsigned long releases;    /* Queries that released the native heap lease; updates under registry mutex. */
static unsigned int pins;         /* Explicit FD roots retained by this adapter; updates under registry mutex. */
static size_t staging;            /* Current native path bytes, including terminators; updates under registry mutex. */
static size_t peak_staging;       /* Largest simultaneous native path staging; updates under registry mutex. */


/* EN: Preserve native admission refusal/cancellation as an ordinary I/O failure.
 * released is a negative admission result; operation is a borrowed label.
 * Return FVM_ERROR, never pretend a cancelled wait failed an allocation.
 * RU: Сохранить отказ допуска/отмену как обычную ошибку I/O.
 * released — отрицательный результат допуска, operation — заимствованная метка.
 * Вернуть FVM_ERROR, не выдавая отмену ожидания за отказ выделения памяти. */
static FVM_Result admission_failure(int released, const char *operation)
{
    kwerrstr("%s %s", operation, released == -2 ? "cancelled" : "needs an executor");
    return FVM_ERROR;
}


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
        kwerrstr("invalid metadata path or result");
        return FVM_INVALID_ARGUMENT;
    }
    length = strlen(path);
    if (length == (size_t)-1)
    {
        kwerrstr("metadata path is too long");
        return FVM_INVALID_ARGUMENT;
    }
    snapshot = malloc(length + 1);
    if (!snapshot)
    {
        kwerrstr("out of memory staging metadata path");
        return FVM_OUT_OF_MEMORY;
    }
    memcpy(snapshot, path, length + 1);
    fvm_port_registry_lock();
    staging += length + 1;
    if (staging > peak_staging)
    {
        peak_staging = staging;
    }
    fvm_port_registry_unlock();
    released = dvm_runtime_gc_native_try_leave();
    if (released < 0)
    {
        fvm_port_registry_lock();
        staging -= length + 1;
        fvm_port_registry_unlock();
        free(snapshot);
        return admission_failure(released, "native path metadata");
    }
    status = fvm_port_path_info(snapshot, output);
    dvm_runtime_gc_native_enter(released);
    fvm_port_registry_lock();
    paths++;
    releases += released != 0;
    staging -= length + 1;
    fvm_port_registry_unlock();
    free(snapshot);
    if (status != FVM_OK)
    {
        kwerrstr(status == FVM_UNSUPPORTED ? "unsupported filesystem path or metadata" : "file stat failed");
    }
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
        kwerrstr("fstat requires a live ordinary file or directory");
        return FVM_INVALID_ARGUMENT;
    }
    ptradd(D2H(fd));
    fvm_port_registry_lock();
    pins++;
    fvm_port_registry_unlock();
    Setmark(D2H(fd));
    dvm_managed_workers_file_call();
    released = dvm_file_gate_try_release(gate, &ticket);
    if (released < 0)
    {
        ptrdel(D2H(fd));
        fvm_port_registry_lock();
        pins--;
        fvm_port_registry_unlock();
        return admission_failure(released, "native metadata");
    }
    status = file ? fvm_port_file_info(file, output) : fvm_port_directory_info(directory, output);
    if (!dvm_file_gate_leave(gate, &ticket))
    {
        fprintf(stderr, "disvm: native metadata admission lost\n");
        exit(1);
    }
    dvm_runtime_gc_native_enter(released);
    fvm_port_registry_lock();
    descriptors++;
    releases += released != 0;
    fvm_port_registry_unlock();
    ptrdel(D2H(fd));
    fvm_port_registry_lock();
    pins--;
    fvm_port_registry_unlock();
    if (status != FVM_OK)
    {
        kwerrstr("fstat requires a live ordinary file or directory");
    }
    return status;
}


/* EN: Retain the cursor through protected result publication, without its mutex.
 * Peer residents may queue; cancellation before admission publishes no batch.
 * RU: Держать курсор до защищённой публикации результата без его мьютекса.
 * Соседи могут ждать; отмена до допуска не публикует batch. */
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
        kwerrstr("dirread requires a live directory and result");
        return FVM_INVALID_ARGUMENT;
    }
    ptradd(D2H(fd));
    fvm_port_registry_lock();
    pins++;
    fvm_port_registry_unlock();
    Setmark(D2H(fd));
    dvm_managed_workers_file_call();
    released = dvm_file_gate_try_release(gate, &ticket);
    if (released < 0)
    {
        ptrdel(D2H(fd));
        fvm_port_registry_lock();
        pins--;
        fvm_port_registry_unlock();
        return admission_failure(released, "native directory read");
    }
    status = fvm_port_directory_read(directory, entries, count);
    dvm_runtime_gc_native_enter(released);
    fvm_port_registry_lock();
    batches++;
    releases += released != 0;
    fvm_port_registry_unlock();
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
    fvm_port_registry_lock();
    pins--;
    fvm_port_registry_unlock();
    if (!published)
    {
        krethrow();
    }
    if (status != FVM_OK)
    {
        kwerrstr("directory read failed");
    }
    return status;
}


/* EN: Print counters from a locked snapshot, normally after native producer join. No arguments/result.
 * RU: Вывести счётчики из снимка под мьютексом, обычно после join производителей. Без параметров/результата. */
void dvm_metadata_statistics(void)
{
    fvm_port_registry_lock();
    unsigned long snapshot_paths = paths;
    unsigned long snapshot_descriptors = descriptors;
    unsigned long snapshot_releases = releases;
    unsigned int snapshot_pins = pins;
    size_t snapshot_staging = staging;
    size_t snapshot_peak_staging = peak_staging;
    unsigned long snapshot_batches = batches;
    fvm_port_registry_unlock();
    fprintf(stderr, "NATIVEMETADATA paths=%lu descriptors=%lu releases=%lu pins=%u staging=%lu peak=%lu batches=%lu\n",
        snapshot_paths, snapshot_descriptors, snapshot_releases, snapshot_pins, (unsigned long)snapshot_staging, (unsigned long)snapshot_peak_staging, snapshot_batches);
}
