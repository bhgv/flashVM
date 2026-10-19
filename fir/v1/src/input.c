/* POSIX temporary-file declarations are available on Linux and Termux. */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif
/* Honor an explicit TMPDIR without insecure predictable temporary names.
 * Unlink immediately: the stream alone owns the staged output. */
FILE *fir_temporary(void)
{
#ifndef _WIN32
    const char *directory = getenv("TMPDIR");
    if (directory && *directory) {
        size_t length = strlen(directory);
        char *path;
        int fd;
        FILE *stream;
        if (length > 4096) return NULL;
        path = (char *)malloc(length + 24);
        if (!path) return NULL;
        snprintf(path, length + 24, "%s/fir1-XXXXXX", directory);
        fd = mkstemp(path);
        if (fd < 0) { free(path); return NULL; }
        if (unlink(path)) { close(fd); free(path); return NULL; }
        free(path);
        stream = fdopen(fd, "w+b");
        if (!stream) close(fd);
        return stream;
    }
#endif
    return tmpfile();
}
/* Shared input encoding check, independent of either generated parser. */
#include <stddef.h>
/* UTF-8 validation rejects overlong sequences, surrogates and values above
 * U+10FFFF. Comments may contain Unicode too; NUL is never an EOF shortcut. */
int fir_utf8(const unsigned char *text, size_t n)
{
    size_t i;
    int remaining = 0;
    unsigned low = 0x80, high = 0xbf;
    for (i = 0; i < n; ++i) {
        unsigned c = text[i];
        if (remaining) {
            if (c < low || c > high) return 0;
            --remaining; low = 0x80; high = 0xbf;
        } else if (!c) return 0;
        else if (c < 0x80) continue;
        else if (c >= 0xc2 && c <= 0xdf) remaining = 1;
        else if (c >= 0xe0 && c <= 0xef) {
            remaining = 2;
            low = c == 0xe0 ? 0xa0 : 0x80; high = c == 0xed ? 0x9f : 0xbf;
        } else if (c >= 0xf0 && c <= 0xf4) {
            remaining = 3;
            low = c == 0xf0 ? 0x90 : 0x80; high = c == 0xf4 ? 0x8f : 0xbf;
        } else return 0;
    }
    return !remaining;
}
