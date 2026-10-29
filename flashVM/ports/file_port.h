/* FlashVM: Portable platform adapter interface or implementation: file port.
 * File: ports/file_port.h.
 */

#ifndef FVM_FILE_PORT_H
/* Define FVM FILE PORT H constant or compilation switch. */
#define FVM_FILE_PORT_H
#include "flashvm/flashvm.h"

#ifdef __cplusplus
extern "C"
{
#endif
/* Portable stdio adapter. The channel owns synchronization. Separate read and
       write positions allow rw channels to read from the start and append records. */
typedef struct FVM_PortFile FVM_PortFile;


/* Open borrowed UTF-8 path in r, w, wx or rw mode; NULL selects rw. wx creates
 * exclusively, rejecting every existing path (including symlinks) without
 * truncation. out receives an owned handle or NULL; close on the same serialized
 * owner. Allocation precedes creation; a later I/O failure may leave a new empty
 * file. Native filesystem operations block; no durability/atomic-write guarantee. */
FVM_Result fvm_port_file_open(const char *, const char *, FVM_PortFile **);


/* Close port file. */
void fvm_port_file_close(FVM_PortFile *);


/* Read port file. */
FVM_Result fvm_port_file_read(FVM_PortFile *, void *, size_t, size_t *);


/* Write port file. */
FVM_Result fvm_port_file_write(FVM_PortFile *, const void *, size_t);


/* Read position of port file. */
long fvm_port_file_position(FVM_PortFile *);


/* Restore port file. */
void fvm_port_file_restore(FVM_PortFile *, long);


/* Implement port file available for this subsystem. */
FVM_Result fvm_port_file_available(FVM_PortFile *, int *);


/* Measure an open file without changing logical positions; output changes on success. */
FVM_Result fvm_port_file_size(FVM_PortFile *file, size_t *size);


#define FVM_FILE_SEEK_START 0   /* Offset is relative to the beginning of the file. */
#define FVM_FILE_SEEK_CURRENT 1 /* Offset is relative to the active logical cursor. */
#define FVM_FILE_SEEK_END 2     /* Offset is relative to the measured file extent. */


/* Move a read-only or write-only file cursor; rw append channels are rejected.
 * file is borrowed and externally synchronized; offset/origin select a nonnegative
 * native-long position. On success position receives that offset. Errors preserve
 * both logical cursors and the output. This call can block on native file I/O. */
FVM_Result fvm_port_file_seek(FVM_PortFile *file, long offset, int origin, long *position);


/* Borrow standard input (descriptor 0); report one bounded read without closing it. */
FVM_Result fvm_port_stdio_read(int descriptor, void *bytes, size_t capacity, size_t *received);


/* Borrow standard output/error (1/2); report actual bytes written without closing it. */
FVM_Result fvm_port_stdio_write(int descriptor, const void *bytes, size_t size, size_t *written);
#ifdef __cplusplus
}
#endif
#endif
