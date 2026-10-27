/* Private VM-to-port metadata bridge; no native OS types cross this interface. */
#ifndef DVM_FILESYSTEM_SYS_H
#define DVM_FILESYSTEM_SYS_H
#include "filesystem_port.h"


/* Register the generated Sys.Dir owner map after Sys initialization. */
void dvm_filesystem_initialize(void);


/* Query a runtime-owned file/directory slot; outputs change only on success. */
FVM_Result dvm_runtime_fd_info(int descriptor, FVM_FileInfo *output);


/* Borrow a directory cursor for the current owner call, or return NULL. */
FVM_PortDirectory *dvm_runtime_directory(int descriptor);
#endif
