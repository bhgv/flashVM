/* Dense ordinals, source mutation defense and callback errors do not depend on hash insertion order. */
#include <stdio.h>
#include <stdlib.h>
#include "sound_snapshot_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 6; version <= 8; ++version)
    {
        for (unsigned mode = 0; mode < 2; ++mode)
        {
            for (unsigned mutation = 0; mutation < 4; ++mutation)
            {
                if (mode && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = tick_context(rt, version);
                SoundSnapshot p;
                FVM_Object *root = sound_snapshot_root(c, p, version);
                FVM_Value a = p.list->indexed[0], b = p.list->indexed[1];
                p.list->indexed.clear();
                p.list->indexed[1] = b;
                p.list->indexed[0] = a;
                p.mutation = mutation;
                if (!(fvm_execution_set_mode(c, mode ? FVM_EXECUTION_JIT : FVM_EXECUTION_INTERPRETER, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.count == 2 && p.delivered[0] == 0 &&
                      p.delivered[1] == 1))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (mode)
                {
                    if (!(c->execution_mode.stats.native_entries > 0))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                tick_clean(c);
                if (!(!c->movieclip_sound_busy))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    for (unsigned error = 0; error < 2; ++error)
    {
        FVM_Context *c = tick_context(rt);
        SoundSnapshot p;
        FVM_Object *root = sound_snapshot_root(c, p);
        seek(c, root, 1, false);
        p.status = error ? FVM_OUT_OF_MEMORY : FVM_ERROR;
        if (!(fvm_movieclip_advance(c, root, 1) == p.status && p.count == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        p.status = FVM_OK;
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.count == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        if (!(!c->movieclip_sound_busy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
