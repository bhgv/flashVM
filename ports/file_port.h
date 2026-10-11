#ifndef FVM_FILE_PORT_H
#define FVM_FILE_PORT_H
#include "flashvm/flashvm.h"
#ifdef __cplusplus
extern "C"
{
#endif
    /* Portable stdio adapter. The channel owns synchronization. Separate read and
       write positions allow rw channels to read from the start and append records. */
    typedef struct FVM_PortFile FVM_PortFile;
    FVM_Result fvm_port_file_open(const char *, const char *, FVM_PortFile **);
    void fvm_port_file_close(FVM_PortFile *);
    FVM_Result fvm_port_file_read(FVM_PortFile *, void *, size_t, size_t *);
    FVM_Result fvm_port_file_write(FVM_PortFile *, const void *, size_t);
    long fvm_port_file_position(FVM_PortFile *);
    void fvm_port_file_restore(FVM_PortFile *, long);
    FVM_Result fvm_port_file_available(FVM_PortFile *, int *);
#ifdef __cplusplus
}
#endif
#endif
