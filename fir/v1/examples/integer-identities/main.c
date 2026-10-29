/* Compile the sibling tutorial through our source and optimized FIR libraries. */
/* Guide: README.md; the caller owns every stream and receives no VM pointer. */
#include "fir.h" /* Declare source, temporary-stream and pipeline C99 APIs. */
#include <stdio.h> /* Read source and write the selected complete artifact. */
#include <string.h> /* Select exactly the requested dis or object target. */


/* INPUT.b OUTPUT dis|object selects a complete optimized artifact. Return zero
 * on success, one on compilation/I/O failure, two on invalid arguments. Calls
 * are synchronous; no parser, snapshot or stream escapes this process. */
int main(int argc, char **argv) /* Borrow the CLI argument array through return. */
{ /* Begin the sole owner of this example's resources. */
    FILE *source = NULL; /* Own the input stream only after successful fopen. */
    FILE *fir = NULL; /* Own the intermediate FIR stream without a disk pathname. */
    FILE *optimized = NULL; /* Own canonical optimized FIR, matching the explicit CLI opt stage. */
    FILE *artifact = NULL; /* Stage complete Dis/SWO before opening the output path. */
    FILE *output = NULL; /* Own the destination stream after successful compilation. */
    FirRunOptions options = {1, NULL, NULL}; /* Enable the common fixed-point optimizer. */
    FirTask task; /* Select the existing Dis or SWO backend, never invent an ABI. */
    char error[256] = ""; /* Keep a bounded caller-owned compiler diagnostic. */
    unsigned char bytes[4096]; /* Copy bounded chunks without retaining compiler state. */
    size_t count; /* Record each byte count independently of text termination. */
    int failed = 1; /* Treat every incomplete initialization as failure. */
    if (argc != 4 || (strcmp(argv[3], "dis") && strcmp(argv[3], "object"))) /* Validate target before I/O. */
    { /* Reject ambiguous or incomplete invocations. */
        fprintf(stderr, "usage: %s INPUT.b OUTPUT dis|object\n", argv[0]); /* Explain literal argument order. */
        return 2; /* No resource has been acquired. */
    } /* Finish argument validation. */
    task = strcmp(argv[3], "dis") == 0 ? FIR_DIS : FIR_OBJECT; /* Reuse the public target enum. */
    source = fopen(argv[1], "rb"); /* Read source bytes without newline conversion. */
    fir = fir_temporary(); /* Use the caller's TMPDIR when supported by the host. */
    optimized = fir_temporary(); /* Serialize live SSA names before backend value-slot numbering. */
    artifact = fir_temporary(); /* Keep partial backend output away from the destination. */
    if (!source || !fir || !optimized || !artifact) /* Check every acquired stream before dereference. */
    { /* Stop on input or temporary storage failure. */
        goto cleanup; /* Close every successfully initialized resource. */
    } /* End resource admission. */
    if (!fir_source(source, fir, FIR_LIMBO, "integer_identities", error, sizeof(error)) || /* Use our Coco/R frontend. */
        fseek(fir, 0, SEEK_SET) || /* Rewind the completed intermediate stream. */
        !fir_run(FIR_OPT, fir, optimized, &options, error, sizeof(error)) || /* Match the explicit CLI optimizer. */
        fseek(optimized, 0, SEEK_SET) || /* Reparse only live definitions for the same CLI slot numbering. */
        !fir_run(task, optimized, artifact, NULL, error, sizeof(error)) || /* Emit without another optimization. */
        fseek(artifact, 0, SEEK_SET)) /* Rewind only a successfully generated artifact. */
    { /* Preserve the destination when compilation or staging fails. */
        goto cleanup; /* Retain the copied compiler diagnostic until reporting. */
    } /* End compiler processing; no native Limbo bootstrap was invoked. */
    output = fopen(argv[2], "wb"); /* Open the destination only after successful compilation. */
    if (!output) /* Do not copy through an invalid stream. */
    { /* Destination creation can still fail independently of compilation. */
        goto cleanup; /* Report failure and release staged compiler output. */
    } /* End output admission. */
    while ((count = fread(bytes, 1, sizeof(bytes), artifact)) != 0) /* Copy binary data in bounded chunks. */
    { /* Preserve embedded zeros and exact encoded lengths. */
        if (fwrite(bytes, 1, count, output) != count) /* Detect an incomplete destination write. */
        { /* A final I/O failure may leave a prefix; this example does not promise rename atomicity. */
            goto cleanup; /* Let cleanup close the still-owned output. */
        } /* Continue only after writing the complete chunk. */
    } /* Finish copying at EOF or a detected source read failure. */
    failed = ferror(artifact) != 0; /* Distinguish clean EOF from a read error. */
cleanup: /* All branches converge on exactly one release path. */
    if (output && fclose(output)) /* Close the destination and include deferred write failures. */
    { /* Preserve any earlier error rather than replacing it with success. */
        failed = 1; /* Report the failed final flush. */
    } /* Output ownership has ended. */
    if (artifact) /* Accept cleanup after partial initialization. */
    { /* Only this process owns the staged backend stream. */
        fclose(artifact); /* Release its native temporary storage. */
    } /* Artifact ownership has ended. */
    if (optimized) /* Guard the canonical optimized stream after any failure. */
    { /* No backend retains its serialized value names. */
        fclose(optimized); /* Release its native temporary storage. */
    } /* Optimized stream ownership has ended. */
    if (fir) /* Guard the optional intermediate stream. */
    { /* Its parser data has already been released by fir_run. */
        fclose(fir); /* Release the copied FIR text. */
    } /* Intermediate ownership has ended. */
    if (source) /* Guard the optional source stream. */
    { /* No compiler callback retains this input. */
        fclose(source); /* Release the original byte reader. */
    } /* Source ownership has ended. */
    if (failed) /* Emit a concise diagnostic for compilation or host I/O failure. */
    { /* Compiler diagnostics are preferred when available. */
        fprintf(stderr, "%s\n", error[0] ? error : "input/output failure"); /* Never print a partial artifact as text. */
    } /* Finish error reporting. */
    return failed ? 1 : 0; /* Publish only the process status, with no retained resources. */
} /* End the executable C99 embedding example. */
