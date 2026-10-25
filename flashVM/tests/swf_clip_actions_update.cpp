/* Modify replaces event tables, omitted fields retain them, and explicit delivery never ticks a scene. */
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
        Bytes one = clip_event_table(version, 2, action(11)), two = clip_event_table(version, 2, action(22));
        FVM_Object *root = clip_load(c, clip_movie(version, one, &two));
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_Object *actor = get(c, root, "actor").object;
        number(c, actor, "seen", 11);
        if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 1, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, actor, "seen", 11);
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && get(c, root, "actor").object == actor))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, actor, "seen", 22);
        if (!(fvm_movieclip_dispatch_clip_actions(c, actor, 2, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, actor, "seen", 22);
        FVM_Value args[2], copy;
        if (!(fvm_value_string(args, "copy") == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_value_number(args + 1, 4);
        if (!(fvm_object_call(c, actor, "duplicateMovieClip", 2, args, &copy) == FVM_OK &&
              copy.type == FVM_VALUE_OBJECT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_movieclip_dispatch_clip_actions(c, copy.object, 2, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, copy.object, "seen", 22);
        FVM_Value frame = get(c, root, "_currentframe");
        if (!(frame.number == 2))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_object_call(c, root, "stop", 0, 0, &frame) == FVM_OK && fvm_movieclip_advance(c, root, 1) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        number(c, actor, "seen", 22);
        clip_clean(c);
        fvm_context_destroy(c);
    }
    fvm_runtime_destroy(rt);
}
