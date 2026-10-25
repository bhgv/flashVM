/* Late parse/resource failures and subsequent retries preserve caller-owned outputs. */
#include <stdio.h>
#include <stdlib.h>
#include "sound_metadata_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
          fvm_context_create(rt, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes swf = metadata_movie();
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    /* Corrupt a canonical envelope's final gain after loading, keeping structure/size stable. */
    size_t corrupt = c->swf_bytes.size() - 5;
    unsigned char saved = c->swf_bytes[corrupt];
    c->swf_bytes[corrupt] = 255;
    for (unsigned route = 3; route < 6; ++route)
    {
        if (!(metadata_query(c, route, true) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    c->swf_bytes[corrupt] = saved;
    for (unsigned route = 0; route < 6; ++route)
    {
        if (!(metadata_query(c, route, false) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_SWFSoundInfo info;
    std::memset(&info, 0xa5, sizeof(info));
    info.struct_size = sizeof(info);
    unsigned char old[sizeof(info)];
    std::memcpy(old, &info, sizeof(info));
    if (!(fvm_swf_sound_at(c, 1, &info) == FVM_NOT_FOUND && !std::memcmp(&info, old, sizeof(info)) &&
          c->avm1_steps == 6))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = true;
    size_t count = 12345;
    if (!(fvm_swf_sound_event_count(c, &count) == FVM_INVALID_ARGUMENT && count == 12345))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
