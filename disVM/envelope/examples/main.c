/* Build a structural Dis64 fixture through the shared C99 codecs; no VM executes it. */
#include <stdio.h>           /* Own the requested output stream and check final I/O. */
#include "dis64.h"           /* Use the accepted envelope reader, writer and directory API. */
#include "dis-string-pool.h" /* Use the agreed lossless pool and signed Rune helpers. */


/* Encode the bounded fixture before opening the destination. argv[1] is the
 * literal requested output path; the function owns/closes its stream and all
 * buffers. Return nonzero on validation/I/O failure. No VM, heap bridge, async
 * listener, bootstrap compiler or executable module claim is involved. */
int main(int argc, char **argv)            /* Entry point for CLI and relocated SDK examples. */
{                                          /* Begin the synchronous host-tool call. */
    const unsigned int rune = 0xffffffffU; /* Preserve a high-bit Rune payload exactly. */
    unsigned char encoded[8];              /* One count field plus one lossless Rune. */
    unsigned int encoded_bytes = 0;        /* Receive the encoded record size only on success. */
    unsigned char pool_bytes[64];          /* Own the complete serialized string pool. */
    unsigned int pool_size = 0;            /* Receive the complete pool size after preflight. */
    DisStringPoolRecord records[2];        /* Two distinct IDs with equal string content. */
    DisStringPoolView pool;                /* Borrow pool_bytes while checking IDs and order. */
    DisRuneView string;                    /* Borrow the Rune payload returned for the first ID. */
    unsigned int observed = 0;             /* Receive a Rune without converting it to signed int. */
    int order = 12345;                     /* Confirm duplicate content compares equal across IDs. */
    const unsigned char module = 'M';      /* Opaque marker, not real module metadata. */
    const unsigned char code = 'C';        /* Opaque marker, not executable instructions. */
    const unsigned char types = 'T';       /* Opaque marker, not real type descriptors. */
    Dis64Payload payloads[4];              /* Borrow markers and the serialized string pool. */
    unsigned char output[256];             /* Own staged canonical envelope bytes. */
    unsigned int output_bytes = 0;         /* Receive its final serialized extent. */
    Dis64View envelope;                    /* Borrow staged output for structural validation only. */
    Dis64Section section;                  /* Confirm the strings section without reading VM objects. */
    FILE *file;                            /* Own a destination only after every codec check succeeds. */
    int ok;                                /* Track final write and close independently. */
    if (argc != 2)
    {                                      /* Require an explicit filename, never an inferred user path. */
        fprintf(stderr, "usage: dis64_envelope_example output.dis64\n"); /* Show the contract. */
        return 2;                                                        /* Report a command-line error. */
    } /* Finish argument validation. */
    if (!disrunewrite(encoded, sizeof(encoded), &rune, 1, &encoded_bytes))
    {             /* Encode all bits. */
        return 1; /* Stop before touching the destination on codec failure. */
    } /* Finish one record. */
    records[0].data = encoded;        /* The first ID borrows the complete record. */
    records[0].bytes = encoded_bytes; /* Include its Rune-count prefix. */
    records[1] = records[0];          /* Duplicate content remains a separate ordinal ID. */
    if (!disstringpoolwrite(pool_bytes, sizeof(pool_bytes), records, 2, &pool_size) || /* Stage pool. */
        !disstringpoolcheck(pool_bytes, pool_size, 2, &pool) ||                        /* Apply a record limit. */
        !disstringpoolget(&pool, 0, &string) ||                                        /* Resolve a checked ID. */
        !disruneat(&string, 0, &observed) || observed != rune ||                       /* Preserve payload bits. */
        !disstringpoolcompare(&pool, 0, 1, &order) || order != 0)
    {                                                                                  /* Compare contents. */
        return 1; /* No file has been opened on any validation failure. */
    } /* Finish the shared pool API example. */
    payloads[0] = (Dis64Payload){Dis64Module, Dis64Required, &module, 1};             /* Mark required module. */
    payloads[1] = (Dis64Payload){Dis64Code, Dis64Required, &code, 1};                 /* Mark opaque code. */
    payloads[2] = (Dis64Payload){Dis64Types, Dis64Required, &types, 1};               /* Mark opaque types. */
    payloads[3] = (Dis64Payload){Dis64Strings, Dis64Required, pool_bytes, pool_size}; /* Add real pool. */
    if (!dis64write(output, sizeof(output), NULL, 0, payloads, 4, &output_bytes) ||   /* Stage envelope. */
        !dis64check(output, output_bytes, 0, 4, NULL, NULL, &envelope) ||             /* No VM extensions. */
        !dis64section(&envelope, 3, &section) || section.kind != Dis64Strings)
    {                                                                                 /* Check directory. */
        return 1; /* A structurally invalid fixture cannot replace user output. */
    } /* The markers still do not constitute an executable module. */
    file = fopen(argv[1], "wb"); /* Open only the explicit destination after validation. */
    if (!file)
    {                            /* Check opening without using an assertion. */
        return 1;                /* The caller receives an ordinary failure status. */
    } /* Finish the open check. */
    ok = fwrite(output, 1, output_bytes, file) == output_bytes; /* Write complete staged bytes. */
    if (fclose(file) != 0)
    {           /* Close even after a short write; I/O is not transactional. */
        ok = 0; /* Preserve the close failure in the exit status. */
    } /* The stream is now closed on both paths. */
    return ok ? 0 : 1; /* Success means a structural fixture file was written. */
} /* End the host example; no program or new-ABI VM was started. */
