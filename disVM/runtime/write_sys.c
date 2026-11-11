/* Copyright (c) 2026 bhgv
 * EN: Native I/O with rooted arguments and mutex-protected shared accounting.
 * RU: Нативный I/O с укоренёнными аргументами и общим учётом под мьютексом.
 */
/* Immutable bounded native output snapshots outside the managed heap interval. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "runtime_gc.h"
#include "channel_sync.h"
#include "managed_workers.h"
#include "file_gate.h"
#include "io_buffer.h"
extern FVM_Result dvm_runtime_write_stdio(int descriptor, const void *bytes,
    size_t capacity, size_t *written, int *released);

#include <stdio.h>
#include <stdlib.h>

#define WRITE_STAGE_BYTES 65536U /* Maximum native output snapshot per call. */

static size_t staged_bytes;      /* Aggregate native snapshots, under the registry mutex. */
static size_t peak_staged_bytes; /* Maximum direct output storage before return; updates under registry mutex. */
static unsigned int pinned;     /* External FD/source-array root registrations; updates under registry mutex. */
static unsigned long fragments; /* Completed native port attempts; updates under registry mutex. */
static unsigned long copied;    /* Actual bytes acknowledged by the native port; updates under registry mutex. */


/* EN: Report native output counters from a locked snapshot (normally after all producers join), for --stats.
 * RU: Сообщить счётчики вывода из снимка под мьютексом (обычно после join производителей), для --stats. */
void dvm_runtime_write_statistics(void)
{
    fvm_port_registry_lock();
    unsigned int snapshot_pinned = pinned;
    size_t snapshot_staged_bytes = staged_bytes;
    size_t snapshot_peak_staged_bytes = peak_staged_bytes;
    unsigned long snapshot_fragments = fragments;
    unsigned long snapshot_copied = copied;
    fvm_port_registry_unlock();
    fprintf(stderr,"NATIVEOUTPUT pins=%u staging=%lu peak=%lu fragments=%lu copied=%lu\n",
        snapshot_pinned,(unsigned long)snapshot_staged_bytes,(unsigned long)snapshot_peak_staged_bytes,snapshot_fragments,snapshot_copied);
}


/* [GC:] Snapshot bytes before leaving heap ownership. fd/bytes are borrowed
 * mapped-frame owners, count clamps to their validated extent. Return one actual
 * acknowledged fragment or -1. The full mapped result is published by Sys_write
 * after this function returns. Pin failures unwind without leaking native storage
 * or external roots. No managed/throwing callback occurs in the released region. */
WORD dvm_runtime_write(Sys_FD *fd, Array *bytes, WORD count)
{
    int descriptor;
    int extent;
    uchar *data;
    FVM_PortFile *file;
    unsigned char *stage;
    size_t capacity;
    size_t written = 0;
    FVM_Result status;
    WORD result = 0;
    int released;
    DVM_FileTicket ticket;
    DVM_FileGate *gate;
    volatile int fd_pinned = 0;
    volatile int buffer_pinned = 0;
    if (bytes == H || count < 0)
    {
        return 0;
    }
    extent = dvm_io_span(fd,bytes,count,&descriptor,&data);
    if (extent < 0)
    {
        return extent;
    }
    file = descriptor >= 3 ? dvm_runtime_file(descriptor) : NULL;
    if (descriptor != 1 && descriptor != 2 && !file)
    {
        return -1;
    }
    gate = dvm_runtime_file_gate(descriptor);
    /* Preserve zero-write descriptor/access checks without staging any bytes.
     * The legacy file-port zero operation stays on the active owner. */
    if (!extent)
    {
        if (descriptor == 1 || descriptor == 2)
        {
            return 0;
        }
        dvm_file_gate_reserve(gate, &ticket);
        if (!dvm_file_gate_enter_now(gate, &ticket))
        {
            return -1;
        }
        status = fvm_port_file_write(file, NULL, 0);
        if (!dvm_file_gate_leave(gate, &ticket))
        {
            fprintf(stderr, "disvm: zero-write file admission lost\n");
            exit(1);
        }
        return status == FVM_OK ? 0 : -1;
    }
    capacity = (size_t)extent < WRITE_STAGE_BYTES ? (size_t)extent : WRITE_STAGE_BYTES;
    stage = malloc(capacity);
    if (!stage)
    {
        error(exNomem);
    }
    fvm_port_registry_lock();
    staged_bytes += capacity;
    if (staged_bytes > peak_staged_bytes)
    {
        peak_staged_bytes = staged_bytes;
    }
    fvm_port_registry_unlock();
    if (waserror())
    {
        if (buffer_pinned)
        {
            ptrdel(D2H(bytes));
            fvm_port_registry_lock();
            pinned--;
            fvm_port_registry_unlock();
        }
        if (fd_pinned)
        {
            ptrdel(D2H(fd));
            fvm_port_registry_lock();
            pinned--;
            fvm_port_registry_unlock();
        }
        fvm_port_registry_lock();
        staged_bytes -= capacity;
        fvm_port_registry_unlock();
        free(stage);
        nexterror();
    }
    ptradd(D2H(fd));
    fd_pinned = 1;
    fvm_port_registry_lock();
    pinned++;
    fvm_port_registry_unlock();
    Setmark(D2H(fd));
    ptradd(D2H(bytes));
    buffer_pinned = 1;
    fvm_port_registry_lock();
    pinned++;
    fvm_port_registry_unlock();
    Setmark(D2H(bytes));
    memmove(stage,data,capacity);
    /* [GC:] Reserve under the gate mutex, wait after native release. The
     * rooted FD pins the file/gate; finish BEFORE reentry prevents lock inversion. */
    if (file)
    {
        dvm_managed_workers_file_call();
    }
    if (descriptor == 1 || descriptor == 2)
    {
        status = dvm_runtime_write_stdio(descriptor, stage, capacity, &written, &released);
    }
    else
    {
        released = dvm_file_gate_try_release(gate, &ticket);
        if (released < 0)
        {
            kwerrstr(released == -2 ? "native output cancelled" : "native output needs an executor");
            result = -1;
            goto finish;
        }
        status = fvm_port_file_write(file, stage, capacity);
        if (status == FVM_OK)
        {
            written = capacity;
        }
        if (!dvm_file_gate_leave(gate, &ticket))
        {
            fprintf(stderr, "disvm: native file admission lost\n");
            exit(1);
        }
        dvm_runtime_gc_native_enter(released);
    }
    fvm_port_registry_lock();
    fragments++;
    fvm_port_registry_unlock();
    if (status != FVM_OK)
    {
        kwerrstr("native output failed");
        result = -1;
    }
    else
    {
        if (written > capacity)
        {
            error(exBounds);
        }
        result = (WORD)written;
        fvm_port_registry_lock();
        copied += (unsigned long)written;
        fvm_port_registry_unlock();
    }
finish:
    ptrdel(D2H(bytes));
    ptrdel(D2H(fd));
    fvm_port_registry_lock();
    pinned -= 2;
    fvm_port_registry_unlock();
    poperror();
    fvm_port_registry_lock();
    staged_bytes -= capacity;
    fvm_port_registry_unlock();
    free(stage);
    return result;
}
