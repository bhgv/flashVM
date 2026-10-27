/* Private bridge between the stdio file port and the selected OS filesystem port. */
#ifndef FVM_FILESYSTEM_NATIVE_H
#define FVM_FILESYSTEM_NATIVE_H
#include "filesystem_port.h"


/* Borrow an OS descriptor inside ports only. Name is a captured display basename;
 * query the open object rather than reopening its path. Output changes on success. */
FVM_Result fvm_fs_native_descriptor(int descriptor, const char *name, FVM_FileInfo *output);


/* Read metadata through a borrowed stdio stream; FILE and descriptor details
 * remain private to the OS port. Output changes only on success. */
FVM_Result fvm_fs_native_stream(void *stream, const char *name, FVM_FileInfo *output);


/* Validate a borrowed ordinary UTF-8 path before destructive open. No I/O;
 * URI/provider paths return UNSUPPORTED, never fall back to stdio. */
FVM_Result fvm_fs_native_path(const char *path);


/* Open a validated path in the borrowed binary stdio mode. Returns an owned
 * opaque FILE stream or NULL with native errno; caller closes the stream. */
void *fvm_fs_native_open(const char *path, const char *mode);
#endif
