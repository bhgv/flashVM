/* Canonical indexing is one charged scan with transactional outputs, malformed tails and a hard cap. */
#include <stdio.h>
#include <stdlib.h>
#include "sound_metadata_fixture.hpp"
#include "../src/swf/sound_event_index.hpp"
#include "../src/swf/sound_event_constants.h"


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
    for (unsigned long limit = 0; limit <= 12; ++limit)
    {
        fvm_detail::vector<size_t> out;
        out.push_back(999);
        rt->config.instruction_limit = limit;
        if (!(fvm_swf_sound_frame_ordinals(c, 1, out) == (limit && limit < 11 ? FVM_ERROR : FVM_OK)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(c->avm1_steps == (limit && limit < 11 ? limit : 11)))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (limit && limit < 11)
        {
            if (!(out.size() == 1 && out[0] == 999))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(out.size() == 2 && out[0] == 0 && out[1] == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    rt->config.instruction_limit = 0;
    fvm_detail::vector<size_t> out;
    out.push_back(999);
    if (!(fvm_swf_sound_frame_ordinals(c, 0, out) == FVM_INVALID_ARGUMENT && out.size() == 1 && out[0] == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_frame_ordinals(c, 2, out) == FVM_INVALID_ARGUMENT && out.size() == 1 && out[0] == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->swf_bytes[c->swf_bytes.size() - 5] = 255;
    if (!(fvm_swf_sound_frame_ordinals(c, 1, out) == FVM_INVALID_ARGUMENT && out.size() == 1 && out[0] == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes tags, body;
    word(body, 7);
    body.push_back(0);
    for (unsigned long i = 0; i <= FVM_SWF_SOUND_MAX_FRAME_ORDINALS; ++i)
    {
        tag(tags, 15, body);
    }
    swf = movie(tags);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_frame_ordinals(c, 1, out) == FVM_INVALID_ARGUMENT && out.size() == 1 && out[0] == 999))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
