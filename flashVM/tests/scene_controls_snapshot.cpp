/* Constructors cannot rewrite, truncate or grow their captured frame-control batch. */
#include <stdio.h>
#include <stdlib.h>
#include "scene_controls_fixture.hpp"


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
            for (unsigned mutation = 0; mutation <= 4; ++mutation)
            {
                if (mode && !fvm_jit_available())
                {
                    continue;
                }
                FVM_Context *c = tick_context(rt, version);
                SceneControlProbe p;
                FVM_Object *root = scene_control_root(c, p, version);
                if (mode)
                {
                    if (!(fvm_execution_set_mode(c, FVM_EXECUTION_JIT, 0) == FVM_OK))
                    {
                        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                        exit(EXIT_FAILURE);
                    }
                }
                p.mutation = mutation;
                /* Insertion order differs from semantic control ordinals. */
                FVM_Value first = p.list->indexed[0], second = p.list->indexed[1], third = p.list->indexed[2];
                p.list->indexed.clear();
                p.list->indexed[2] = third;
                p.list->indexed[0] = first;
                p.list->indexed[1] = second;
                if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && p.calls == 3))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(p.depths[0] == -16383 && p.depths[1] == -16382 && p.depths[2] == -16381))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(get(c, root, "first").type == FVM_VALUE_OBJECT &&
                      get(c, root, "second").type == FVM_VALUE_OBJECT))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                if (!(get(c, root, "third").type == FVM_VALUE_OBJECT &&
                      get(c, root, "late").type == FVM_VALUE_UNDEFINED))
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
                if (!(fvm_gc_collect(c, 0) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
                fvm_context_destroy(c);
            }
        }
    }
    fvm_runtime_destroy(rt);
}
