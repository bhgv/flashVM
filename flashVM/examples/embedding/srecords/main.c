/* Binary duplex tutorial: independent tag-3 schemas, fragmented reception and ordinary file I/O. */
#include "flashvm/sbindings.h" /* Direction-local immutable metadata: doc/SRECORD.md. */
#include "flashvm/sstream.h"   /* Caller-buffered fragments: doc/SRECORD.md. */
#include <stdio.h>            /* Optional ordinary fopen/fwrite/fread adapter, not part of codec. */
#include <string.h>           /* Copies schema identifiers into native owned descriptors. */
#define EXAMPLE_CAPACITY 512U /* Bound every tutorial record and receive buffer. */
/* [GC:] The example owns only native bytes/metadata; no VM object, root or runtime exists. */
static int exchange(const char *identity, const unsigned char *data, size_t size, /* Encode and receive one direction using native buffers. */
                    FILE *file) /* One direction. */
{                               /* Each call models a separate receiver/session direction. */
    FVM_SBinding declaration, slots[1], resolved; /* Owned metadata, no transferred pointers. */
    FVM_SBindings incoming; /* Receiver table independent from the opposite direction. */
    FVM_SStream stream;     /* Incremental decoder with explicit caller storage. */
    FVM_SRecord view;       /* Borrowed only until decoder reset or storage reuse. */
    unsigned char payload[EXAMPLE_CAPACITY], wire[EXAMPLE_CAPACITY], /* Separate metadata payload from framed wire output. */
        receive[EXAMPLE_CAPACITY];                  /* Disjoint buffers. */
    size_t payload_size, wire_size, used, position; /* Exact byte counts, never strlen(binary). */
    FVM_SResult status = FVM_S_MORE;              /* Track complete/incomplete decode explicitly. */
    memset(&declaration, 0, sizeof(declaration)); /* Native padding is never written on wire. */
    declaration.tag = 3; /* Choose the same compact numeric tag in both directions. */
    declaration.version = 1; /* Same numeric tag, direction-local meaning. */
    declaration.type_size = /* Measure only the trusted schema-name literal. */
        (unsigned int)strlen(identity); /* This trusted literal fits TYPE_MAX. */
    memcpy(declaration.type, identity, /* Copy the exact identity bytes into owned metadata. */
           declaration.type_size); /* Copy identity, not native schema pointers. */
    if (fvm_sbindings_init(&incoming, slots, 1) != FVM_S_OK) /* Validate caller table initialization before declaring types. */
        return 0; /* Empty receiver table. */
    if (fvm_sbindings_payload_write(&declaration, 1, payload, sizeof(payload), &payload_size) != /* Preflight the whole declaration into its separate payload buffer. */
        FVM_S_OK) /* Continue only when the complete payload is encoded. */
        return 0; /* Metadata body. */
    if (fvm_srecord_write(FVM_SRECORD_CONTROL_TAG, payload, payload_size, wire, sizeof(wire), /* Wrap the declaration with reserved tag zero and checksum. */
                          &wire_size) != FVM_S_OK) /* Receive the exact complete frame byte count. */
        return 0; /* CRC framing. */
    if (fvm_sbindings_apply(&incoming, wire, wire_size, &used) != FVM_S_OK || used != wire_size) /* Accept only a completely consumed, CRC-verified declaration. */
        return 0; /* Verify then copy binding. */
    if (file && fwrite(wire, 1, wire_size, file) != wire_size) /* Check a complete declaration write when file output is enabled. */
        return 0; /* Optional file contains complete binary records. */
    if (fvm_srecord_write(3, data, size, wire, sizeof(wire), &wire_size) != FVM_S_OK) /* Encode the value using its direction-local tag. */
        return 0; /* Value body follows declared schema. */
    if (file && fwrite(wire, 1, wire_size, file) != wire_size) /* Check the full value write separately from declaration output. */
        return 0; /* No text conversion/NUL truncation. */
    if (fvm_sstream_init(&stream, receive, sizeof(receive), EXAMPLE_CAPACITY) != FVM_S_OK) /* Prepare one receive buffer with an explicit body quota. */
        return 0; /* No allocation/I/O. */
    for (position = 0; position < wire_size; /* Offer each wire byte once in transport order. */
         ++position) /* Model a transport delivering one byte at a time. */
    {                /* Real sockets/files may return arbitrary fragment sizes. */
        status = fvm_sstream_push(&stream, wire + position, 1, &view, /* Pass the next byte and receive consumed-input information. */
                                  &used); /* Exactly one input byte offered. */
        if (used != 1 || status != (position + 1 == wire_size ? FVM_S_OK : FVM_S_MORE)) /* Require MORE until precisely the final verified byte. */
            return 0; /* Never use incomplete view. */
    } /* The final successful push publishes one verified borrowed view. */
    if (status != FVM_S_OK || fvm_sbindings_lookup(&incoming, view.tag, &resolved) != FVM_S_OK) /* Resolve the verified tag using only this incoming table. */
        return 0; /* Resolve locally. */
    if (view.size != size || memcmp(view.payload, data, size)) /* Compare payload length and bytes, including embedded NULs. */
        return 0; /* Validate independent expected payload bytes. */
    printf("tag=%lu type=%s bytes=%lu\n", view.tag, resolved.type, /* Print metadata and lengths, never binary data as a string. */
           (unsigned long)view.size); /* Copied metadata, bounded scalars. */
    if (fvm_sstream_reset(&stream) != FVM_S_OK) /* End the borrow explicitly after its last use. */
        return 0; /* Invalidate borrow only after its last use. */
    return 1;     /* All native stack-owned storage is released normally. */
} /* A peer must never merge the two direction-local tables. */
/* Check a file through ordinary fread, counting exactly four CRC-verified framed records. */
static int verify_file( /* Verify records read from an ordinary native file. */
    const char *path) /* Demonstrates byte-file transport, not a mounted remote namespace. */
{                     /* Local filesystem selection belongs to the embedding host. */
    unsigned char bytes[EXAMPLE_CAPACITY]; /* Total tutorial file fits this explicit limit. */
    FVM_SRecord view;               /* Borrows the local file buffer during each iteration. */
    FILE *file = fopen(path, "rb"); /* Native CRT path rules apply, including Windows encoding. */
    size_t size, offset = 0, used, count = 0; /* Independent file-read cursor. */
    int failed;                               /* Preserve read/EOF errors before close. */
    if (!file) /* Handle a failed open without acquiring resources. */
        return 0;                                /* No descriptor acquired on open failure. */
    size = fread(bytes, 1, sizeof(bytes), file); /* Binary read preserves every octet. */
    failed = ferror(file) || !feof(file);        /* Reject unread/truncated-to-capacity content. */
    if (fclose(file) != 0) /* Preserve close errors in addition to fread errors. */
        failed = 1; /* Always close even on input failure. */
    if (failed) /* Reject I/O failure or unread data beyond our bounded storage. */
        return 0;         /* No malformed file is treated as verified. */
    while (offset < size) /* Complete codec consumes exactly one record per call. */
    {                     /* Record boundaries are independent from fread boundaries. */
        if (fvm_srecord_read(bytes + offset, size - offset, EXAMPLE_CAPACITY, &view, &used) != /* Parse exactly one complete frame from the unread file suffix. */
            FVM_S_OK) /* Do not consume damaged or incomplete records. */
            return 0; /* Check CRC first. */
        if (view.tag != ((count & 1U) ? 3UL : FVM_SRECORD_CONTROL_TAG)) /* Check declaration/value ordering in this demonstration transcript. */
            return 0; /* Declaration then value for each direction. */
        offset += used; /* Advance the byte offset by the verified record extent. */
        ++count; /* Advance only after accepted framing. */
    } /* This file concatenates two independent direction transcripts for demonstration only. */
    return count == 4; /* Direction multiplexing needs an explicit transport envelope in a real
                          connection. */
} /* A file of records is not yet a 9P/Styx server. */
/* Console is a thin optional file adapter; all protocol functions remain directly callable from C.
 */
