/* Restored/private malformed records fail during preparation before a valid earlier record runs. */
#include <stdio.h>
#include <stdlib.h>
#include "clip_actions_fixture.hpp"
#include "flashvm/table.h"


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned bad = 0; bad < 7; ++bad)
    {
        FVM_Context *c = clip_context(rt);
        Bytes events;
        u16(events, 0);
        u32(events, 1);
        clip_record(events, 8, 1, action(42));
        clip_record(events, 8, 1, action(17));
        u32(events, 0);
        FVM_Object *root = clip_load(c, clip_movie(8, events));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object,
                   *table = actor->resource_result.object->properties["clip_actions"].object,
                   *record = table->indexed[1].object;
        FVM_Value v;
        switch (bad)
        {
        case 0:
            fvm_value_number(&v, 0);
            record->properties["events"] = v;
            break;
        case 1:
            fvm_value_number(&v, 256);
            record->properties["key"] = v;
            break;
        case 2:
            fvm_value_number(&v, 1);
            record->properties["key"] = v;
            break;
        case 3:
            if (!(fvm_value_string(&v, "zz") == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_table_set_index(c, record, 0, &v) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            break;
        case 4:
            record->indexed.erase(0);
            break;
        case 5:
            table->indexed[2] = table->indexed[1];
            table->indexed.erase(1);
            break;
        default:
            fvm_value_number(&v, 0);
            record->indexed[1] = v;
            break;
        }
        if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0) == FVM_INVALID_ARGUMENT &&
              get(c, actor, "seen").type == FVM_VALUE_UNDEFINED))
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
    fvm_runtime_destroy(rt);
}
