/* Immutable bounded native output snapshots outside the managed heap interval. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "runtime_gc.h"
#include "managed_workers.h"
#include "file_gate.h"
#include "io_buffer.h"
extern FVM_Result dvm_runtime_write_stdio(int descriptor, const void *bytes,
    size_t capacity, size_t *written, int *released);

#include <stdio.h>
#include <stdlib.h>

#define WRITE_STAGE_BYTES 65536U /* Maximum native output snapshot per call. */

static size_t staged_bytes;      /* Native snapshot storage currently owned by Dis. */
static size_t peak_staged_bytes; /* Maximum direct output storage before return. */
static unsigned int pinned;     /* External FD/source-array root registrations. */
static unsigned long fragments; /* Completed native port attempts. */
static unsigned long copied;    /* Actual bytes acknowledged by the native port. */


/* Report native output resources for explicit --stats. No arguments/result;
 * called on the Dis owner after managed return or cleanup, never by collectors. */
void dvm_runtime_write_statistics(void)
{
    fprintf(stderr,"NATIVEOUTPUT pins=%u staging=%lu peak=%lu fragments=%lu copied=%lu\n",
        pinned,(unsigned long)staged_bytes,(unsigned long)peak_staged_bytes,fragments,copied);
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
    staged_bytes += capacity;
    if (staged_bytes > peak_staged_bytes)
    {
        peak_staged_bytes = staged_bytes;
    }
    if (waserror())
    {
        if (buffer_pinned)
        {
            ptrdel(D2H(bytes));
            pinned--;
        }
        if (fd_pinned)
        {
            ptrdel(D2H(fd));
            pinned--;
        }
        staged_bytes -= capacity;
        free(stage);
        nexterror();
    }
    ptradd(D2H(fd));
    fd_pinned = 1;
    pinned++;
    Setmark(D2H(fd));
    ptradd(D2H(bytes));
    buffer_pinned = 1;
    pinned++;
    Setmark(D2H(bytes));
    memmove(stage,data,capacity);
    /* [GC:] Reserve under heap exclusion, wait after native release. The
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
    fragments++;
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
        copied += (unsigned long)written;
    }
finish:
    ptrdel(D2H(bytes));
    ptrdel(D2H(fd));
    pinned -= 2;
    poperror();
    staged_bytes -= capacity;
    free(stage);
    return result;
}
