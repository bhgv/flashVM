/* Independent protected-MP3 reservoir histories on four real workers with C++ allocation disabled. */
#define main allocation_reference_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "swf_crc_mp3_fixture.hpp"
#include <cstring>
#include "../src/swf/sound_mp3.hpp"
#include "../ports/thread_port.h"
#include "../ports/channel_sync.h"


struct Job
{
    FVM_PortSync *gate;
    unsigned index;
    bool failed;
    size_t frames;
};


/* Each worker owns decoder/PCM/error state; only immutable original encoded arrays are shared. */
static void worker(void *raw)
{
    Job *job = (Job *)raw;
    fvm_port_sync_lock(job->gate);
    fvm_port_sync_unlock(job->gate);
    for (unsigned pass = 0; pass < 8; ++pass)
    {
        for (unsigned index = 0; index < 8; ++index)
        {
            const crc_mp3_fixture::Fixture &v = crc_mp3_fixture::fixtures[(index + job->index) % 8];
            FVM_SWFMP3Decoder decoder;
            size_t at = 0, frames = 0;
            while (at < v.size)
            {
                unsigned char bad[10000];
                if (v.size - at > sizeof(bad))
                {
                    job->failed = true;
                    return;
                }
                std::memcpy(bad, v.data + at, v.size - at);
                bad[4] ^= 1;
                mp3dec_t saved = decoder.state;
                FVM_SWFMP3Frame packet, old;
                std::memset(&old, 77, sizeof(old));
                packet = old;
                if (decoder.decode(bad, v.size - at, &packet) != FVM_INVALID_ARGUMENT ||
                    std::memcmp(&saved, &decoder.state, sizeof(saved)) || std::memcmp(&packet, &old, sizeof(old)))
                {
                    job->failed = true;
                    return;
                }
                if (decoder.decode(v.data + at, v.size - at, &packet) != FVM_OK)
                {
                    job->failed = true;
                    return;
                }
                for (size_t p = 0; p < sizeof(crc_mp3_fixture::positions) / sizeof(crc_mp3_fixture::positions[0]); ++p)
                {
                    if (crc_mp3_fixture::positions[p] >= frames &&
                        crc_mp3_fixture::positions[p] - frames < packet.samples)
                    {
                        for (unsigned channel = 0; channel < v.channels; ++channel)
                        {
                            if (std::labs(
                                    (long)decoder.pcm[(crc_mp3_fixture::positions[p] - frames) * v.channels + channel] -
                                    v.golden[p * v.channels + channel]) > 2)
                            {
                                job->failed = true;
                                return;
                            }
                        }
                    }
                }
                at += packet.bytes;
                frames += packet.samples;
            }
            if (frames != 5760)
            {
                job->failed = true;
                return;
            }
            job->frames += frames;
        }
    }
}


/* The gate orders allocation denial before worker decode; all joins precede source/gate cleanup. */
int main()
{
    FVM_PortSync *gate = fvm_port_sync_create();
    if (!(gate))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_port_sync_lock(gate);
    FVM_PortThread *threads[4];
    Job jobs[4];
    for (unsigned i = 0; i < 4; ++i)
    {
        jobs[i].gate = gate;
        jobs[i].index = i;
        jobs[i].failed = false;
        jobs[i].frames = 0;
        threads[i] = fvm_port_thread_start(worker, jobs + i);
        if (!(threads[i]))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    allowance = 0;
    fvm_port_sync_unlock(gate);
    for (unsigned i = 0; i < 4; ++i)
    {
        fvm_port_thread_join(threads[i]);
        if (!(!jobs[i].failed && jobs[i].frames == 8UL * 8UL * 5760UL))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (!(allowance == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    allowance = -1;
    fvm_port_sync_destroy(gate);
    return 0;
}
