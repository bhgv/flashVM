/* Actual StopSounds native entries match interpretation and preserve callback barriers. */
#define FVM_CLASS_FIXTURE_ONLY
#include <stdio.h>
#include <stdlib.h>
#include "jit_classes.cpp"
#include "flashvm/multimedia.h"
#include "flashvm/stage.h"


struct AudioHost
{
    FVM_Context *context;
    FVM_AudioChannel *channel;
    FVM_Result status;
    unsigned stops;
    unsigned id;
    bool probe;
};

static unsigned order[8], order_count;


static FVM_MediaFrame pcm_frame()
{
    static const unsigned char pcm[2] = {0, 0};
    FVM_MediaFrame frame;
    std::memset(&frame, 0, sizeof(frame));
    frame.kind = FVM_MEDIA_AUDIO;
    frame.sample_rate = 8000;
    frame.channels = 1;
    frame.samples = 1;
    frame.data = pcm;
    frame.size = sizeof(pcm);
    return frame;
}


static FVM_Result write_audio(void *, const FVM_MediaFrame *)
{
    return FVM_OK;
}


static FVM_Result stop_audio(void *raw)
{
    AudioHost *host = (AudioHost *)raw;
    ++host->stops;
    if (!(order_count < sizeof(order) / sizeof(order[0])))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    order[order_count++] = host->id;
    if (host->probe)
    {
        int active = 0;
        if (!(host->context->audio_busy && host->context->gc_native_depth))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_audio_channel_active(host->channel, &active) == FVM_OK && active))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_gc_collect(host->context, 0) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_audio_stop_all(host->context) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_audio_channel_stop(host->channel) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        if (!(fvm_audio_channel_close(host->channel) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        FVM_MediaFrame frame = pcm_frame();
        if (!(fvm_audio_channel_write(host->channel, &frame) == FVM_INVALID_ARGUMENT))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
        fvm_context_destroy(host->context);
        if (!(!host->context->destroying && host->context->audio_busy))
        {
            fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
            exit(EXIT_FAILURE);
        }
    }
    if (host->status == FVM_EXCEPTION)
    {
        FVM_Value marker;
        fvm_value_number(&marker, host->id);
        return fvm_exception_raise(host->context, &marker);
    }
    return host->status;
}


static void register_audio(FVM_Context *c, AudioHost &host, unsigned id)
{
    host.context = c;
    host.channel = 0;
    host.status = FVM_OK;
    host.stops = 0;
    host.id = id;
    host.probe = false;
    FVM_MediaOutput output;
    output.struct_size = sizeof(output);
    output.userdata = &host;
    output.write = write_audio;
    output.stop = stop_audio;
    if (!(fvm_audio_channel_create(c, &output, &host.channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void activate(AudioHost &host)
{
    FVM_MediaFrame frame = pcm_frame();
    if (!(fvm_audio_channel_write(host.channel, &frame) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}


static void active(AudioHost &host, bool expected)
{
    int value = -1;
    if (!(fvm_audio_channel_active(host.channel, &value) == FVM_OK && !!value == expected))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#ifndef FVM_STOP_SOUNDS_FIXTURE_ONLY


int main()
{
    if (!fvm_jit_available())
    {
        return 0;
    }
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    for (unsigned version = 0; version <= 8; ++version)
    {
        for (class_mode = 0; class_mode < 3; ++class_mode)
        {
            FVM_Context *c = 0, *other = 0;
            if (!(fvm_context_create(rt, &c) == FVM_OK && fvm_context_create(rt, &other) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (version)
            {
                unsigned char swf[] = {'F', 'W', 'S', 8, 18, 0, 0, 0, 8, 0, 0, 12, 1, 0, 0x40, 0, 0, 0};
                swf[3] = (unsigned char)version;
                if (!(fvm_load_memory(c, swf, sizeof(swf)) == FVM_OK))
                {
                    fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                    exit(EXIT_FAILURE);
                }
            }
            Program stop;
            stop.number(42);
            stop.op(9);
            stop.finish();
            if (!(class_run(c, stop).number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            AudioHost a, b, idle, peer;
            register_audio(c, a, 1);
            register_audio(c, b, 2);
            register_audio(c, idle, 3);
            register_audio(other, peer, 4);
            activate(a);
            activate(b);
            activate(peer);
            a.probe = b.probe = true;
            a.status = FVM_ERROR;
            b.status = FVM_INVALID_ARGUMENT;
            order_count = 0;
            class_run(c, stop, FVM_ERROR);
            if (!(order_count == 2 && order[0] == 1 && order[1] == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(a.stops == 1 && b.stops == 1 && !idle.stops && !peer.stops))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            active(a, true);
            active(b, true);
            active(peer, true);
            if (!(!c->audio_busy))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            a.status = FVM_OK;
            order_count = 0;
            class_run(c, stop, FVM_INVALID_ARGUMENT);
            active(a, false);
            active(b, true);
            if (!(order_count == 2 && a.stops == 2 && b.stops == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            b.status = FVM_OK;
            order_count = 0;
            if (!(class_run(c, stop).number == 42))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(order_count == 1 && order[0] == 2 && b.stops == 3))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            order_count = 0;
            if (!(class_run(c, stop).number == 42 && !order_count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Script exceptions unwind natively after every active output was attempted. */
            activate(a);
            activate(b);
            order_count = 0;
            a.status = FVM_EXCEPTION;
            b.status = FVM_ERROR;
            class_run(c, stop, FVM_EXCEPTION);
            if (!(order_count == 2 && order[0] == 1 && order[1] == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            active(a, true);
            active(b, true);
            a.status = b.status = FVM_OK;
            order_count = 0;
            if (!(class_run(c, stop).number == 42 && order_count == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* Both newly native actions coexist in one compiled body without disturbing the lower value. */
            activate(a);
            activate(b);
            order_count = 0;
            if (!(fvm_stage_set_quality(c, FVM_STAGE_QUALITY_BEST) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            Program combined;
            combined.number(42);
            combined.op(8);
            combined.op(9);
            combined.finish();
            if (!(class_run(c, combined).number == 42 && order_count == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            FVM_StageQuality quality;
            if (!(fvm_stage_get_quality(c, &quality) == FVM_OK && quality == FVM_STAGE_QUALITY_LOW))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            /* A source checkpoint gates delivery, but later budget exhaustion cannot undo it. */
            activate(a);
            activate(b);
            order_count = 0;
            unsigned long saved_limit = rt->config.instruction_limit;
            rt->config.instruction_limit = 1;
            class_run(c, stop, FVM_ERROR);
            if (!(!order_count))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            active(a, true);
            active(b, true);
            rt->config.instruction_limit = 2;
            class_run(c, stop, FVM_ERROR);
            if (!(order_count == 2))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            active(a, false);
            active(b, false);
            rt->config.instruction_limit = saved_limit;
            if (!(fvm_audio_channel_close(a.channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_audio_channel_close(b.channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_audio_channel_close(idle.channel) == FVM_OK))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            if (!(fvm_audio_channel_close(peer.channel) == FVM_OK && peer.stops == 1))
            {
                fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
                exit(EXIT_FAILURE);
            }
            fvm_context_destroy(c);
            fvm_context_destroy(other);
        }
    }
    fvm_runtime_destroy(rt);
    return 0;
}
#endif
