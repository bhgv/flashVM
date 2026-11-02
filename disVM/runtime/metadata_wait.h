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


/* Read one bounded native directory batch with the same FD pin/FIFO/lease order.
 * On success entries/count transfer native storage to the caller, which must
 * free it after ACTIVE VM publication or error cleanup. No VM record is built
 * outside heap ownership. Invalid descriptors return an explicit failure. */
FVM_Result dvm_metadata_directory_read(Sys_FD *fd, int descriptor, FVM_FileInfo **entries, size_t *count);


/* Report copied counters after managed executors join. Caller owns the heap;
 * no native worker or collector reads these ACTIVE-only global counters. */
void dvm_metadata_statistics(void);
#endif
