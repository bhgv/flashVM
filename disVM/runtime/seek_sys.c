/* Seek follows the same rooted native-wait and per-handle FIFO as read/write. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "file_gate.h"
#include "file_port.h"
#include "io_buffer.h"
#include "managed_workers.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

extern Type *TFD; /* Generated typed FD identity, inspected only on ACTIVE heap owner. */


/* [GC:] frame borrows Sys.seek's mapped FD/big offset and result cell. Validate
 * narrowing and pin FD before reserving/releasing; native work sees only copied
 * scalars and the retained native file/gate. Finish FIFO before heap reentry.
 * Return the new native position or -1 as a full big. No managed access/throw
 * occurs outside the heap; nil/malformed FD raises the ordinary managed error. */
void Sys_seek(void *frame)
{
    F_Sys_seek call;
    FVM_PortFile *file;
    DVM_FileGate *gate;
    FVM_Result status;
    LONG result = -1;
    long position;
    DVM_FileTicket ticket;
    int descriptor;
    int released;
    dvm_sys_capture(frame, &call, sizeof(call), offsetof(F_Sys_seek, ret), sizeof(*call.ret));
    if (call.fd == H || call.fd == nil || D2H(call.fd)->t != TFD)
    {
        error(exNilref);
    }
    if (call.fd->fd < 3 || call.fd->fd >= 32)
    {
        kwerrstr("seek requires an ordinary live file");
    }
    else if (call.start < FVM_FILE_SEEK_START || call.start > FVM_FILE_SEEK_END ||
        call.off < LONG_MIN || call.off > LONG_MAX)
    {
        kwerrstr("invalid seek origin or native offset range");
    }
    else
    {
        descriptor = (int)call.fd->fd;
        file = dvm_runtime_file(descriptor);
        gate = dvm_runtime_file_gate(descriptor);
        if (!file || !gate)
        {
            kwerrstr("seek requires an ordinary live file");
        }
        else
        {
            ptradd(D2H(call.fd));
            Setmark(D2H(call.fd));
            dvm_managed_workers_file_call();
            released = dvm_file_gate_try_release(gate, &ticket);
            if (released < 0)
            {
                ptrdel(D2H(call.fd));
                kwerrstr(released == -2 ? "native seek cancelled" : "native seek needs an executor");
                memmove(call.ret, &result, sizeof(result));
                return;
            }
            status = fvm_port_file_seek(file, (long)call.off, (int)call.start, &position);
            if (!dvm_file_gate_leave(gate, &ticket))
            {
                fprintf(stderr, "disvm: native seek admission lost\n");
                exit(1);
            }
            dvm_runtime_gc_native_enter(released);
            ptrdel(D2H(call.fd));
            if (status == FVM_OK)
            {
                result = position;
            }
            else
            {
                kwerrstr("file seek failed");
            }
        }
    }
    memmove(call.ret, &result, sizeof(result));
}
