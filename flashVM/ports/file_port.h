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


/* Open port file. */
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


/* Borrow standard input (descriptor 0); report one bounded read without closing it. */
FVM_Result fvm_port_stdio_read(int descriptor, void *bytes, size_t capacity, size_t *received);


/* Borrow standard output/error (1/2); report actual bytes written without closing it. */
FVM_Result fvm_port_stdio_write(int descriptor, const void *bytes, size_t size, size_t *written);
#ifdef __cplusplus
}
#endif
#endif
