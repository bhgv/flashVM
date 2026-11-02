/* Construct native handles from private snapshots, outside managed heap turns. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "open_wait.h"
#include "runtime_gc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long attempts; /* Completed native open attempts, under heap exclusion. */
static unsigned long releases; /* Attempts that released the native heap lease. */
static size_t staging;         /* Simultaneously live native path snapshot bytes. */
static size_t peak;            /* Peak snapshot bytes, including NUL terminators. */


/* See open_wait.h for ownership and threading. Native locals receive the new
 * handle; the caller's table remains untouched until native heap reentry. */
FVM_Result dvm_open_wait(const char *path, const char *mode, int asynchronous,
    FVM_PortFile **file, FVM_PortDirectory **directory)
{
    FVM_FileInfo metadata;
    FVM_Result status;
    char *snapshot;
    size_t length;
    int released;
    if (!path || !mode || !file || !directory)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *file = NULL;
    *directory = NULL;
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
    if (staging > peak)
    {
        peak = staging;
    }
    released = asynchronous ? dvm_runtime_gc_native_leave() : 0;
    if (!strcmp(mode, "r") && fvm_port_path_info(snapshot, &metadata) == FVM_OK && metadata.directory)
    {
        status = fvm_port_directory_open(snapshot, directory);
    }
    else
    {
        status = fvm_port_file_open(snapshot, mode, file);
    }
    if (asynchronous)
    {
        dvm_runtime_gc_native_enter(released);
    }
    attempts++;
    releases += released != 0;
    staging -= length + 1;
    free(snapshot);
    return status;
}


/* Aggregate counters are inspected only under heap exclusion after all native
 * continuations finish; this does not synchronize the native handles themselves. */
void dvm_open_statistics(void)
{
    fprintf(stderr, "NATIVEOPEN attempts=%lu releases=%lu staging=%lu peak=%lu\n",
        attempts, releases, (unsigned long)staging, (unsigned long)peak);
}
