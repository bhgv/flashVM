/* Canonical membership/order/completeness preflight precedes every external notification. */
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
    for (unsigned kind = 0; kind < 12; ++kind)
    {
        FVM_Context *c = tick_context(rt);
        SoundSnapshot p;
        FVM_Object *root = sound_snapshot_root(c, p);
        FVM_Object *frames = root->resource_result.object->properties["frames"].object;
        FVM_Value value;
        if (kind == 0 || kind == 10)
        {
            p.list->indexed[1] = p.list->indexed[0];
        }
        if (kind == 1)
        {
            value = p.list->indexed[0];
            p.list->indexed[0] = p.list->indexed[1];
            p.list->indexed[1] = value;
        }
        if (kind == 2)
        {
            fvm_value_number(&value, 2);
            p.list->indexed[1] = value;
        }
        if (kind == 3)
        {
            fvm_value_number(&value, (double)(c->swf_bytes.size() / 3 - 1));
            p.list->indexed[1] = value;
        }
        if (kind == 4)
        {
            p.list->indexed.erase(1);
        }
        if (kind == 5)
        {
            p.list->indexed.clear();
        }
        if (kind == 6)
        {
            frames->indexed[1].object->properties.erase("sounds");
        }
        if (kind == 7)
        {
            frames->indexed.erase(1);
        }
        if (kind == 8)
        {
            p.list->indexed[2] = p.list->indexed[1];
        }
        if (kind == 9)
        {
            frames->indexed[2].object->properties["sounds"] = tick_object(p.list);
            seek(c, root, 2, false);
        }
        if (kind >= 10)
        {
            if (!(fvm_movieclip_set_sound_callback(c, 0, 0) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_movieclip_advance(c, root, 1) == (kind == 11 ? FVM_UNSUPPORTED : FVM_INVALID_ARGUMENT) && !p.count))
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
    /* Empty canonical frames require matching source but no host callback. */
    FVM_Context *c = tick_context(rt);
    Bytes tags, pcm, empty;
    u16(pcm, 7);
    pcm.push_back(12);
    u32(pcm, 2);
    pcm.push_back(0);
    pcm.push_back(255);
    tag(tags, 14, pcm);
    tag(tags, 1, empty);
    sound_event(tags, 0);
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 2);
    FVM_Object *root = 0;
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
          fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_UNSUPPORTED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    fvm_context_destroy(c);
    c = tick_context(rt);
    SoundSnapshot retained;
    root = sound_snapshot_root(c, retained);
    FVM_Object *list = retained.list;
    const unsigned char end[] = {0};
    if (!(fvm_movieclip_define_frame(c, root, 1, "replacement", end, sizeof(end)) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(root->resource_result.object->properties["frames"].object->indexed[1].object->properties["sounds"].object ==
          list))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_movieclip_advance(c, root, 1) == FVM_OK && retained.count == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tick_clean(c);
    FVM_Value seen;
    if (!(fvm_object_get(c, root, "seen", &seen) == FVM_OK && seen.type == FVM_VALUE_UNDEFINED))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
