/* Bad ordinal topology/ownership is rejected before action effects; later bad code remains partial. */
#include <stdio.h>
#include <stdlib.h>
#include "frame_blocks_fixture.hpp"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned route = 0; route <= 4; ++route)
    {
        for (unsigned bad = 0; bad < 9; ++bad)
        {
            if (route == 3 && !fvm_jit_available())
            {
                continue;
            }
            FVM_Context *c = tick_context(rt), *other = tick_context(rt);
            FVM_Object *clip = tick_root(c), *foreign = tick_root(other);
            FVM_Object *segments = frame_segments(c, clip),
                       *record = clip->resource_result.object->properties["frames"].object->indexed[1].object;
            FVM_Value saved = record->properties["segments"], one = frame_hex(c, action(1));
            segments->indexed[0] = one;
            if (bad == 0)
            {
                segments->indexed[2] = one;
            }
            if (bad == 1)
            {
                segments->indexed.clear();
                segments->indexed[1] = one;
            }
            if (bad == 2)
            {
                record->properties["segments"] = tick_object(0);
            }
            if (bad == 3)
            {
                record->properties["segments"] = tick_object(foreign);
            }
            if (bad == 4)
            {
                fvm_value_number(&record->properties["segments"], 1);
            }
            if (bad == 5)
            {
                for (unsigned long i = 0; i <= FVM_MOVIECLIP_MAX_FRAME_BLOCKS; ++i)
                {
                    segments->indexed[i] = one;
                }
            }
            if (bad == 6)
            {
                fvm_value_number(&segments->indexed[1], 17);
            }
            if (bad == 7)
            {
                if (!(fvm_value_string(&segments->indexed[1], "zz") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (bad == 8)
            {
                if (!(fvm_value_string(&segments->indexed[1], "0") == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            if (!(run_frame(c, clip, route) == FVM_INVALID_ARGUMENT))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            tick_clean(c);
            FVM_Value seen;
            if (!(fvm_object_get(c, clip, "seen", &seen) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(bad >= 6 ? (seen.type == FVM_VALUE_NUMBER && seen.number == 1) : seen.type == FVM_VALUE_UNDEFINED))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            record->properties["segments"] = saved;
            segments->indexed.clear();
            segments->indexed[0] = one;
            /* Tick already committed entered=1: use ActionCall to retry code without replaying a tick. */
            if (!(run_frame(c, clip, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            number(c, clip, "seen", 1);
            fvm_context_destroy(c);
            fvm_context_destroy(other);
        }
    }
    fvm_runtime_destroy(rt);
}
