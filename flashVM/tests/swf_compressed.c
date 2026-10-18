#include "flashvm/swf.h"
#include "flashvm/avm1.h"
#include "zlib.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    /* Valid one-frame SWF, also usable as an empty action execution test. */
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
    assert(compress2(cws + 8, &compressed, fws + 8, sizeof(fws) - 8, Z_BEST_COMPRESSION) == Z_OK);
    size = (size_t)compressed + 8;
    info.struct_size = sizeof(info);
    assert(fvm_swf_inspect(cws, size, &info) == FVM_OK);
    assert(info.version == 8 && info.declared_length == 16);
    assert(info.frame_count == 1 && info.tag_count == 1);

    assert(fvm_config_init(&config) == FVM_OK);
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_load_memory(context, cws, size) == FVM_OK);
    assert(fvm_swf_execute_actions(context) == FVM_OK);
    /* A failed reload must preserve the previously loaded movie. */
    assert(fvm_load_memory(context, cws, size - 1) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_info(context, &info) == FVM_OK && info.version == 8);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);

    for (version = 6; version <= 8; ++version)
    {
        cws[3] = (unsigned char)version;
        assert(fvm_swf_inspect(cws, size, &info) == FVM_OK);
    }
    cws[3] = 5;
    assert(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT);
    cws[3] = 8;
    cws[size] = 0;
    assert(fvm_swf_inspect(cws, size + 1, &info) == FVM_INVALID_ARGUMENT);
    cws[4] = 15;
    assert(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT);
    cws[4] = 17;
    assert(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT);
    cws[4] = 16;
    cws[8] ^= 0xff;
    assert(fvm_swf_inspect(cws, size, &info) == FVM_INVALID_ARGUMENT);
    cws[8] ^= 0xff;
    cws[7] = 127;
    assert(fvm_swf_inspect(cws, size, &info) == FVM_OUT_OF_MEMORY);
    cws[7] = 0;

    config.memory_limit_bytes = 15;
    assert(fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    assert(fvm_load_memory(context, cws, size) == FVM_OUT_OF_MEMORY);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);

    memcpy(malformed, fws, sizeof(fws));
    malformed[8] = 0;
    assert(fvm_swf_inspect(malformed, sizeof(malformed), &info) == FVM_INVALID_ARGUMENT);
    malformed[8] = 8;
    malformed[14] = 1; /* End with a nonzero payload. */
    assert(fvm_swf_inspect(malformed, sizeof(malformed), &info) == FVM_INVALID_ARGUMENT);
    return 0;
}
