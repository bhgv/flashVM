/* Inspect an object/library through the C API, without launching the SWL CLI. */
#include "swl.h"                                  /* SwlModuleInfo, SwlModuleVisitor and swl_inspect contracts. */
#include <stdio.h>                                /* Regular-file input and host-selected console reporting. */
#include <stdlib.h>                               /* Caller-owned input storage. */

#define EXAMPLE_INPUT_LIMIT (64L * 1024L * 1024L) /* Same input bound as SWL. */


/* The visitor borrows info/name; this tutorial retains only the total count. */
static int describe(void *user, const SwlModuleInfo *info) /* Called after full validation. */
{                                                          /* Begin the host callback. */
    size_t *count = (size_t *)user;                        /* Recover caller-owned state, not library globals. */
    if (printf("%s: %zu code bytes; requirements=%u\n", info->name, info->code_size, info->requirements) <
        0)                                                 /* Reporting policy belongs to the host. */
    {
        return 0;                                          /* Cancel visitation when the host cannot report a module. */
    }
    ++*count;                                              /* Update state only after a successful formatted write. */
    return 1;                                              /* Continue to the next module, if any. */
} /* The borrowed metadata must not be retained here. */


/* Accept a regular file; pipes and changing files need a different host reader. */
int main(int argc, char **argv)  /* No subprocess, compiler or VM dependency. */
{                                /* Begin resource ownership scope. */
    FILE *input;                 /* Host owns and closes the opened file. */
    unsigned char *bytes;        /* Host keeps input alive throughout swl_inspect. */
    long length;                 /* ftell result checked before conversion to size_t. */
    size_t count = 0, read_size; /* Per-invocation state, safe for independent hosts. */
    char error[256];             /* Writable diagnostic storage owned by this invocation. */
    int ok, closed;              /* Preserve both reading and closing failures. */
    if (argc != 2)               /* Exactly one complete SWO1/SWA1 file is inspected. */
    {                            /* Explain invocation errors separately from malformed object files. */
        fprintf(stderr, "usage: inspect_example INPUT.swo|INPUT.swa\n"); /* Host usage. */
        return 2; /* Usage failure, matching the console convention. */
    } /* Required path is present. */
    input = fopen(argv[1], "rb");               /* Binary mode preserves bytes on Windows. */
    if (!input)                                 /* Do not pass a missing file as an empty object. */
    {                                           /* Report the host-side read failure. */
        fprintf(stderr, "cannot open input\n"); /* No library operation has run. */
        return 1;                               /* Nothing needs cleanup yet. */
    } /* The host now owns one open stream. */
    if (fseek(input, 0, SEEK_END) || (length = ftell(input)) <= 0 || length > EXAMPLE_INPUT_LIMIT ||
        fseek(input, 0, SEEK_SET)) /* Bound regular files. */
    {                              /* Empty, oversized or non-seekable input is unsuitable for this reader. */
        fclose(input);             /* Release the stream even on a sizing failure. */
        fprintf(stderr, "cannot size input (expected 1..64 MiB regular file)\n"); /* Policy. */
        return 1;                                                                 /* No input allocation exists. */
    } /* Convert only a positive bounded file length. */
    bytes = (unsigned char *)malloc((size_t)length);  /* Allocate exactly the input size. */
    if (!bytes)                                       /* Allocation failure belongs to the host reader. */
    {                                                 /* Release the only acquired resource. */
        fclose(input);                                /* Avoid leaking the stream on out-of-memory. */
        fprintf(stderr, "input allocation failed\n"); /* Host-owned diagnostic. */
        return 1;                                     /* No library context has been created by the caller. */
    } /* Both the stream and buffer now belong to this function. */
    read_size = fread(bytes, 1, (size_t)length, input); /* Read exact binary bytes. */
    ok = read_size == (size_t)length && !ferror(input); /* Reject short reads. */
    if (ok && fgetc(input) != EOF)                      /* Detect growth beyond the measured length. */
    {
        ok = 0;                                         /* Do not silently inspect a prefix of a changed file. */
    }
    if (ferror(input))                                  /* The extra-byte check can also fail while reading. */
    {
        ok = 0;                                         /* Preserve the read error before closing the stream. */
    }
    closed = fclose(input);                             /* Closing always happens, independently of read status. */
    if (!ok || closed)                                  /* A host read failure must not become a format diagnosis. */
    {                                                   /* Buffer cleanup is still required. */
        free(bytes);                                    /* Release the caller-owned copy. */
        fprintf(stderr, "input read/close failed\n");   /* No callbacks have run. */
        return 1;                                       /* Report the failure to the invoking host. */
    } /* Input storage remains valid without an open file. */
    ok = swl_inspect(bytes, (size_t)length, describe, &count, error, sizeof(error)); /* C API. */
    free(bytes); /* SWL retains nothing after returning, so input is now disposable. */
    if (!ok)     /* Includes format failure or visitor cancellation. */
    {            /* Earlier host callback effects cannot be rolled back on cancellation. */
        fprintf(stderr, "inspection failed: %s\n", error); /* Use supplied error storage. */
        return 1;                                          /* The library never exits the host itself. */
    } /* Every module was reported successfully. */
    if (printf("modules: %zu\n", count) < 0 || fflush(stdout)) /* Catch buffered output errors. */
    {
        return 1; /* Output may be partial; the input was never modified. */
    }
    return 0;     /* All host resources have been released. */
} /* End standalone embedding tutorial. */
