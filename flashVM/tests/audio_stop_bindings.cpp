/* Native controls stop prepared output, isolate contexts, contain exceptions and survive GC/reentry. */
#include "flashvm/audio_stop.h"
#include "flashvm/multimedia.h"
#include "flashvm/gc.h"
#include "flashvm/standard.h"
#include "../src/core/runtime.hpp"
#include <cassert>
#include <new>
#include <cstring>
struct Host
{
    FVM_Context *context;
    FVM_AudioStopBinding *binding;
    unsigned calls;
    int failure;
    bool reenter;
    Host(FVM_Context *c) : context(c), binding(0), calls(0), failure(0), reenter(true) {}
};
static FVM_Result stop(void *raw)
{
    Host *h = (Host *)raw;
    ++h->calls;
    if(h->reenter)
    {
        FVM_AudioStopBinding *other = (FVM_AudioStopBinding *)1;
        assert(fvm_audio_stop_binding_create(h->context, stop, h, &other) == FVM_INVALID_ARGUMENT && !other);
        assert(fvm_audio_stop_binding_close(h->binding) == FVM_INVALID_ARGUMENT);
        assert(fvm_audio_stop_all(h->context) == FVM_INVALID_ARGUMENT);
        unsigned long reclaimed = 99;
        assert(fvm_gc_collect(h->context, &reclaimed) == FVM_INVALID_ARGUMENT);
        fvm_context_destroy(h->context); /* Guarded void destruction must leave the owner live. */
    }
    if(h->failure == 1) return FVM_UNSUPPORTED;
    if(h->failure == 2) throw std::bad_alloc();
    if(h->failure == 3) throw 17;
    return FVM_OK;
}
static FVM_Result write_pcm(void *,const FVM_MediaFrame *) { return FVM_OK; }
int main()
{
    FVM_Config cfg; FVM_Runtime *runtime=0; FVM_Context *a=0,*b=0;
    assert(fvm_config_init(&cfg)==FVM_OK && fvm_runtime_create(&cfg,&runtime)==FVM_OK);
    assert(fvm_context_create(runtime,&a)==FVM_OK && fvm_context_create(runtime,&b)==FVM_OK);
    Host first(a),last(a),other(b);
    assert(fvm_audio_stop_binding_create(a,stop,&first,&first.binding)==FVM_OK);
    assert(fvm_audio_stop_binding_create(a,stop,&last,&last.binding)==FVM_OK);
    assert(fvm_audio_stop_binding_create(b,stop,&other,&other.binding)==FVM_OK);
    assert(!first.calls && !last.calls && !other.calls);
    for(int scenario=0;scenario<4;++scenario)
    {
        first.failure=scenario;
        FVM_Result expected=scenario==1?FVM_UNSUPPORTED:scenario==2?FVM_OUT_OF_MEMORY:scenario==3?FVM_ERROR:FVM_OK;
        assert(fvm_audio_stop_all(a)==expected);
        assert(first.calls==(unsigned)scenario+1 && last.calls==first.calls && !other.calls);
        assert(!a->audio_busy && !a->gc_native_depth);
    }
    first.failure=0;
    assert(fvm_standard_install(a)==FVM_OK);
    FVM_Value out;
    assert(fvm_call_bytes(a,"stopAllSounds",13,0,0,&out)==FVM_OK && first.calls==5 && last.calls==5);
    assert(fvm_audio_stop_binding_close(first.binding)==FVM_OK && first.calls==5);
    assert(fvm_audio_stop_all(a)==FVM_OK && last.calls==6);
    /* Explicit close does not need a live device and never calls the stop callback. */
    last.failure=3;
    assert(fvm_audio_stop_binding_close(last.binding)==FVM_OK && last.calls==6);
    assert(fvm_audio_stop_binding_close(0)==FVM_OK);
    FVM_AudioStopBinding *invalid=(FVM_AudioStopBinding *)1;
    assert(fvm_audio_stop_binding_create(0,stop,&first,&invalid)==FVM_INVALID_ARGUMENT && !invalid);
    assert(fvm_audio_stop_binding_create(a,0,&first,&invalid)==FVM_INVALID_ARGUMENT && !invalid);
    Host legacy(a),tail(a);
    legacy.reenter=false;legacy.failure=3;
    FVM_MediaOutput media={sizeof(media),&legacy,write_pcm,stop};
    FVM_AudioChannel *channel=0;assert(fvm_audio_channel_create(a,&media,&channel)==FVM_OK);
    unsigned char bytes[2]={0,0};FVM_MediaFrame frame;std::memset(&frame,0,sizeof(frame));
    frame.kind=FVM_MEDIA_AUDIO;frame.data=bytes;frame.size=2;frame.sample_rate=8000;frame.channels=1;frame.samples=1;
    assert(fvm_audio_channel_write(channel,&frame)==FVM_OK);
    assert(fvm_audio_stop_binding_create(a,stop,&tail,&tail.binding)==FVM_OK);
    assert(fvm_audio_stop_all(a)==FVM_ERROR && legacy.calls==1 && tail.calls==1);
    assert(fvm_audio_channel_close(channel)==FVM_ERROR && legacy.calls==2);
    legacy.failure=0;
    assert(fvm_audio_channel_close(channel)==FVM_OK && legacy.calls==3);
    assert(fvm_audio_stop_binding_close(tail.binding)==FVM_OK);
    /* Context teardown contains terminal errors and still releases every registration. */
    Host terminal(a),after(a);
    terminal.failure=2;
    assert(fvm_audio_stop_binding_create(a,stop,&terminal,&terminal.binding)==FVM_OK);
    assert(fvm_audio_stop_binding_create(a,stop,&after,&after.binding)==FVM_OK);
    fvm_context_destroy(a);
    assert(terminal.calls==1 && after.calls==1);
    fvm_context_destroy(b);assert(other.calls==1);
    fvm_runtime_destroy(runtime);
    return 0;
}
