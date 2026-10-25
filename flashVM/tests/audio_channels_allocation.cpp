/* Registration OOM is atomic; stop/close do not allocate, even with failed allocation armed. */
#define main audio_allocation_fixture_main
#include <stdio.h>
#include <stdlib.h>
#include "native_instance_allocation.cpp"

#undef main
#include "flashvm/multimedia.h"

static unsigned written, stopped;


static FVM_Result output_write(void *, const FVM_MediaFrame *)
{
    ++written;
    return FVM_OK;
}


static FVM_Result output_stop(void *)
{
    ++stopped;
    return FVM_OK;
}


int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    bool failed = false, succeeded = false;
    for (long boundary = 0; boundary < 16 && !succeeded; ++boundary)
    {
        FVM_Context *c = 0;
        if (!(fvm_context_create(rt, &c) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_MediaOutput output;
        output.struct_size = sizeof(output);
        output.userdata = 0;
        output.write = output_write;
        output.stop = output_stop;
        FVM_AudioChannel *channel = (FVM_AudioChannel *)1;
        allowance = boundary;
        FVM_Result result = fvm_audio_channel_create(c, &output, &channel);
        allowance = -1;
        if (result == FVM_OUT_OF_MEMORY)
        {
            failed = true;
            if (!(!channel && c->audio_channels.empty()))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_audio_channel_create(c, &output, &channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (!(result == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            succeeded = true;
        }
        if (!(c->audio_channels.size() == 1))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        unsigned char data[2] = {0};
        FVM_MediaFrame frame;
        std::memset(&frame, 0, sizeof(frame));
        frame.kind = FVM_MEDIA_AUDIO;
        frame.data = data;
        frame.size = 2;
        frame.channels = 1;
        frame.sample_rate = 8000;
        frame.samples = 1;
        if (!(fvm_audio_channel_write(channel, &frame) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = 0;
        if (!(fvm_audio_stop_all(c) == FVM_OK && fvm_audio_channel_close(channel) == FVM_OK))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        allowance = -1;
        if (!(c->audio_channels.empty()))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(c);
    }
    if (!(failed && succeeded && written == stopped))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(rt);
    return 0;
}
