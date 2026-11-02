/* Test-only pause inside real file fwrite; no production VM callback is replaced. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int blocked; /* Atomic one-shot flag; only the first ordinary file write pauses. */


/* Forward borrowed fwrite arguments unchanged. For the first ordinary stream,
 * DVM_TEST_FILE_BARRIER names an inherited pipe descriptor. Announce the native
 * wait using write(2), block in read(2), then call the genuine fwrite. This is a
 * controlled native barrier, not evidence of actual slow-disk behavior. */
size_t fwrite(const void *bytes, size_t size, size_t count, FILE *stream)
{
    size_t (*write_file)(const void *, size_t, size_t, FILE *);
    void *symbol = dlsym(RTLD_NEXT, "fwrite");
    const char *selection = getenv("DVM_TEST_FILE_BARRIER");
    if (!symbol || sizeof(write_file) != sizeof(symbol))
    {
        return 0;
    }
    memcpy(&write_file, &symbol, sizeof(write_file));
    if (selection && fileno(stream) > 2 && __sync_bool_compare_and_swap(&blocked, 0, 1))
    {
        char permit;
        ssize_t received;
        const char marker[] = "FILE_NATIVE_BLOCKED\n";
        if (write(2, marker, sizeof(marker) - 1) != sizeof(marker) - 1)
        {
            return 0;
        }
        do
        {
            received = read(atoi(selection), &permit, 1);
        }
        while (received < 0 && errno == EINTR);
        if (received != 1 || permit != '1')
        {
            return 0;
        }
    }
    return write_file(bytes, size, count, stream);
}
