/* Optional acceptance host using only public FlashVM C headers. The FIR
 * compiler itself never links FlashVM. Trace text is copied synchronously. */
#include "flashvm/flashvm.h"
#include "flashvm/avm1.h"
#include "flashvm/extensions.h"
#include "flashvm/value.h"
#include <stdio.h>
#include <stdlib.h>


static FVM_Result trace(FVM_Context *context,
                        FVM_Object *receiver,
                        unsigned int count,
                        const FVM_Value *args,
                        FVM_Value *result,
                        void *userdata)
{
    (void)context;
    (void)receiver;
    (void)userdata;
    if (count != 1 || args[0].type != FVM_VALUE_STRING)
    {
        return FVM_ERROR;
    }
    printf("%s\n", args[0].string);
    fvm_value_undefined(result);
    return FVM_OK;
}


int main(int argc, char **argv)
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_Result status;
    FILE *input;
    unsigned char *bytes;
    size_t count;
    if (argc != 2)
    {
        return 2;
    }
    input = fopen(argv[1], "rb");
    bytes = (unsigned char *)malloc(2u * 1024u * 1024u);
    if (!input || !bytes)
    {
        if (input)
        {
            fclose(input);
        }
        free(bytes);
        return 2;
    }
    count = fread(bytes, 1, 2u * 1024u * 1024u, input);
    fclose(input);
    status = fvm_config_init(&config);
    if (status == FVM_OK)
    {
        status = fvm_runtime_create(&config, &runtime);
    }
    if (status == FVM_OK)
    {
        status = fvm_context_create(runtime, &context);
    }
    if (status == FVM_OK)
    {
        status = fvm_extensions_install(context);
    }
    if (status == FVM_OK)
    {
        status = fvm_avm1_set_trace_rooted(context, trace, 0, 0);
    }
    if (status == FVM_OK)
    {
        status = fvm_load_memory(context, bytes, count);
    }
    if (status == FVM_OK)
    {
        status = fvm_swf_execute_actions(context);
    }
    if (context)
    {
        fvm_context_destroy(context);
    }
    if (runtime)
    {
        fvm_runtime_destroy(runtime);
    }
    free(bytes);
    if (status != FVM_OK)
    {
        fprintf(stderr, "FlashVM status %d\n", (int)status);
    }
    return status == FVM_OK ? 0 : 1;
}
