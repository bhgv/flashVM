/* Exercise actual versioned scalar signatures and ordinal import aliases. */
#include <stdio.h>          /* Report an ordinary host-tool result. */
#include <string.h>         /* Verify that rejected writes preserve existing bytes. */
#include "dis64-bindings.h" /* Shared structural codecs; no runtime calls. */


/* Own all independent buffers; no arguments or files are used. Return nonzero
 * on any codec/alias/atomicity mismatch. Borrowed views expire at return.
 * This example resolves no dependencies, allocates no VM and invokes no callee. */
int main(void)                                               /* SDK and standalone C99 entry point. */
{                                                            /* Begin bounded structural interface validation. */
    const unsigned int module_name = 'M';                    /* One lossless module-name Rune. */
    const unsigned int symbol_name = 'f';                    /* One lossless symbol-name Rune. */
    const unsigned int parameters[2] = {Dis64Int, Dis64Big}; /* Scalar parameters in declared order. */
    unsigned char names[2][8];                               /* Independent complete Rune records. */
    unsigned char pool_data[20];                             /* Count and two single-Rune records. */
    unsigned char signatures[2][20];                         /* Full version/result/count/parameter records. */
    unsigned char output[128];                               /* Stage binding-table bytes without heap publication. */
    unsigned char saved[128];                                /* Snapshot every byte before a deliberate rejection. */
    unsigned int recordbytes;                                /* Exact Rune record extent. */
    unsigned int bytes;                                      /* Exact serialized extent returned on success. */
    unsigned int written;                                    /* Writer extent preserved on failure. */
    DisStringPoolRecord records[2];                          /* Borrow each complete name record. */
    DisStringPoolView pool;                                  /* Borrow the checked immutable name pool. */
    Dis64Signature signature;                                /* Borrow the first canonical scalar signature. */
    Dis64Binding imports[2];                                 /* Distinct ordinal aliases for one module/symbol. */
    Dis64Bindings bindings;                                  /* Borrow the complete checked import table. */
    Dis64Binding alias;                                      /* Resolve ordinal one without creating a call target. */
    if (!disrunewrite(names[0], 8, &module_name, 1, &recordbytes) || /* Encode M. */
        !disrunewrite(names[1], 8, &symbol_name, 1, &recordbytes))   /* Encode f. */
    {             /* A failed record cannot enter a binding context. */
        return 1; /* All storage remains owner-local. */
    } /* Complete both names before building their pool. */
    records[0] = (DisStringPoolRecord){names[0], recordbytes};                       /* Module name ID0. */
    records[1] = (DisStringPoolRecord){names[1], recordbytes};                       /* Symbol name ID1. */
    if (!disstringpoolwrite(pool_data, sizeof(pool_data), records, 2, &bytes) ||     /* Serialize names. */
        !disstringpoolcheck(pool_data, bytes, 2, &pool) ||                           /* Check full extent. */
        !dis64signaturewrite(signatures[0], 20, Dis64Bool, parameters, 2, &bytes) || /* bool(int,big). */
        !dis64signaturecheck(signatures[0], bytes, 2, &signature)) /* Validate inclusive versioned extent. */
    {             /* Unknown kinds or malformed signatures fail before a binding write. */
        return 1; /* No native function or pointer has been resolved. */
    } /* The signature is structural evidence only. */
    imports[0] = (Dis64Binding){0, 1, 0, 0, signature}; /* Import M.f with exact signature. */
    imports[1] = imports[0];                            /* Keep a second ordinal alias with identical content. */
    memset(output, 0x5a, sizeof(output));               /* Make failure preservation observable. */
    if (!dis64bindingswrite(output, sizeof(output), 0, imports, 2, &pool, NULL, 0, 2, &written) || /* Emit imports. */
        !dis64bindingscheck(output, written, 0, &pool, NULL, 0, 2, 2, &bindings) ||                /* Check aliases. */
        !dis64bindingget(&bindings, 1, &alias) || alias.module != 0 || alias.symbol != 1)          /* Resolve ID1. */
    {             /* Identical signatures are allowed as ordinal aliases. */
        return 1; /* Preserve failure as a normal host status. */
    } /* No call ABI or root coverage is implied by successful decoding. */
    memcpy(saved, output, sizeof(saved)); /* Retain the successful bytes for the negative case. */
    if (!dis64signaturewrite(signatures[1], 20, Dis64Int, parameters, 2, &bytes) || /* Change result kind. */
        !dis64signaturecheck(signatures[1], bytes, 2, &imports[1].signature))       /* Keep it independently valid. */
    {             /* A different valid signature still conflicts at the same import name. */
        return 1; /* Stop without replacing the earlier binding table. */
    } /* Test pair ambiguity through the shared writer. */
    bytes = written; /* Keep the extent sentinel independent of the output bytes. */
    if (dis64bindingswrite(output, sizeof(output), 0, imports, 2, &pool, NULL, 0, 2, &written) || /* Must reject. */
        written != bytes || memcmp(saved, output, sizeof(output))) /* Both outputs must stay unchanged. */
    {             /* Conflicting aliases must never publish a partial table. */
        return 1; /* Report the broken structural contract. */
    } /* Every borrowed view expires below. */
    puts("PASS scalar signatures/import aliases; no call execution"); /* State the actual scope. */
    return 0; /* No scheduler, heap owner or callee was started. */
} /* End the independently buildable C99 example. */
