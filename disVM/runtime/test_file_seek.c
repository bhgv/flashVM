/* Exercise shared file positioning, overflow rejection and unchanged append channels. */
#include "file_port.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>


/* argv[1] names disposable storage. Return zero only after all real I/O checks pass;
 * every failure closes the owned handle and removes the temporary file. */
int main(int argc, char **argv)
{
    FVM_PortFile *file = NULL;
    long position = 77;
    size_t received = 0;
    unsigned char bytes[8];
    int status = 1;
    if (argc != 2)
    {
        fprintf(stderr, "file-seek test requires a temporary path\n");
        return 1;
    }
    if (fvm_port_file_open(argv[1], "w", &file) != FVM_OK || fvm_port_file_write(file, "abcd", 4) != FVM_OK ||
        fvm_port_file_seek(file, -2, FVM_FILE_SEEK_CURRENT, &position) != FVM_OK || position != 2 ||
        fvm_port_file_write(file, "Z", 1) != FVM_OK ||
        fvm_port_file_seek(file, 2, FVM_FILE_SEEK_END, &position) != FVM_OK || position != 6 ||
        fvm_port_file_write(file, "!", 1) != FVM_OK)
    {
        goto cleanup;
    }
    fvm_port_file_close(file);
    file = NULL;
    if (fvm_port_file_open(argv[1], "r", &file) != FVM_OK ||
        fvm_port_file_read(file, bytes, sizeof(bytes), &received) != FVM_OK || received != 7 ||
        memcmp(bytes, "abZd\0\0!", 7) != 0 || fvm_port_file_seek(file, -1, FVM_FILE_SEEK_END, &position) != FVM_OK ||
        position != 6 || fvm_port_file_read(file, bytes, 1, &received) != FVM_OK || received != 1 || bytes[0] != '!')
    {
        goto cleanup;
    }
    position = 77;
    if (fvm_port_file_seek(file, LONG_MIN, FVM_FILE_SEEK_CURRENT, &position) != FVM_INVALID_ARGUMENT ||
        position != 77 || fvm_port_file_position(file) != 7 ||
        fvm_port_file_seek(file, LONG_MAX, FVM_FILE_SEEK_CURRENT, &position) != FVM_INVALID_ARGUMENT ||
        position != 77 || fvm_port_file_position(file) != 7 ||
        fvm_port_file_seek(file, 0, -1, &position) != FVM_INVALID_ARGUMENT || position != 77 ||
        fvm_port_file_seek(NULL, 0, FVM_FILE_SEEK_START, &position) != FVM_INVALID_ARGUMENT || position != 77 ||
        fvm_port_file_seek(file, 0, FVM_FILE_SEEK_START, NULL) != FVM_INVALID_ARGUMENT ||
        fvm_port_file_seek(file, 0, FVM_FILE_SEEK_START, &position) != FVM_OK || position != 0 ||
        fvm_port_file_read(file, bytes, 2, &received) != FVM_OK || received != 2 || memcmp(bytes, "ab", 2) != 0)
    {
        goto cleanup;
    }
    /* A legacy restore caller cannot make seek arithmetic overflow with a bad cursor. */
    fvm_port_file_restore(file, LONG_MIN);
    position = 77;
    if (fvm_port_file_seek(file, -1, FVM_FILE_SEEK_CURRENT, &position) != FVM_INVALID_ARGUMENT ||
        position != 77 || fvm_port_file_position(file) != LONG_MIN ||
        fvm_port_file_seek(file, 0, FVM_FILE_SEEK_START, &position) != FVM_OK || position != 0)
    {
        goto cleanup;
    }
    fvm_port_file_close(file);
    file = NULL;
    position = 77;
    if (fvm_port_file_open(argv[1], "rw", &file) != FVM_OK ||
        fvm_port_file_seek(file, 0, FVM_FILE_SEEK_START, &position) != FVM_UNSUPPORTED || position != 77 ||
        fvm_port_file_read(file, bytes, 2, &received) != FVM_OK || received != 2 || memcmp(bytes, "ab", 2) != 0 ||
        fvm_port_file_write(file, "X", 1) != FVM_OK || fvm_port_file_read(file, bytes, 6, &received) != FVM_OK ||
        received != 6 || memcmp(bytes, "Zd\0\0!X", 6) != 0)
    {
        goto cleanup;
    }
    status = 0;
cleanup:
    if (file)
    {
        fvm_port_file_close(file);
    }
    remove(argv[1]);
    if (status)
    {
        fprintf(stderr, "file seek failed: cursor, overflow, sparse write or append semantics\n");
    }
    return status;
}
