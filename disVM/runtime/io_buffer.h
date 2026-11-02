/* Internal typed buffer admission; caller includes matching native runt.h first. */
#ifndef DVM_IO_BUFFER_H
#define DVM_IO_BUFFER_H
#include "file_port.h"

/* Preserve a formatted native error in the bound owner environment. format and
 * arguments are borrowed; call only while heap ownership is active. */
void kwerrstr(char *format, ...);

/* [GC:] Admit a borrowed live FD and byte-array span on the active Dis owner.
 * count clamps to array length; native/data outputs change only on success.
 * Returns bounded bytes or -1; malformed descriptor owners raise normally. */
int dvm_io_span(Sys_FD *descriptor, Array *bytes, WORD count, int *native, uchar **data);

/* Borrow a live native file slot while its managed FD remains rooted. Null
 * rejects standard descriptors and non-file slots. Active owner calls only. */
FVM_PortFile *dvm_runtime_file(int descriptor);

/* Write a bounded immutable native snapshot after validating/pinning fd/bytes.
 * count is clamped; return actual bytes acknowledged or -1. Active Dis owner
 * only; no managed data access occurs during the native port operation. */
WORD dvm_runtime_write(Sys_FD *fd, Array *bytes, WORD count);

/* Report owner-private output resources after managed return/cleanup. */
void dvm_runtime_write_statistics(void);

/* Report owner-private native read resources/counters for explicit --stats.
 * Called after managed return/cleanup, never by UI or a collector. */
void dvm_runtime_read_statistics(void);

/* Reset the busy-work budget after actual stdin bytes, on the active owner. */
void dvm_runtime_input_progress(void);
#endif
