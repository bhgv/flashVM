/* Internal typed metadata waits; caller includes the generated native runt.h. */
#ifndef DVM_METADATA_WAIT_H
#define DVM_METADATA_WAIT_H
#include "filesystem_port.h"


/* Copy borrowed validated UTF-8 path into native storage before releasing the
 * ACTIVE heap turn. output receives copied metadata only on success. No managed
 * pointer is followed while suspended; native staging is freed after reentry. */
FVM_Result dvm_metadata_path_info(const char *path, FVM_FileInfo *output);


/* Borrow the mapped FD and checked descriptor while ACTIVE, pin FD, reserve its
 * file/directory gate, release heap ownership, query native metadata, finish gate
 * BEFORE reentry and unpin. output changes only on success. Invalid slots return
 * an explicit status; root-registration errors use the ordinary managed unwind. */
FVM_Result dvm_metadata_fd_info(Sys_FD *fd, int descriptor, FVM_FileInfo *output);


/* Read a bounded batch, then publish it under NativeWorld while still owning
 * the cursor ticket. entries/count are borrowed by publish, which may raise.
 * The adapter frees the batch and releases the ticket on every path. */
FVM_Result dvm_metadata_directory_read(Sys_FD *fd, int descriptor, FVM_FileInfo **entries,
    size_t *count, int (*publish)(void *), void *argument);


/* Report copied counters after managed executors join. Caller owns the heap;
 * no native worker or collector reads these ACTIVE-only global counters. */
void dvm_metadata_statistics(void);
#endif
