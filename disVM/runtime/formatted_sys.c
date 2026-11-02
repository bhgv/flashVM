/* Formatting stays on Dis; only completed native bytes leave heap ownership. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "sys_capture.h"
#include "runtime_gc.h"
#include "io_buffer.h"
#include "formatted_sys.h"
#include <stdio.h>

extern Type *TFD; /* Retained typed FD identity, read only on the active heap owner. */

#define FORMAT_BYTES 4096 /* Existing bounded formatting contract, including UTF tail reserve. */

static unsigned long calls;    /* Native port attempts completed on the sole Dis owner. */
static unsigned long releases; /* Attempts which actually released native_world ownership. */
static unsigned int pins;      /* External FD roots currently held by formatted output. */


/* Report counters after managed return/cleanup on the Dis owner. No arguments,
 * allocation or heap mutation; stderr is the existing diagnostic destination. */
void dvm_formatted_statistics(void)
{
    fprintf(stderr,"NATIVEFORMATTED pins=%u calls=%lu releases=%lu\n",pins,calls,releases);
}


/* [GC:] Write borrowed native bytes after formatting. fd is a live mapped owner
 * or NULL for implicit stdout. descriptor is captured on the active owner; bytes
 * and count borrow the caller's bounded C stack buffer through this synchronous
 * call. Return actual acknowledged bytes or -1. Pin failures unwind on the active
 * owner; while released, only native handles and immutable bytes reach OS I/O. */
static WORD formatted_write(Sys_FD *fd, int descriptor, const char *bytes, size_t count)
{
    FVM_PortFile *file = descriptor >= 3 ? dvm_runtime_file(descriptor) : NULL;
    FVM_Result status;
    size_t written = 0;
    int released;
    if (descriptor != 1 && descriptor != 2 && !file)
    {
        kwerrstr("formatted descriptor write failed");
        return -1;
    }
    if (fd)
    {
        ptradd(D2H(fd));
        pins++;
        Setmark(D2H(fd));
    }
    released = dvm_runtime_gc_native_leave();
    if (descriptor == 1 || descriptor == 2)
    {
        status = fvm_port_stdio_write(descriptor,bytes,count,&written);
    }
    else
    {
        status = fvm_port_file_write(file,bytes,count);
        if (status == FVM_OK)
        {
            written = count;
        }
    }
    dvm_runtime_gc_native_enter(released);
    calls++;
    releases += released != 0;
    if (fd)
    {
        ptrdel(D2H(fd));
        pins--;
    }
    if (status != FVM_OK)
    {
        kwerrstr("formatted descriptor write failed");
        return -1;
    }
    if (written > count)
    {
        error(exBounds);
    }
    return (WORD)written;
}


/* Format a mapped variadic Sys.print frame on Dis and write native stdout bytes.
 * frame and its managed arguments remain rooted in the saved native call frame;
 * only the completed C buffer is used while released. Publish one full WORD
 * result after reentry. Oversize/invalid formatting raises the existing error. */
void Sys_print(void *frame)
{
    F_Sys_print call;
    char bytes[FORMAT_BYTES];
    int count;
    WORD result;
    dvm_sys_capture(frame,&call,sizeof(call),offsetof(F_Sys_print,ret),sizeof(*call.ret));
    count = xprint(currun(),frame,(uchar *)frame+offsetof(F_Sys_print,vargs),call.s,bytes,sizeof(bytes));
    if (count < 0 || count >= sizeof(bytes)-UTFmax-2)
    {
        error("experimental stdout message limit");
    }
    result = formatted_write(NULL,1,bytes,(size_t)count);
    memmove(call.ret,&result,sizeof(result));
}


/* Format a mapped Sys.fprint frame before releasing heap ownership. frame owns
 * the typed FD/format/variadic arguments; a separate FD root retains its native
 * handle through the OS wait. Return bytes or -1 in the full mapped WORD slot.
 * Nil/malformed FD and oversize messages use the ordinary managed error path. */
void Sys_fprint(void *frame)
{
    F_Sys_fprint call;
    char bytes[FORMAT_BYTES];
    WORD descriptor;
    WORD result;
    int count;
    dvm_sys_capture(frame,&call,sizeof(call),offsetof(F_Sys_fprint,ret),sizeof(*call.ret));
    if (call.fd == H || call.fd == nil || D2H(call.fd)->t != TFD)
    {
        error(exNilref);
    }
    memmove(&descriptor,&call.fd->fd,sizeof(descriptor));
    count = xprint(currun(),frame,(uchar *)frame+offsetof(F_Sys_fprint,vargs),call.s,bytes,sizeof(bytes));
    if (count < 0 || count >= sizeof(bytes)-UTFmax-2)
    {
        error("experimental formatted FD message limit");
    }
    /* Reject native WORD narrowing rather than mapping a bad FD onto stdout. */
    if (descriptor < 0 || descriptor >= 32)
    {
        kwerrstr("formatted descriptor write failed");
        result = -1;
    }
    else
    {
        result = formatted_write(call.fd,(int)descriptor,bytes,(size_t)count);
    }
    memmove(call.ret,&result,sizeof(result));
}
