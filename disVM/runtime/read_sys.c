/* Copyright (c) 2026 bhgv
 * EN: Native I/O with rooted arguments and mutex-protected shared accounting.
 * RU: Нативный I/O с укоренёнными аргументами и общим учётом под мьютексом.
 */
/* Native input stages outside the managed heap, then copies after reentry. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "channel_sync.h"
#include "managed_workers.h"
#include "file_gate.h"
#include "io_buffer.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

extern FVM_Result dvm_runtime_read_stdin(void *bytes, size_t capacity, size_t *received);


#define READ_STAGE_BYTES 65536U /* Maximum native staging allocation per builtin call. */


static size_t staged_bytes;       /* Aggregate native staging, under the registry mutex. */
static size_t peak_staged_bytes;  /* Maximum direct staging bytes, excluding managed roots; updates under registry mutex. */
static unsigned int pinned;      /* External root registrations owned by read callbacks; updates under registry mutex. */
static unsigned long fragments;  /* Completed native read attempts, including EOF/errors; updates under registry mutex. */
static unsigned long copied;     /* Actual bytes committed after managed reentry; updates under registry mutex. */


/* EN: Report counters from a locked snapshot (normally after all producers join). No parameters/result.
 * RU: Сообщить счётчики из снимка под мьютексом (обычно после join производителей). Без параметров/результата. */
void dvm_runtime_read_statistics(void)
{
    fvm_port_registry_lock();
    unsigned int snapshot_pinned = pinned;
    size_t snapshot_staged_bytes = staged_bytes;
    size_t snapshot_peak_staged_bytes = peak_staged_bytes;
    unsigned long snapshot_fragments = fragments;
    unsigned long snapshot_copied = copied;
    fvm_port_registry_unlock();
    fprintf(stderr,"NATIVEINPUT pins=%u staging=%lu peak=%lu fragments=%lu copied=%lu\n",
        snapshot_pinned,(unsigned long)snapshot_staged_bytes,(unsigned long)snapshot_peak_staged_bytes,snapshot_fragments,snapshot_copied);
}


/* [GC:] Read into a bounded native buffer, never a managed byte-array outside
 * the active heap interval. fd/buffer are borrowed native-frame arguments; ret
 * is the captured full result cell. complete selects readn looping. Roots pin
 * the FD, array and its slice-root graph until all native work has ended. Root
 * registration errors unwind with exact cleanup; port failures return partial
 * bytes or -1. No throwing/managed callback occurs between leave and enter. */
static void read_buffer(Sys_FD *fd, Array *buffer, WORD requested, WORD *ret, int complete)
{
    int descriptor;
    int extent;
    uchar *data;
    unsigned char *stage;
    FVM_PortFile *file;
    WORD result = 0;
    size_t capacity;
    volatile int fd_pinned = 0;
    volatile int buffer_pinned = 0;
    if (buffer == H || requested < 0)
    {
        memmove(ret,&result,sizeof(result));
        return;
    }
    extent = dvm_io_span(fd,buffer,requested,&descriptor,&data);
    if (extent <= 0)
    {
        result = extent;
        memmove(ret,&result,sizeof(result));
        return;
    }
    file = descriptor == 0 ? NULL : dvm_runtime_file(descriptor);
    if (descriptor != 0 && !file)
    {
        result = -1;
        memmove(ret,&result,sizeof(result));
        return;
    }
    capacity = (size_t)extent < READ_STAGE_BYTES ? (size_t)extent : READ_STAGE_BYTES;
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
            ptrdel(D2H(buffer));
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
    ptradd(D2H(buffer));
    buffer_pinned = 1;
    fvm_port_registry_lock();
    pinned++;
    fvm_port_registry_unlock();
    Setmark(D2H(buffer));
    while (result < extent)
    {
        size_t received = 0;
        size_t fragment = (size_t)(extent-result);
        FVM_Result status;
        int released;
        DVM_FileTicket ticket;
        DVM_FileGate *gate = dvm_runtime_file_gate(descriptor);
        if (fragment > capacity)
        {
            fragment = capacity;
        }
        /* [GC:] Reserve under the gate mutex, wait after native release. The
         * rooted FD pins the file/gate; finish BEFORE reentry prevents lock inversion. */
        if (file)
        {
            dvm_managed_workers_file_call();
        }
        if (descriptor == 0)
        {
            status = dvm_runtime_read_stdin(stage, fragment, &received);
        }
        else
        {
            released = dvm_file_gate_try_release(gate, &ticket);
            if (released < 0)
            {
                kwerrstr(released == -2 ? "native input cancelled" : "native input needs an executor");
                if (!result)
                {
                    result = -1;
                }
                break;
            }
            status = fvm_port_file_read(file, stage, fragment, &received);
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
            kwerrstr("native input failed");
            if (!result)
            {
                result = -1;
            }
            break;
        }
        if (received > fragment)
        {
            error(exBounds);
        }
        if (!received)
        {
            break;
        }
        /* Nonmoving GC preserves the owner, but revalidate before any copyback.
         * Other actors may mutate aliases while this owner is suspended; the
         * acknowledged fragment commits only after revalidation under the heap. */
        {
            int current_descriptor = -1;
            int current = dvm_io_span(fd,buffer,requested,&current_descriptor,&data);
            if (current < result || current_descriptor != descriptor ||
                received > (size_t)(current-result))
            {
                error("native input destination changed");
            }
        }
        memmove(data+result,stage,received);
        result += (WORD)received;
        fvm_port_registry_lock();
        copied += (unsigned long)received;
        fvm_port_registry_unlock();
        if (descriptor == 0)
        {
            dvm_runtime_input_progress();
        }
        if (!complete)
        {
            break;
        }
    }
    ptrdel(D2H(buffer));
    ptrdel(D2H(fd));
    fvm_port_registry_lock();
    pinned -= 2;
    fvm_port_registry_unlock();
    poperror();
    fvm_port_registry_lock();
    staged_bytes -= capacity;
    fvm_port_registry_unlock();
    free(stage);
    memmove(ret,&result,sizeof(result));
}


/* Read one genuine fragment. frame is a borrowed mapped Sys.read frame; all
 * inputs/results are captured before pinning, blocking or heap release. */
void Sys_read(void *frame)
{
    F_Sys_read call;
    dvm_sys_capture(frame,&call,sizeof(call),offsetof(F_Sys_read,ret),sizeof(*call.ret));
    read_buffer(call.fd,call.buf,call.n,call.ret,0);
}


/* Read requested bytes, EOF or error through bounded native fragments. frame
 * remains rooted throughout; bytes after the acknowledged prefix are untouched. */
void Sys_readn(void *frame)
{
    F_Sys_readn call;
    dvm_sys_capture(frame,&call,sizeof(call),offsetof(F_Sys_readn,ret),sizeof(*call.ret));
    read_buffer(call.fd,call.buf,call.n,call.ret,1);
}
