/* History configuration is transactional; release/hard stop and exact retry do not allocate. */
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <new>

static bool fail_alloc = false;


void *operator new(size_t n)
{
    if (fail_alloc)
    {
        throw std::bad_alloc();
    }
    void *p = std::malloc(n ? n : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
    return p;
}

void *operator new[](size_t n)
{
    return ::operator new(n);
}


void operator delete(void *p) throw()
{
    std::free(p);
}


void operator delete[](void *p) throw()
{
    std::free(p);
}


void *operator new(size_t n, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(n);
    }
    catch (...)
    {
        return 0;
    }
}


void *operator new[](size_t n, const std::nothrow_t &) throw()
{
    try
    {
        return ::operator new(n);
    }
    catch (...)
    {
        return 0;
    }
}


void operator delete(void *p, const std::nothrow_t &) throw()
{
    std::free(p);
}


void operator delete[](void *p, const std::nothrow_t &) throw()
{
    std::free(p);
}

#include "swf_sound_output_fixture.hpp"

using namespace output_fixture;


int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    conversion_fixture::Samples samples;
    for (unsigned i = 0; i < 1000; ++i)
    {
        samples.push_back(10000);
    }
    conversion_fixture::Bytes swf = conversion_fixture::movie(samples, 3, 1);
    if (!(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, 2);
    Device d;
    FVM_AudioQueueBackend table = backend(d);
    FVM_SWFSoundOutput *out = 0;
    if (!(fvm_swf_sound_output_create(mixer, &table, 8, 16, &out) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_alloc = true;
    if (!(fvm_swf_sound_output_set_release(out, 4) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop_release(out) == FVM_INVALID_ARGUMENT))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_alloc = false;
    if (!(fvm_swf_sound_output_set_release(out, 4) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_alloc = true;
    if (!(fvm_swf_sound_output_set_release(out, 6) == FVM_OUT_OF_MEMORY))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_alloc = false;
    if (!(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pump(out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_pause(out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    consume(d, 8);
    fail_alloc = true;
    if (!(fvm_swf_sound_output_stop_release(out) == FVM_OK && d.queued == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_stop(out) == FVM_OK && !d.queued))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_swf_sound_output_set_release(out, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fail_alloc = false;
    fvm_swf_sound_output_close(out);
    fvm_swf_sound_mixer_close(mixer);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
}
