#include "flashvm/multimedia.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>

typedef struct Host { int opens, reads, closes, writes, stops, bad; } Host;
typedef struct Stream { Host *host; unsigned char data[8]; int done; } Stream;
static FVM_Result host_open(void *opaque, const FVM_MediaRequest *request, void **out)
{
    Stream *s;
    Host *h = (Host *)opaque;
    assert(request->struct_size == sizeof(*request));
    assert(request->kind == FVM_MEDIA_AUDIO);
    assert(strcmp(request->url, "host:pcm") == 0);
    s = (Stream *)malloc(sizeof(*s));
    if (!s) return FVM_OUT_OF_MEMORY;
    memset(s, 0, sizeof(*s)); s->host = h; ++h->opens; *out = s;
    return FVM_OK;
}
static FVM_Result host_read(void *opaque, FVM_MediaFrame *out)
{
    Stream *s = (Stream *)opaque;
    ++s->host->reads;
    if (s->done) return FVM_NOT_FOUND;
    s->done = 1;
    out->kind = FVM_MEDIA_AUDIO; out->data = s->data; out->size = sizeof(s->data);
    out->sample_rate = 8000; out->channels = 2; out->samples = s->host->bad ? (size_t)-1 : 2;
    return FVM_OK;
}
static FVM_Result host_seek(void *opaque, double seconds)
{
    Stream *s = (Stream *)opaque;
    if (seconds != 0) return FVM_UNSUPPORTED;
    s->done = 0; return FVM_OK;
}
static void host_close(void *opaque)
{
    Stream *s = (Stream *)opaque; ++s->host->closes; free(s);
}
static FVM_Result host_write(void *opaque, const FVM_MediaFrame *frame)
{
    Host *h = (Host *)opaque; assert(frame->samples == 2); ++h->writes; return FVM_OK;
}
static FVM_Result host_stop(void *opaque)
{
    Host *h = (Host *)opaque; ++h->stops; return FVM_OK;
}
static FVM_Result sensor_poll(void *opaque, FVM_DeviceEvent *event)
{
    Host *h = (Host *)opaque;
    event->kind = h->bad ? 999 : FVM_DEVICE_ACCELEROMETER;
    event->x = 1; event->y = 2; event->z = 3; return FVM_OK;
}
int main(void)
{
    Host host;
    FVM_MediaBackend backend, copy;
    FVM_MediaSession *session = 0;
    FVM_MediaFrame frame;
    FVM_MediaOutput output;
    FVM_DeviceBridge device;
    FVM_DeviceEvent event;
    memset(&host, 0, sizeof(host)); memset(&backend, 0, sizeof(backend));
    backend.struct_size = sizeof(backend); backend.abi_version = FVM_MEDIA_ABI_VERSION;
    backend.userdata = &host; backend.open = host_open; backend.read = host_read;
    backend.seek = host_seek; backend.close = host_close;
    assert(fvm_audio_open(0, "host:pcm", &session) == FVM_INVALID_ARGUMENT && !session);
    assert(fvm_audio_open(&backend, "", &session) == FVM_INVALID_ARGUMENT && !session);
    ++backend.abi_version;
    assert(fvm_audio_open(&backend, "host:pcm", &session) == FVM_INVALID_ARGUMENT);
    --backend.abi_version;
    assert(fvm_audio_open(&backend, "host:pcm", &session) == FVM_OK && session);
    copy = backend; memset(&backend, 0, sizeof(backend));
    assert(fvm_media_read(session, &frame) == FVM_OK && frame.samples == 2);
    memset(&output, 0, sizeof(output)); output.struct_size = sizeof(output);
    output.userdata = &host; output.write = host_write; output.stop = host_stop;
    assert(fvm_audio_write(&output, &frame) == FVM_OK && host.writes == 1);
    assert(fvm_video_present(&output, &frame) == FVM_INVALID_ARGUMENT);
    assert(fvm_audio_stop(&output) == FVM_OK && host.stops == 1);
    assert(fvm_media_read(session, &frame) == FVM_NOT_FOUND && !frame.data);
    assert(fvm_media_seek(session, -1) == FVM_INVALID_ARGUMENT);
    assert(fvm_media_seek(session, DBL_MAX) == FVM_UNSUPPORTED);
    assert(fvm_media_seek(session, 0) == FVM_OK);
    host.bad = 1;
    assert(fvm_media_read(session, &frame) == FVM_ERROR && !frame.data && !frame.size);
    fvm_media_close(session); assert(host.opens == 1 && host.closes == 1);
    fvm_media_close(0);
    assert(fvm_camera_open(&copy, "device", "", &session) == FVM_INVALID_ARGUMENT && !session);
    device.struct_size = sizeof(device); device.userdata = &host; device.poll = sensor_poll;
    assert(fvm_device_poll(&device, &event) == FVM_ERROR && event.kind == 0);
    host.bad = 0;
    assert(fvm_device_poll(&device, &event) == FVM_OK && event.z == 3);
    assert(fvm_media_read(0, &frame) == FVM_INVALID_ARGUMENT && !frame.data);
#ifdef NO_MEDIA
    assert(fvm_media_ffmpeg_backend(&backend) == FVM_UNSUPPORTED && !backend.open);
#endif
    return 0;
}
