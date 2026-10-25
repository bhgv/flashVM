/* Exact budget rejects before new; every adapter allocation unwinds; tail hot paths allocate nothing. */
#include <stdio.h>
#include <stdlib.h>
#include <new>
#include <cstdlib>

static long tail_allowance = -1;
static size_t tail_allocations = 0;


void *operator new(std::size_t n) throw(std::bad_alloc)
{
    ++tail_allocations;
    if (tail_allowance == 0)
    {
        throw std::bad_alloc();
    }
    if (tail_allowance > 0)
    {
        --tail_allowance;
    }
    void *p = std::malloc(n ? n : 1);
    if (!p)
    {
        throw std::bad_alloc();
    }
    return p;
}


void operator delete(void *p) throw()
{
    std::free(p);
}


void *operator new[](std::size_t n) throw(std::bad_alloc)
{
    return ::operator new(n);
}


void operator delete[](void *p) throw()
{
    ::operator delete(p);
}

#define main tail_reference_main
#include "swf_sound_mix_tail.cpp"

#undef main


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
    Samples samples;
    for (unsigned i = 0; i < 8; ++i)
    {
        samples.push_back((short)(100 + i));
    }
    Bytes swf = movie(samples, 3, 1);
    if (!(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixer *mixer = create(44100, 1);
    if (!(fvm_swf_sound_mixer_submit(mixer, c, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    TailQueue q;
    std::memset(&q, 0, sizeof(q));
    FVM_AudioQueueBackend b;
    std::memset(&b, 0, sizeof(b));
    b.struct_size = sizeof(b);
    b.abi_version = FVM_AUDIO_QUEUE_ABI_VERSION;
    b.sample_rate = 44100;
    b.channels = 1;
    b.userdata = &q;
    b.write = tail_write;
    b.queued = tail_count;
    b.pause = tail_pause;
    b.clear = tail_clear;
    FVM_AudioQueueBackend adapted = b;
    FVM_SWFSoundMixQueue *queue = 0;
    size_t bytes = 0;
    if (!(fvm_swf_sound_mix_queue_storage_required(1, 2, &bytes) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    size_t before = tail_allocations;
    tail_allowance = 0;
    if (!(fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 2, bytes - 1, &queue, &adapted) == FVM_OUT_OF_MEMORY &&
          !queue && tail_allocations == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    tail_allowance = -1;
    for (long budget = 0; budget <= 3; ++budget)
    {
        tail_allowance = budget;
        FVM_Result result = fvm_swf_sound_mix_queue_create_bounded(mixer, &b, 2, bytes, &queue, &adapted);
        tail_allowance = -1;
        if (budget < 3)
        {
            if (!(result == FVM_OUT_OF_MEMORY && !queue && !std::memcmp(&adapted, &b, sizeof(b))))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK && queue))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
    }
    before = tail_allocations;
    tail_allowance = 0;
    size_t accepted = 91;
    q.fail = 1;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_ERROR && !accepted))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueuePending p;
    p.struct_size = sizeof(p);
    if (!(fvm_swf_sound_mix_queue_pending(queue, &p) == FVM_OK && p.origin == FVM_SWF_SOUND_MIX_QUEUE_PENDING_TAIL))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    q.fail = 0;
    if (!(fvm_swf_sound_mix_queue_pump_tail(queue, 2, 4, &accepted) == FVM_OK && accepted == 2 && q.saved[0] == 100 &&
          q.saved[1] == 101))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    FVM_SWFSoundMixQueueStorageInfo storage;
    storage.struct_size = sizeof(storage);
    if (!(fvm_swf_sound_mix_queue_storage_info(queue, &storage) == FVM_OK && storage.retained_bytes == bytes))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(adapted.pause(adapted.userdata, 1) == FVM_OK && adapted.clear(adapted.userdata) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(tail_allocations == before))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_swf_sound_mix_queue_close(queue);
    fvm_swf_sound_mixer_close(mixer);
    tail_allowance = -1;
    return 0;
}