int main(int argc, char **argv) /* No arguments runs entirely in native memory. */
{ /* One optional filename requests binary transcript write/read using ordinary CRT I/O. */
    static const unsigned char real[] = { /* Use an explicit floating-point wire representation. */
        0x3f, 0xf4, 0, 0, /* Most significant binary64 octets precede the low zero octets. */
        0,    0,    0, 0}; /* IEEE754 binary64 big-endian 1.25, no native double cast. */
    static const unsigned char boolean[] = { /* Use an explicit canonical scalar payload for true. */
        0x81};      /* Canonical integer one for the example boolean schema. */
    FILE *file = 0; /* Optional owned file; all exit paths close it. */
    int ok;         /* Combine operation and close failures without skipping cleanup. */
    if (argc > 2) /* Reject extra arguments before opening any file. */
    { /* Report a usage error with no acquired resources. */
        fprintf(stderr, "Usage: %s [transcript-file]\n", argv[0]); /* Describe the one optional native transcript filename. */
        return 2; /* Distinguish bad arguments from runtime I/O failures. */
    } /* No implicit filename. */
    if (argc == 2) /* Only an explicit filename enables file output. */
        file = /* Acquire a binary output file owned by this main routine. */
            fopen(argv[1], "wb"); /* Explicit destination may be overwritten at user's request. */
    if (argc == 2 && !file) /* Handle the explicit output-open failure. */
    { /* No protocol operations or file writes occurred yet. */
        fprintf(stderr, "cannot open transcript\n"); /* Report the failure without claiming protocol success. */
        return 1; /* Return a normal I/O error status. */
    } /* No acquired handle to close. */
    ok = exchange("ieee754.f64.be", real, sizeof(real), /* Model the first direction using the binary64 schema. */
                  file); /* A to B: tag 3 means big-endian binary64. */
    if (ok) /* Run the opposite direction only after the first succeeds. */
        ok = exchange("boolean", boolean, sizeof(boolean), file); /* B to A: tag 3 means boolean. */
    if (file && fclose(file) != 0) /* Check close even when protocol encoding failed. */
        ok = 0; /* Flush/close failures are real output failures. */
    if (ok && argc == 2) /* Read back only a completely written optional transcript. */
        ok = verify_file(argv[1]); /* Ordinary read validates the binary transcript. */
    if (!ok) /* Report any protocol or file verification failure. */
        fprintf(stderr, /* Write a human-readable error, never a binary record. */
                "binary exchange or file verification failed\n"); /* No VM shutdown is required. */
    return ok ? 0 : 1; /* Caller buffers and table metadata never escape this example. */
} /* No SDL, sockets, threads, global tag registry or managed object is required. */
