/* FlashVM: bounded DoAction or root-timeline runner for external SWF trace comparisons.
 * This tool does not simulate frame/input scheduling or certify whole-movie compatibility. */
#include "flashvm/avm1.h"
#include "flashvm/standard.h"
#include "flashvm/movieclip.h"
#include "flashvm/swf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Write exact counted trace bytes, followed by one newline, to the chosen output stream. */
static FVM_Result output_trace(FVM_Context *context, FVM_Object *receiver,
                              unsigned int count, const FVM_Value *args,
                              FVM_Value *result, void *userdata)
{
    FILE *stream = (FILE *)userdata;
    size_t length;
    (void)context;
    (void)receiver;
    fvm_value_undefined(result);
    if (count != 1 || args[0].type != FVM_VALUE_STRING)
        return FVM_INVALID_ARGUMENT;
    length = fvm_value_string_size(args);
    if (fwrite(args[0].string, 1, length, stream) != length || fputc('\n', stream) == EOF)
        return FVM_ERROR;
    return FVM_OK;
}

/* Read a bounded SWF into host memory; return no buffer on incomplete input. */
static FVM_Result read_swf(const char *path, unsigned char **data, size_t *size)
{
    FILE *stream;
    long length;
    unsigned char *buffer;
    *data = NULL;
    *size = 0;
    stream = fopen(path, "rb");
    if (!stream)
        return FVM_ERROR;
    if (fseek(stream, 0, SEEK_END) || (length = ftell(stream)) < 0 ||
        (unsigned long)length > FVM_SWF_DEFAULT_LIMIT || fseek(stream, 0, SEEK_SET))
    {
        fclose(stream);
        return FVM_INVALID_ARGUMENT;
    }
    buffer = (unsigned char *)malloc(length ? (size_t)length : 1);
    if (!buffer)
    {
        fclose(stream);
        return FVM_OUT_OF_MEMORY;
    }
    if (fread(buffer, 1, (size_t)length, stream) != (size_t)length || ferror(stream))
    {
        free(buffer);
        fclose(stream);
        return FVM_ERROR;
    }
    if (fclose(stream))
    {
        free(buffer);
        return FVM_ERROR;
    }
    *data = buffer;
    *size = (size_t)length;
    return FVM_OK;
}

/* Accept a bounded positive decimal tick count without signs, suffixes or overflow. */
static int parse_ticks(const char *text, unsigned int *ticks)
{
    unsigned int value = 0;
    const char *cursor = text;
    if (!*cursor)
        return 0;
    while (*cursor)
    {
        if (*cursor < '0' || *cursor > '9')
            return 0;
        value = value * 10 + (unsigned int)(*cursor++ - '0');
        if (value > 10000)
            return 0;
    }
    if (!value)
        return 0;
    *ticks = value;
    return 1;
}

/* Run actions or bounded root timeline ticks and report failures separately from traces. */
int main(int argc, char **argv)
{
    FVM_Config config;
    FVM_Runtime *runtime = NULL;
    FVM_Context *context = NULL;
    unsigned char *data = NULL;
    size_t size = 0;
    FVM_Result status;
    FVM_Object *root = NULL;
    FVM_Value value;
    unsigned int ticks = 0;
    const char *path;
    if (argc == 4 && strcmp(argv[1], "--timeline") == 0 && parse_ticks(argv[2], &ticks))
        path = argv[3];
    else if (argc == 2)
        path = argv[1];
    else
    {
        fprintf(stderr, "Usage: flashvm_reference_swf [--timeline TICKS] INPUT.swf\n");
        return 2;
    }
    status = read_swf(path, &data, &size);
    if (status == FVM_OK)
        status = fvm_config_init(&config);
    if (status == FVM_OK)
    {
        config.instruction_limit = 10000000UL;
        status = fvm_runtime_create(&config, &runtime);
    }
    if (status == FVM_OK)
        status = fvm_context_create(runtime, &context);
    if (status == FVM_OK)
        status = fvm_load_memory(context, data, size);
    if (status == FVM_OK)
        status = fvm_standard_install(context);
    if (status == FVM_OK)
        status = fvm_avm1_set_trace_rooted(context, output_trace, stdout, NULL);
    if (status == FVM_OK && ticks)
        status = fvm_movieclip_load_swf(context, data, size, "root", &root);
    if (status == FVM_OK && ticks)
    {
        /* [GC:] Publish the imported root before advancing any script that can collect it. */
        fvm_value_undefined(&value);
        value.type = FVM_VALUE_OBJECT;
        value.object = root;
        status = fvm_set_global(context, "_root", &value);
        if (status == FVM_OK)
            status = fvm_set_global(context, "_level0", &value);
    }
    free(data);
    if (status == FVM_OK)
        status = ticks ? fvm_movieclip_advance(context, root, ticks) : fvm_swf_execute_actions(context);
    if (fflush(stdout) && status == FVM_OK)
        status = FVM_ERROR;
    if (status != FVM_OK)
        fprintf(stderr, "FlashVM result %d for %s\n", (int)status, path);
    /* [GC:] Context destruction releases VM objects after the final sink invocation. */
    if (context)
        fvm_context_destroy(context);
    if (runtime)
        fvm_runtime_destroy(runtime);
    return status == FVM_OK ? 0 : 1;
}
