/* Structural control corruption is rejected before any constructor is invoked. */
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
    for (unsigned kind = 0; kind < 6; ++kind)
    {
        FVM_Context *c = tick_context(rt), *foreign = tick_context(rt);
        SceneControlProbe p;
        FVM_Object *root = scene_control_root(c, p);
        if (kind == 0)
        {
            p.list->indexed.erase(1);
        }
        if (kind == 1)
        {
            p.list->indexed[3] = p.list->indexed[1];
            p.list->indexed.erase(1);
        }
        if (kind == 2)
        {
            FVM_Value v;
            fvm_value_number(&v, 3);
            p.list->indexed[1] = v;
        }
        if (kind == 3)
        {
            p.list->indexed[1] = tick_object(0);
        }
        if (kind == 4)
        {
            FVM_Object *o = 0;
            if (!(fvm_object_create(foreign, &o) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            p.list->indexed[1] = tick_object(o);
        }
        if (kind == 5)
        {
            for (unsigned long i = 3; i <= FVM_MOVIECLIP_MAX_SCENE_CONTROLS; ++i)
            {
                p.list->indexed[i] = p.list->indexed[0];
            }
        }
        if (!(fvm_movieclip_advance(c, root, 1) == FVM_INVALID_ARGUMENT && p.calls == 0))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        tick_clean(c);
        fvm_context_destroy(c);
        fvm_context_destroy(foreign);
    }
    fvm_runtime_destroy(rt);
}
