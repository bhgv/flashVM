/* Every flagged import allocation boundary leaves no published clip and a reusable source. */
#define main muted_allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#define main muted_import_reference_main
#include "swf_muted_stream_import.cpp"

#undef main


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    FVM_Context *c = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(r, &c) == FVM_OK && fvm_standard_install(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    Bytes bytes = source(false, false);
    if (!(fvm_load_memory(c, bytes.begin(), bytes.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool complete = false;
    for (long failure = 0; failure < 2048 && !complete; ++failure)
    {
        FVM_Object *clip = 0;
        allowance = failure;
        FVM_Result result =
            fvm_movieclip_load_swf_ex(c, bytes.begin(), bytes.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip);
        allowance = -1;
        if (!(result == FVM_OK || result == FVM_OUT_OF_MEMORY))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(result == FVM_OK || !clip))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        complete = result == FVM_OK;
        /* [GC:] Retain the successful graph before reclaiming abandoned partial imports. */
        if (complete)
        {
            if (!(fvm_gc_pin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        if (!(fvm_gc_collect(c, 0) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        size_t count = 0;
        if (!(fvm_swf_stream_sound_count(c, &count) == FVM_OK && count == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (complete)
        {
            if (!(fvm_movieclip_advance(c, clip, 1) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_gc_unpin(c, clip) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (!(complete))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Object *clip = 0;
    allowance = 0;
    c->gc_collecting = true;
    if (!(fvm_movieclip_load_swf_ex(c, bytes.begin(), bytes.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
              FVM_INVALID_ARGUMENT &&
          !clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_collecting = false;
    c->gc_releasing = true;
    if (!(fvm_movieclip_load_swf_ex(c, bytes.begin(), bytes.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
              FVM_INVALID_ARGUMENT &&
          !clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->gc_releasing = false;
    c->destroying = true;
    if (!(fvm_movieclip_load_swf_ex(c, bytes.begin(), bytes.size(), "root", FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) ==
              FVM_INVALID_ARGUMENT &&
          !clip))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    c->destroying = false;
    allowance = -1;
    fvm_context_destroy(c);
    fvm_runtime_destroy(r);
    return 0;
}
