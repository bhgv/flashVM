/* Build and semantically check the agreed pilot42 through shared C99 writers. */
#include <string.h>      /* Compare the explicit initialized-fixture option. */
#include <stdio.h>       /* Own and close the explicit destination stream. */
#include "isa.h"         /* Include existing opcode/mode numbers exactly once. */
#include "dis64-pilot.h" /* Reuse the upstream whole-pilot semantic gate. */


/* argv[1] names the output file; argc must be two. All buffers and the final
 * stream are local owners, borrowed codec views expire on return. Stage/check
 * every payload before opening output; return nonzero on codec or final I/O
 * failure. A short write is not a filesystem transaction. No VM allocation,
 * scheduler, EXIT handler, bootstrap compiler or frame clock is started. */
int main(int argc, char **argv)                           /* Entry for both source and relocated SDK builds. */
{                                                         /* Begin a bounded synchronous C99 example. */
    const unsigned char map = 0;                          /* No managed references in scalar data/frame. */
    const unsigned int name[] = {80, 105, 108, 111, 116}; /* Lossless Rune spelling Pilot. */
    Dis64ModuleInfo metadata = {0, 1, 4, 0, 0, 1};        /* Name0, executable, data4/type0, entry0/frame1. */
    Dis64Type descriptors[2] = {{4, 4, &map, 1}, {64, 8, &map, 1}}; /* Full map8 for both objects. */
    unsigned char module[24];                                       /* Exactly the conditional metadata extent. */
    unsigned char types[22];                                        /* Count plus two size/alignment/map descriptors. */
    unsigned char record[24];                                       /* Rune count plus five BE u32 payloads. */
    unsigned char strings[28];                       /* Pool count followed by the complete name record. */
    unsigned char code[10];                          /* Instruction count and MOVW42/EXIT records. */
    unsigned char data[16] = {0};                    /* Own either count0 or one scalar initializer. */
    unsigned char output[192];                       /* Capacity for either shared hand-written golden. */
    unsigned int databytes = 0;                      /* Exact initializer payload extent. */
    Dis64Initializer initial = {0, Dis64Int, 0, 40}; /* Write int32 forty to offset zero. */
    int initialized = argc == 3 && !strcmp(argv[2], "--initialized"); /* Explicit opt-in second golden. */
    unsigned int modulebytes = 0;                                     /* Written only after metadata preflight. */
    unsigned int typebytes = 0;                                       /* Written only after all descriptors validate. */
    unsigned int recordbytes = 0;                                     /* Complete non-nil Rune record length. */
    unsigned int stringbytes = 0;                                     /* Exact complete pool length. */
    unsigned int bytes = 0;        /* Final envelope extent after successful serialization. */
    DisStringPoolRecord item;      /* Borrow record while constructing the pool. */
    Dis64Payload sections[5];      /* Borrow each separately staged payload. */
    Dis64Pilot pilot;              /* Borrow final output only after whole-file validation. */
    Dis64Instruction instruction;  /* Confirm the decoded source immediate. */
    FILE *file;                    /* Own a stream only after every semantic check succeeds. */
    int ok;                        /* Retain write and close errors independently. */
    if (argc != 2 && !initialized) /* Require a literal caller-selected destination. */
    {                              /* Diagnose usage without touching a file. */
        fprintf(stderr, "usage: dis64_pilot_example output.dis64 [--initialized]\n"); /* Explain exact argv. */
        return 2; /* Return the command-line error status. */
    } /* Finish argument validation. */
    if (!dis64modulewrite(module, sizeof(module), &metadata, &modulebytes) || /* Stage metadata. */
        !dis64typeswrite(types, sizeof(types), descriptors, 2, &typebytes) || /* Stage complete maps. */
        !disrunewrite(record, sizeof(record), name, 5, &recordbytes))         /* Preserve all name Runes. */
    {             /* Stop before opening output on any payload failure. */
        return 1; /* The destination remains untouched. */
    } /* Finish the metadata/type/name writers. */
    item = (DisStringPoolRecord){record, recordbytes};                         /* Borrow the already complete record. */
    if (!disstringpoolwrite(strings, sizeof(strings), &item, 1, &stringbytes)) /* Emit ordinal ID0. */
    {             /* Reject a pool construction failure before output publication. */
        return 1; /* No stream has been opened. */
    } /* Finish the pool writer. */
    dis64put32(code, 2);                    /* Count decoded Dis instructions, not code bytes. */
    code[4] = initialized ? IADDW : IMOVW;  /* Select existing scalar move or arithmetic, never a new opcode. */
    code[5] = SRC(AIMM) | DST(AMP) | AXNON; /* Immediate source, module-data destination, no middle. */
    if (disoperand(code + 6, 1, initialized ? 2 : 42) != 1 ||
        disoperand(code + 7, 1, 0) != 1)    /* Checked signed30 operands. */
    {                                       /* Preserve codec failure instead of assuming one-byte capacity. */
        return 1;                           /* Do not produce a malformed program file. */
    } /* Finish source and destination framing. */
    code[8] = IEXIT;                         /* Decode-only terminal instruction in current acceptance. */
    code[9] = SRC(AXXX) | DST(AXXX) | AXNON; /* EXIT has no source, destination or middle. */
    if (!dis64initializerswrite(data, sizeof(data), &descriptors[0], &initial, initialized ? 1 : 0, &databytes))
    {             /* Use the shared preflight rather than hand-emitting initializer records. */
        return 1; /* No destination has been opened. */
    } /* Default fixture keeps count0; initialized fixture contains int32 forty. */
    sections[0] = (Dis64Payload){Dis64Module, Dis64Required, module, modulebytes};   /* Link metadata. */
    sections[1] = (Dis64Payload){Dis64Code, Dis64Required, code, sizeof(code)};      /* Link real code. */
    sections[2] = (Dis64Payload){Dis64Types, Dis64Required, types, typebytes};       /* Link real descriptors. */
    sections[3] = (Dis64Payload){Dis64Data, Dis64Required, data, databytes};         /* Link default initialization. */
    sections[4] = (Dis64Payload){Dis64Strings, Dis64Required, strings, stringbytes}; /* Link actual name pool. */
    if (!dis64write(output, sizeof(output), NULL, 0, sections, 5, &bytes) ||         /* Stage canonical envelope. */
        !dis64pilotcheck(output, bytes, 4096, 1024, &pilot) ||          /* Check all cross-section references. */
        !dis64codeget(&pilot.code, pilot.module.entry, &instruction) || /* Resolve checked Dis-PC. */
        instruction.op != (initialized ? IADDW : IMOVW) ||
        instruction.source != (initialized ? 2 : 42))                   /* Confirm the fixture's first instruction. */
    {             /* Semantic validation never starts a scheduler or calls an EXIT handler. */
        return 1; /* Leave existing destination untouched on validation failure. */
    } /* Every shared writer and whole-pilot reader has succeeded. */
    file = fopen(argv[1], "wb"); /* Open only the explicit requested destination. */
    if (!file)                   /* Check host I/O independently of codec acceptance. */
    {                            /* Report failure without inventing a VM diagnostic. */
        return 1;                /* No stream ownership remains. */
    } /* Finish the open check. */
    ok = fwrite(output, 1, bytes, file) == bytes; /* Copy the fully staged checked image. */
    if (fclose(file) != 0)                        /* Always close even after a short write. */
    {                                             /* Retain a late close failure in the exit code. */
        ok = 0;                                   /* Do not report successful publication after failed I/O. */
    } /* End stream ownership; borrowed views expire at return. */
    return ok ? 0 : 1; /* Success means a checked pilot file, not an executed module. */
} /* End the example without changing the active NC64 VM ABI. */
