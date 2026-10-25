#include <stdio.h>
#include <stdlib.h>
#include "flashvm/swf.h"
#include "flashvm/avm1.h"
#include "zlib.h"
#include <string.h>


int main(void)
{ /* Valid one-frame SWF, also usable as an empty action execution test. */
    static const unsigned char fws[] = {'F', 'W', 'S', 8, 16, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0, 0};
    unsigned char cws[128];
    unsigned char malformed[sizeof(fws)];
    uLongf compressed = sizeof(cws) - 8;
    FVM_SWFInfo info;
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    size_t size;
    unsigned int version;


    memcpy(cws, fws, 8);
    cws[0] = 'C';
    if (!(compress2(cws + 8, &compressed, fws + 8, sizeof(fws) - 8, Z_BEST_COMPRESSION) == Z_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size = (size_t)compressed + 8;
    info.struct_size = sizeof(info);
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.version == 8 && info.declared_length == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(info.frame_count == 1 && info.tag_count == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }

    if (!(fvm_config_init(&config) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(context, cws, size) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_execute_actions(context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* A failed reload must preserve the previously loaded movie. */
    if (!(fvm_load_memory(context, cws, size - 1) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_info(context, &info) == FVM_OK && info.version == 8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);

    for (version = 6; version <= 8; ++version)
    {
        cws[3] = (unsigned char)version;
        if (!(fvm_swf_inspect(cws, size, &info) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    cws[3] = 5;
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[3] = 8;
    cws[size] = 0;
    if (!(fvm_swf_inspect(cws, size + 1, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[4] = 15;
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[4] = 17;
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[4] = 16;
    cws[8] ^= 0xff;
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[8] ^= 0xff;
    cws[7] = 127;
    if (!(fvm_swf_inspect(cws, size, &info) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cws[7] = 0;

    config.memory_limit_bytes = 15;
    if (!(fvm_runtime_create(&config, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_load_memory(context, cws, size) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);


    memcpy(malformed, fws, sizeof(fws));
    malformed[8] = 0;
    if (!(fvm_swf_inspect(malformed, sizeof(malformed), &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    malformed[8] = 8;
    malformed[14] = 1; /* End with a nonzero payload. */
    if (!(fvm_swf_inspect(malformed, sizeof(malformed), &info) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    return 0;
}
