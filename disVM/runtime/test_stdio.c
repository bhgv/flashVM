/* Exercise the shared standard-stream port with real inherited OS descriptors. */
#include "file_port.h"
#include <stdio.h>
#include <string.h>

/* Echo bytes or write a stderr marker, preserving actual short read/write counts.
 * Optional argv[1]=stderr selects the error stream. Returns nonzero for a failed contract. */
int main(int argc, char **argv)
{
    unsigned char bytes[4096];
    size_t received = 99;
    size_t written = 99;
    size_t offset;
    if (fvm_port_stdio_read(1, bytes, 1, &received) != FVM_INVALID_ARGUMENT || received != 99 ||
        fvm_port_stdio_write(0, bytes, 1, &written) != FVM_INVALID_ARGUMENT || written != 99)
    {
        fprintf(stderr, "invalid standard descriptor was accepted\n");
        return 1;
    }
    if (argc == 2 && strcmp(argv[1], "stderr") == 0)
    {
        return fvm_port_stdio_write(2, "native-error", 12, &written) == FVM_OK && written == 12 ? 0 : 1;
    }
    for (;;)
    {
        if (fvm_port_stdio_read(0, bytes, sizeof(bytes), &received) != FVM_OK)
        {
            return 1;
        }
        if (!received)
        {
            return 0;
        }
        offset = 0;
        while (offset < received)
        {
            if (fvm_port_stdio_write(1, bytes + offset, received - offset, &written) != FVM_OK || !written)
            {
                return 1;
            }
            offset += written;
        }
    }
}
