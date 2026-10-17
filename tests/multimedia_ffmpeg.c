#include "flashvm/multimedia.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef MEDIA
static const unsigned char wav[] = {
    'R','I','F','F',52,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,
    1,0,2,0,64,31,0,0,0,125,0,0,4,0,16,0,'d','a','t','a',16,0,0,0,
    1,0,254,255,3,0,252,255,5,0,250,255,7,0,248,255
};
static const unsigned char ppm[] = {'P','6','\n','2',' ','2','\n','2','5','5','\n',
    255,0,0,0,255,0,0,0,255,255,255,255};
static void save(const char *name, const unsigned char *data, size_t length)
{
    FILE *file = fopen(name,"wb"); assert(file);
    assert(fwrite(data,1,length,file) == length); assert(fclose(file) == 0);
}
#endif
int main(void)
{
#ifdef MEDIA
    FVM_MediaBackend backend;
    FVM_MediaSession *session = 0;
    FVM_MediaFrame frame;
    short values[8];
    save("multimedia_fixture.wav",wav,sizeof(wav));
    save("multimedia_fixture.ppm",ppm,sizeof(ppm));
    assert(fvm_media_ffmpeg_backend(&backend) == FVM_OK);
    assert(fvm_audio_open(&backend,"multimedia_fixture.wav",&session) == FVM_OK);
    assert(fvm_media_read(session,&frame) == FVM_OK);
    assert(frame.kind == FVM_MEDIA_AUDIO && frame.channels == 2 && frame.sample_rate == 8000);
    assert(frame.samples == 4 && frame.size == sizeof(values));
    memcpy(values,frame.data,sizeof(values));
    assert(values[0]==1 && values[1]==-2 && values[6]==7 && values[7]==-8);
    assert(frame.has_timestamp && frame.seconds == 0);
    assert(fvm_media_read(session,&frame) == FVM_NOT_FOUND && !frame.data);
    assert(fvm_media_read(session,&frame) == FVM_NOT_FOUND);
    assert(fvm_media_seek(session,0) == FVM_OK);
    assert(fvm_media_read(session,&frame) == FVM_OK && frame.samples == 4);
    fvm_media_close(session);
    assert(fvm_video_open(&backend,"multimedia_fixture.ppm",&session) == FVM_OK);
    assert(fvm_media_read(session,&frame) == FVM_OK);
    assert(frame.width==2 && frame.height==2 && frame.stride==8 && frame.size==16);
    assert(frame.data[0]==255 && frame.data[1]==0 && frame.data[2]==0 && frame.data[3]==255);
    assert(frame.data[4]==0 && frame.data[5]==255 && frame.data[6]==0 && frame.data[7]==255);
    assert(fvm_media_read(session,&frame) == FVM_NOT_FOUND);
    fvm_media_close(session);
    assert(fvm_camera_open(&backend,"not-a-device","wav",&session) == FVM_UNSUPPORTED && !session);
    assert(fvm_audio_open(&backend,"no-such-multimedia-fixture.wav",&session) == FVM_ERROR && !session);
    save("multimedia_malformed.wav",ppm,4);
    assert(fvm_audio_open(&backend,"multimedia_malformed.wav",&session) == FVM_ERROR && !session);
    assert(remove("multimedia_fixture.wav")==0);
    assert(remove("multimedia_fixture.ppm")==0);
    assert(remove("multimedia_malformed.wav")==0);
#endif
    return 0;
}
