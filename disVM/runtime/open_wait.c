/* Copyright (c) 2026 bhgv
 * EN: Native I/O with rooted arguments and mutex-protected shared accounting.
 * RU: Нативный I/O с укоренёнными аргументами и общим учётом под мьютексом.
 */
/* Construct native handles from private snapshots, outside managed heap turns. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "interp.h"
#include "open_wait.h"
#include "runtime_gc.h"
#include "channel_sync.h"
#include "vfs/guest.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long attempts; /* Completed native open attempts, under the registry mutex. */
static unsigned long releases; /* Attempts that released the native heap lease; updates under registry mutex. */
static size_t staging;         /* Simultaneously live native path snapshot bytes; updates under registry mutex. */
static size_t peak;            /* Peak snapshot bytes, including NUL terminators; updates under registry mutex. */


/* See open_wait.h for ownership and threading. Native locals receive the new
 * handle; the caller's table remains untouched until native heap reentry. */
FVM_Result dvm_open_wait_selected(const char *path,
                                  const char *mode,
                                  int asynchronous,
                                  DVM_ResourcePlan *selected,
                                  FVM_PortFile **file,
                                  FVM_PortDirectory **directory)
{
    FVM_FileInfo metadata;
    FVM_Result status;
    char *snapshot;
    size_t length;
    int released;
    DVM_ResourcePlan local;
    int owns = 0;
    if (!path || !mode || !file || !directory)
    {
        return FVM_INVALID_ARGUMENT;
    }
    *file = NULL;
    *directory = NULL;
    if (asynchronous && !selected)
    {
        if (!dvm_resource_plan(dvm_namespace_current_group(), path, 0, &local))
        {
            return FVM_INVALID_ARGUMENT;
        }
        selected = &local;
        owns = 1;
    }
    if (selected)
    {
        path = selected->names.native;
    }
    length = strlen(path);
    if (length == (size_t)-1)
    {
        if (owns)
        {
            dvm_resource_close(&local);
        }
        return FVM_INVALID_ARGUMENT;
    }
    snapshot = malloc(length + 1);
    if (!snapshot)
    {
        if (owns)
        {
            dvm_resource_close(&local);
        }
        return FVM_OUT_OF_MEMORY;
    }
    memcpy(snapshot, path, length + 1);
    fvm_port_registry_lock();
    staging += length + 1;
    if (staging > peak)
    {
        peak = staging;
    }
    fvm_port_registry_unlock();
    released = asynchronous ? dvm_runtime_gc_native_try_leave() : 0;
    if (released < 0)
    {
        fvm_port_registry_lock();
        staging -= length + 1;
        fvm_port_registry_unlock();
        free(snapshot);
        if (owns)
        {
            dvm_resource_close(&local);
        }
        return FVM_OUT_OF_MEMORY;
    }
    if (selected)
    {
        status = dvm_resource_open(selected, mode, file, directory);
    }
    else if (!strcmp(mode, "r") && fvm_port_path_info(snapshot, &metadata) == FVM_OK && metadata.directory)
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
    fvm_port_registry_lock();
    attempts++;
    releases += released != 0;
    staging -= length + 1;
    fvm_port_registry_unlock();
    free(snapshot);
    if (owns)
    {
        dvm_resource_close(&local);
    }
    return status;
}


/* EN/RU: See open_wait.h; capture current namespace for asynchronous Sys callers.
 * См. open_wait.h; захватить namespace для asynchronous Sys callers. */
FVM_Result
dvm_open_wait(const char *path, const char *mode, int asynchronous, FVM_PortFile **file, FVM_PortDirectory **directory)
{
    return dvm_open_wait_selected(path, mode, asynchronous, NULL, file, directory);
}


/* EN: Print aggregate counters from a locked snapshot, normally after native producer join; no handle access.
 * RU: Вывести общие счётчики после join производителей, без доступа к handles. */
void dvm_open_statistics(void)
{
    fvm_port_registry_lock();
    unsigned long snapshot_attempts = attempts;
    unsigned long snapshot_releases = releases;
    size_t snapshot_staging = staging;
    size_t snapshot_peak = peak;
    fvm_port_registry_unlock();
    fprintf(stderr,
            "NATIVEOPEN attempts=%lu releases=%lu staging=%lu peak=%lu\n",
            snapshot_attempts,
            snapshot_releases,
            (unsigned long)snapshot_staging,
            (unsigned long)snapshot_peak);
}
