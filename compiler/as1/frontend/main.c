/* Standalone syntax check driver. One source file per process avoids Coco/R's
 * global scanner/parser state being reused or shared between threads. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <fcntl.h>
#ifdef _WIN32
#include <io.h>
#define open _open
#define close _close
#define read _read
#define lseek _lseek
#else
#include <unistd.h>
#ifndef O_BINARY
#define O_BINARY 0
#endif
#endif
#include "as1s.h"
#include "as1p.h"

/* Error callback owned by the generated parser; no heap allocations are needed. */
extern Error_Func Custom_Error;
static const char *source_path;
/* Coco/R emits diagnostic text alongside the scanner and parser. */
static const char *const error_messages[] = {
#include "as1e.h"
};

/* Reject malformed UTF-8 and embedded NUL before Coco/R's byte-based scanner.
 * Keep the original descriptor so validation and parsing use the same file.
 * Return 0 on success, 1 on encoding error, 2 on read/seek failure. */
static int validate_source(void)
{
    unsigned char bytes[4096];
    int remaining = 0;
    unsigned int low = 0x80, high = 0xbf;
    long offset = 0;
    long count;
    while ((count = (long)read(S_src, bytes, sizeof(bytes))) > 0) {
        long index;
        for (index = 0; index < count; ++index, ++offset) {
            unsigned int ch = bytes[index];
            /* The generated C scanner uses int lengths internally. */
            if (offset >= INT_MAX) {
                fprintf(stderr, "%s: source exceeds scanner length limit\n", source_path);
                return 2;
            }
            if (remaining) {
                if (ch < low || ch > high) goto invalid;
                --remaining;
                low = 0x80;
                high = 0xbf;
            } else if (ch == 0) {
                goto invalid;
            } else if (ch < 0x80) {
                continue;
            } else if (ch >= 0xc2 && ch <= 0xdf) {
                remaining = 1;
            } else if (ch >= 0xe0 && ch <= 0xef) {
                remaining = 2;
                low = ch == 0xe0 ? 0xa0 : 0x80;
                high = ch == 0xed ? 0x9f : 0xbf;
            } else if (ch >= 0xf0 && ch <= 0xf4) {
                remaining = 3;
                low = ch == 0xf0 ? 0x90 : 0x80;
                high = ch == 0xf4 ? 0x8f : 0xbf;
            } else {
                goto invalid;
            }
        }
    }
    if (count < 0 || lseek(S_src, 0L, SEEK_SET) < 0) {
        perror(source_path);
        return 2;
    }
    if (!remaining) return 0;
invalid:
    fprintf(stderr, "%s: invalid UTF-8 or embedded NUL at byte %ld\n", source_path, offset);
    return 1;
}


/* Report byte-based source location and the generated diagnostic number. */
static void report_error(int code, int line, int column, long offset)
{
    const char *message = "syntax error";
    if (code >= 0 && (size_t)code < sizeof(error_messages) / sizeof(error_messages[0])) {
        message = error_messages[code];
    }
    fprintf(stderr, "%s:%d:%d: %s E%d (byte %ld)\n",
            source_path, line, column, message, code, offset);
}

/* Parse one file, closing its descriptor on success or a syntax rejection.
 * Status 0 means syntactically accepted, 1 rejected, 2 command/I/O error. */
int main(int argc, char **argv)
{
    int accepted;
    if (argc == 2 && !strcmp(argv[1], "--version")) {
        puts("as1_parse 0.1.1 (Coco/R syntax frontend)"); return 0;
    }
    if (argc == 2 && !strcmp(argv[1], "--help")) {
        puts("usage: as1_parse SOURCE\n"
             "Syntax only: no type checking, FIR, bytecode or SWF output.\n"
             "Grammar: compiler/as1/frontend/as1.atg (bhgv/coco-r-C).\n"
             "One input per process; UTF-8 validated; byte-based coordinates.\n"
             "Exit: 0 syntax accepted, 1 rejected, 2 CLI/I/O failure.\n"
             "Coverage/build: compiler/as1/README.md and README.ru.md.\n"
             "Numeric source-to-FIR tutorials: build/fir1/fir1_source --help.");
        return 0;
    }
    if (argc != 2) {
        fprintf(stderr, "usage: as1_parse SOURCE\n");
        return 2;
    }
    source_path = argv[1];
    S_src = open(source_path, O_RDONLY | O_BINARY);
    if (S_src < 0) {
        perror(source_path);
        return 2;
    }
    accepted = validate_source();
    if (accepted != 0) {
        close(S_src);
        return accepted;
    }
    Custom_Error = report_error;
    Parse();
    accepted = Successful();
    if (close(S_src) != 0) {
        perror(source_path);
        return 2;
    }
    return accepted ? 0 : 1;
}
