/* Registration OOM/capacity rollback; stop/detach/terminal cleanup allocate nothing. */
#define main allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/audio_stop.h"
#include "flashvm/multimedia.h"


static FVM_Result stop(void *raw)
{
    ++*(unsigned *)raw;
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *r = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &r) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool failed = false, passed = false;
    unsigned calls = 0;
    for (long boundary = 0; boundary < 10 && !passed; ++boundary)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(r, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_AudioStopBinding *binding = (FVM_AudioStopBinding *)1;
        allowance = boundary;
        FVM_Result result = fvm_audio_stop_binding_create(c, stop, &calls, &binding);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(!binding && c->audio_stop_bindings.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && binding))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            passed = true;
            allowance = 0;
            if (!(fvm_audio_stop_all(c) == FVM_OK && fvm_audio_stop_binding_close(binding) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            allowance = -1;
        }
        fvm_context_destroy(c);
    }
    if (!(failed && passed && calls == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_Context *c = 0;
    if (!(fvm_context_create(r, &c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_AudioStopBinding *bindings[FVM_AUDIO_STOP_MAX_BINDINGS];
    for (unsigned i = 0; i < FVM_AUDIO_STOP_MAX_BINDINGS; ++i)
    {
        if (!(fvm_audio_stop_binding_create(c, stop, &calls, &bindings[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    FVM_AudioStopBinding *extra = (FVM_AudioStopBinding *)1;
    if (!(fvm_audio_stop_binding_create(c, stop, &calls, &extra) == FVM_OUT_OF_MEMORY && !extra))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = 0;
    if (!(fvm_audio_stop_all(c) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned i = 0; i < FVM_AUDIO_STOP_MAX_BINDINGS; ++i)
    {
        if (!(fvm_audio_stop_binding_close(bindings[i]) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    fvm_context_destroy(c);
    allowance = -1;
    if (!(calls == 1 + FVM_AUDIO_STOP_MAX_BINDINGS))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(r);
    return 0;
}
