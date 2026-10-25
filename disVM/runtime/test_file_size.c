/* Test the actual shared file port: measuring a file must preserve reader position. */
#include "file_port.h"
#include <stdio.h>
#include <string.h>

/* Write binary input, measure before/after reads, and close the owned port handle.
 * argv[1] names caller-owned test storage. Returns nonzero with a visible error on failure. */
int main(int argc, char **argv)
{
    FVM_PortFile *file = NULL;
    const unsigned char source[] = {65, 0, 66};
    unsigned char result[3];
    size_t size = 99;
    size_t received = 0;
    int status = 1;
    if (argc != 2)
    {
        fprintf(stderr, "file-size test requires a temporary file path\n");
        return 1;
    }
    if (fvm_port_file_open(argv[1], "w", &file) != FVM_OK ||
        fvm_port_file_write(file, source, sizeof(source)) != FVM_OK)
    {
        goto cleanup;
    }
    fvm_port_file_close(file);
    file = NULL;
    if (fvm_port_file_open(argv[1], "r", &file) != FVM_OK || fvm_port_file_size(file, &size) != FVM_OK ||
        size != sizeof(source) || fvm_port_file_read(file, result, 1, &received) != FVM_OK || received != 1 ||
        result[0] != 65 || fvm_port_file_size(file, &size) != FVM_OK || size != sizeof(source) ||
        fvm_port_file_read(file, result, 2, &received) != FVM_OK || received != 2 || result[0] != 0 ||
        result[1] != 66 || fvm_port_file_size(NULL, &size) != FVM_INVALID_ARGUMENT || size != sizeof(source))
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
        fprintf(stderr, "file extent query changed stream position, contents or failure output\n");
    }
    return status;
}
