/* Independent malformed wire controls and strict version masks never escape tag/action bounds. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_actions_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 5; version <= 8; ++version)
    {
        FVM_Context *c = clip_context(rt);
        Bytes valid = clip_event_table(version, 2, action(42));
        for (size_t n = 0; n < valid.size(); ++n)
        {
            Bytes truncated;
            for (size_t i = 0; i < n; ++i)
            {
                truncated.push_back(valid[i]);
            }
            clip_load(c, clip_movie(version, truncated), FVM_INVALID_ARGUMENT);
        }
        Bytes bad = valid;
        bad[0] = 1;
        clip_load(c, clip_movie(version, bad), FVM_INVALID_ARGUMENT);
        bad = valid;
        bad.push_back(1);
        clip_load(c, clip_movie(version, bad), FVM_INVALID_ARGUMENT);
        bad = valid;
        bad[2] = 3;
        clip_load(c, clip_movie(version, bad), FVM_INVALID_ARGUMENT);
        Bytes bytes;
        bytes.push_back(0x96);
        bytes.push_back(5);
        bytes.push_back(0);
        bytes.push_back(7);
        clip_load(c, clip_movie(version, clip_event_table(version, 2, bytes)), FVM_INVALID_ARGUMENT);
        if (version >= 6)
        {
            bad = valid;
            bad[5] = 0x80;
            clip_load(c, clip_movie(version, bad), FVM_INVALID_ARGUMENT);
            Bytes zero;
            u16(zero, 0);
            u32(zero, 0x20000);
            u32(zero, 0x20000);
            u32(zero, 0);
            u32(zero, 0);
            clip_load(c, clip_movie(version, zero), FVM_INVALID_ARGUMENT);
        }
        if (version < 7)
        {
            clip_load(c,
                      clip_movie(version, clip_event_table(version, version == 5 ? 0x200 : 0x40000, action(1))),
                      FVM_UNSUPPORTED);
        }
        Bytes empty;
        u16(empty, 0);
        clip_flags(empty, version, 0);
        clip_flags(empty, version, 0);
        FVM_Object *root = clip_load(c, clip_movie(version, empty));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK &&
              get(c, get(c, root, "actor").object, "seen").type == FVM_VALUE_UNDEFINED))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        clip_clean(c);
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    FVM_Context *c = clip_context(rt);
    clip_load(c, clip_movie(4, clip_event_table(4, 2, action(1))), FVM_UNSUPPORTED);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
