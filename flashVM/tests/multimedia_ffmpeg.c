#include <stdlib.h>
#include "flashvm/multimedia.h"
#include "flashvm/avm1.h"
#include <stdio.h>
#include <string.h>

#ifdef MEDIA
typedef struct PcmCollector
{
    short values[8];
    unsigned writes, stops;
} PcmCollector;


static FVM_Result collect_pcm(void *raw, const FVM_MediaFrame *frame)
{
    PcmCollector *sink = (PcmCollector *)raw;
    if (!(frame->size == sizeof(sink->values)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memcpy(sink->values, frame->data, frame->size);
    ++sink->writes;
    return FVM_OK;
}


static FVM_Result stop_pcm(void *raw)
{
    PcmCollector *sink = (PcmCollector *)raw;
    ++sink->stops;
    return FVM_OK;
}

static const unsigned char wav[] = {'R', 'I', 'F', 'F', 52, 0,   0,   0,   'W', 'A', 'V', 'E', 'f', 'm', 't',
                                    ' ', 16,  0,   0,   0,  1,   0,   2,   0,   64,  31,  0,   0,   0,   125,
                                    0,   0,   4,   0,   16, 0,   'd', 'a', 't', 'a', 16,  0,   0,   0,   1,
                                    0,   254, 255, 3,   0,  252, 255, 5,   0,   250, 255, 7,   0,   248, 255};
static const unsigned char ppm[] = {'P', '6', '\n', '2', ' ', '2', '\n', '2', '5', '5', '\n', 255,
                                    0,   0,   0,    255, 0,   0,   0,    255, 255, 255, 255};


static void save(const char *name, const unsigned char *data, size_t length)
{
    FILE *file = fopen(name, "wb");
    if (!(file))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fwrite(data, 1, length, file) == length))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fclose(file) == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
}
#endif


int main(void)
{
#ifdef MEDIA
    FVM_MediaBackend backend;
    FVM_MediaSession *session = 0;
    FVM_MediaFrame frame;
    short values[8];
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    FVM_AudioChannel *channel = 0;
    FVM_MediaOutput output;
    PcmCollector sink;
    const unsigned char stop[] = {9, 0};
    memset(&sink, 0, sizeof(sink));
    if (!(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK &&
          fvm_context_create(runtime, &context) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    output.struct_size = sizeof(output);
    output.userdata = &sink;
    output.write = collect_pcm;
    output.stop = stop_pcm;
    if (!(fvm_audio_channel_create(context, &output, &channel) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    save("multimedia_fixture.wav", wav, sizeof(wav));
    save("multimedia_fixture.ppm", ppm, sizeof(ppm));
    if (!(fvm_media_ffmpeg_backend(&backend) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_open(&backend, "multimedia_fixture.wav", &session) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.kind == FVM_MEDIA_AUDIO && frame.channels == 2 && frame.sample_rate == 8000))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.samples == 4 && frame.size == sizeof(values)))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_write(channel, &frame) == FVM_OK && sink.writes == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_avm1_execute(context, stop, sizeof(stop)) == FVM_OK && sink.stops == 1))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    memcpy(values, frame.data, sizeof(values));
    if (!(!memcmp(values, sink.values, sizeof(values))))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(values[0] == 1 && values[1] == -2 && values[6] == 7 && values[7] == -8))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.has_timestamp && frame.seconds == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_NOT_FOUND && !frame.data))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_seek(session, 0) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_OK && frame.samples == 4))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_channel_write(channel, &frame) == FVM_OK && sink.writes == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_context_destroy(context);
    if (!(sink.stops == 2))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_runtime_destroy(runtime);
    fvm_media_close(session);
    if (!(fvm_video_open(&backend, "multimedia_fixture.ppm", &session) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_OK))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.width == 2 && frame.height == 2 && frame.stride == 8 && frame.size == 16))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.data[0] == 255 && frame.data[1] == 0 && frame.data[2] == 0 && frame.data[3] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(frame.data[4] == 0 && frame.data[5] == 255 && frame.data[6] == 0 && frame.data[7] == 255))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_media_read(session, &frame) == FVM_NOT_FOUND))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    fvm_media_close(session);
    if (!(fvm_camera_open(&backend, "not-a-device", "wav", &session) == FVM_UNSUPPORTED && !session))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(fvm_audio_open(&backend, "no-such-multimedia-fixture.wav", &session) == FVM_ERROR && !session))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    save("multimedia_malformed.wav", ppm, 4);
    if (!(fvm_audio_open(&backend, "multimedia_malformed.wav", &session) == FVM_ERROR && !session))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(remove("multimedia_fixture.wav") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(remove("multimedia_fixture.ppm") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
    if (!(remove("multimedia_malformed.wav") == 0))
    {
        fprintf(stderr, "%s:%d: condition check failed\n", __FILE__, __LINE__);
        exit(EXIT_FAILURE);
    }
#endif
    return 0;
}
