/* Typed managed byte-array admission shared by native input/output. */
#include "lib9.h"
#include "dat.h"
#include "fns.h"
#include "isa.h"
#include "interp.h"
#include "runt.h"
#include "raise.h"
#include "io_buffer.h"

extern Type *TFD; /* Runtime descriptor type, initialized before application execution. */


/* Validate a typed byte-array I/O and keep its native count within the C port range.
 * descriptor/bytes borrow a live FD and mutable array; count is clamped to its extent.
 * Returns the bounded byte count, or -1 for invalid array metadata. */
int dvm_io_span(Sys_FD *descriptor, Array *bytes, WORD count, int *native, uchar **data)
{
    Array view;
    WORD number;
    if (descriptor == H || descriptor == nil || D2H(descriptor)->t != TFD)
    {
        error(exNilref);
    }
    memmove(&number, &descriptor->fd, sizeof(number));
    if (number < 0 || number >= 32)
    {
        kwerrstr("invalid native file descriptor");
        return -1;
    }
    if (bytes == nil || bytes == H || D2H(bytes)->t != &Tarray)
    {
        kwerrstr("invalid byte-array I/O owner");
        return -1;
    }
    memmove(&view, bytes, sizeof(view));
    if (view.t == nil || view.t == H || view.t->size != 1 || view.t->np != 0 || view.len < 0 || view.len > 0x7fffffff ||
        count < 0 || count > 0x7fffffff ||
        (view.len && (!view.data || view.data == H || (uintptr)view.data > (uintptr)-1 - (size_t)view.len)))
    {
        kwerrstr("invalid byte-array I/O span");
        return -1;
    }
    if (count > view.len)
    {
        count = view.len;
    }
    *native = (int)number;
    *data = view.data;
    return (int)count;
}

