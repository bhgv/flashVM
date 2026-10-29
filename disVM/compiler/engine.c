/* Source/FIR/linker libraries execute here without borrowing either VM heap. */
#include "compiler_engine.h"
#include "fir.h"
#include "swl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Copy one complete staged stream to bounded owned bytes. stream is borrowed;
 * destination/size are caller slots. Return stage on failure without publishing
 * a partial buffer. Rewind/read errors are distinct from an oversized result. */
static DvmCompileStage capture(FILE *stream, unsigned char **destination, size_t *size)
{
    long length;
    unsigned char *bytes;
    if (fflush(stream) || (length = ftell(stream)) < 0 || fseek(stream, 0, SEEK_SET))
    {
        return DVM_COMPILE_RESOURCE;
    }
    if ((unsigned long)length > DVM_COMPILER_MAX_RESULT)
    {
        return DVM_COMPILE_LIMIT;
    }
    bytes = malloc(length ? (size_t)length : 1);
    if (!bytes)
    {
        return DVM_COMPILE_RESOURCE;
    }
    if (fread(bytes, 1, (size_t)length, stream) != (size_t)length || ferror(stream))
    {
        free(bytes);
        return DVM_COMPILE_RESOURCE;
    }
    *destination = bytes;
    *size = (size_t)length;
    return DVM_COMPILE_OK;
}


/* Write a linker's borrowed emission once to the private staged stream user.
 * Return nonzero only after consuming all bytes. No VM callback or publication. */
static int write_swf(void *user, const unsigned char *bytes, size_t size)
{
    return fwrite(bytes, 1, size, user) == size;
}


/* Release result-owned buffers; output is borrowed and may already be empty. */
void dvm_compile_dispose(DvmCompileOutput *output)
{
    if (output)
    {
        free(output->artifact);
        free(output->debug);
        output->artifact = NULL;
        output->debug = NULL;
        output->artifact_size = output->debug_size = 0;
    }
}


/* Execute a complete bounded transaction through the selected libraries.
 * All streams, FIR and linker objects stay private and retire on every path.
 * No borrowed input is retained; only a fully completed result owns byte buffers. */
void dvm_compile(
    int language, int target, int optimize, const unsigned char *source, size_t size, DvmCompileOutput *output)
{
    FILE *input = NULL;
    FILE *fir = NULL;
    FILE *debug = NULL;
    FILE *binary = NULL;
    FILE *object = NULL;
    FILE *canonical = NULL;
    FirModule *module = NULL;
    SwlContext *linker = NULL;
    unsigned char *swo = NULL;
    size_t swo_size = 0;
    FirSourceDiagnostic diagnostic;
    SwlOptions options = {640, 480, 12, 0};
    if (!output)
    {
        return;
    }
    memset(output, 0, sizeof(*output));
    output->offset = -1;
    output->stage = DVM_COMPILE_INVALID;
    if (language < 1 || language > 4 || target < 1 || target > 3 || (optimize != 0 && optimize != 1) ||
        (!source && size))
    {
        snprintf(output->message, sizeof(output->message), "invalid compiler request");
        return;
    }
    if (size > DVM_COMPILER_MAX_SOURCE)
    {
        output->stage = DVM_COMPILE_LIMIT;
        snprintf(output->message, sizeof(output->message), "source exceeds 64 KiB");
        return;
    }
    output->stage = DVM_COMPILE_RESOURCE;
    input = fir_temporary();
    fir = fir_temporary();
    debug = fir_temporary();
    binary = fir_temporary();
    module = malloc(sizeof(*module));
    if (!input || !fir || !debug || !binary || !module || (size && fwrite(source, 1, size, input) != size) ||
        fseek(input, 0, SEEK_SET))
    {
        goto done;
    }
    output->stage = DVM_COMPILE_SOURCE;
    if (!fir_source_debug(input,
                          fir,
                          (FirLanguage)language,
                          "ide_build",
                          output->message,
                          sizeof(output->message),
                          &diagnostic,
                          debug))
    {
        output->kind = diagnostic.kind;
        if (diagnostic.has_location)
        {
            output->line = diagnostic.line;
            output->offset = diagnostic.byte_offset;
            output->length = diagnostic.byte_length;
        }
        goto done;
    }
    output->stage = DVM_COMPILE_VERIFY;
    if (fseek(fir, 0, SEEK_SET) || !fir_parse(fir, module, output->message, sizeof(output->message)) ||
        !fir_verify(module, output->message, sizeof(output->message)) ||
        (optimize && !fir_optimise(module, output->message, sizeof(output->message))) ||
        !fir_verify(module, output->message, sizeof(output->message)))
    {
        goto done;
    }
    /* CLI backends consume serialized FIR. Reparse the same canonical form so
     * removed constant temporaries cannot change value-slot allocation merely
     * because the service emits in-process. Original FIRDBG1 remains separate. */
    if (target != 2)
    {
        output->stage = DVM_COMPILE_RESOURCE;
        canonical = fir_temporary();
        if (!canonical || !fir_print(canonical, module) || fseek(canonical, 0, SEEK_SET))
        {
            goto done;
        }
        output->stage = DVM_COMPILE_VERIFY;
        if (!fir_parse(canonical, module, output->message, sizeof(output->message)) ||
            !fir_verify(module, output->message, sizeof(output->message)))
        {
            goto done;
        }
    }
    output->stage = DVM_COMPILE_BACKEND;
    if (target == 1)
    {
        if (!fir_dis(binary, module, output->message, sizeof(output->message)))
        {
            goto done;
        }
    }
    else if (target == 2)
    {
        if (!fir_print(binary, module))
        {
            output->stage = DVM_COMPILE_RESOURCE;
            goto done;
        }
    }
    else
    {
        object = fir_temporary();
        linker = swl_create(&options);
        if (!object || !linker)
        {
            output->stage = DVM_COMPILE_RESOURCE;
            goto done;
        }
        if (!fir_object(object, module, output->message, sizeof(output->message)))
        {
            goto done;
        }
        output->stage = capture(object, &swo, &swo_size);
        if (output->stage != DVM_COMPILE_OK)
        {
            goto done;
        }
        output->stage = DVM_COMPILE_BACKEND;
        if (!swl_add_object(linker, swo, swo_size, output->message, sizeof(output->message)) ||
            !swl_emit(linker, write_swf, binary, output->message, sizeof(output->message)))
        {
            goto done;
        }
    }
    output->stage = capture(binary, &output->artifact, &output->artifact_size);
    if (output->stage == DVM_COMPILE_OK)
    {
        output->stage = capture(debug, &output->debug, &output->debug_size);
    }
done:
    if (output->stage != DVM_COMPILE_OK)
    {
        dvm_compile_dispose(output);
        if (!output->message[0])
        {
            snprintf(output->message,
                     sizeof(output->message),
                     "compiler staging or output failed at stage %d",
                     output->stage);
        }
    }
    else
    {
        output->message[0] = 0;
    }
    if (input)
    {
        fclose(input);
    }
    if (fir)
    {
        fclose(fir);
    }
    if (debug)
    {
        fclose(debug);
    }
    if (binary)
    {
        fclose(binary);
    }
    if (object)
    {
        fclose(object);
    }
    if (canonical)
    {
        fclose(canonical);
    }
    swl_destroy(linker);
    free(swo);
    free(module);
}
