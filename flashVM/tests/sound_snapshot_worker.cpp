/* Real workers stop before source comparison or between a canonical batch's host notifications. */
#include <stdio.h>
#include <stdlib.h>
#include "sound_snapshot_fixture.hpp"
#include "flashvm/worker.h"


struct SoundStop
{
    unsigned calls;
};


static FVM_Result stop_sound(FVM_Context *c, FVM_Object *, size_t ordinal, void *raw)
{
    SoundStop *p = static_cast<SoundStop *>(raw);
    if (!(ordinal == p->calls))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    ++p->calls;
    if (p->calls == 1)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    return FVM_OK;
}


static FVM_Result sound_worker(FVM_Context *c, const FVM_Value *input, FVM_Value *out, void *)
{
    if (!(fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes tags, pcm, empty;
    u16(pcm, 7);
    pcm.push_back(12);
    u32(pcm, 2);
    pcm.push_back(0);
    pcm.push_back(255);
    tag(tags, 14, pcm);
    for (unsigned i = 0; i < 200; ++i)
    {
        sound_event(tags, 0);
    }
    tag(tags, 1, empty);
    tag(tags, 0, empty);
    Bytes swf = movie(tags, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *root = 0;
    if (!(fvm_movieclip_load_swf(c, swf.begin(), swf.size(), "root", &root) == FVM_OK && fvm_gc_pin(c, root) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    SoundStop p = {0};
    if (!(fvm_movieclip_set_sound_callback(c, stop_sound, &p) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!input->boolean)
    {
        FVM_Worker *self = 0;
        if (!(fvm_worker_current(c, &self) == FVM_OK && fvm_worker_stop(self) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!((input->boolean ? fvm_movieclip_advance(c, root, 1) : sound_snapshot_run(c, root)) == FVM_ERROR))
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
    if (input->boolean)
    {
        if (!(p.calls > 0 && p.calls < 200))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (!(!p.calls))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_value_boolean(out, 1);
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    if (!(fvm_config_init(&cfg) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    cfg.instruction_limit = 0;
    for (unsigned i = 0; i < 2; ++i)
    {
        FVM_Value input, out;
        fvm_value_boolean(&input, i != 0);
        FVM_Worker *w = 0;
        if (!(fvm_worker_run(&cfg, sound_worker, &input, 0, &w) == FVM_OK && fvm_worker_join(w, &out) == FVM_OK &&
              out.boolean))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_worker_destroy(w);
    }
}
