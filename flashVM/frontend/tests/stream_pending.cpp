/* Real native queue/immutable sources with a quiesced worker: completion observation is read-only.
 */
#include "../common/audio.h"
#include "../../tests/swf_sound_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
using namespace sound_fixture;
int main()
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    Bytes tags, head, pcm, empty;
    head.push_back(0x0e);
    head.push_back(0x3e);
    word(head, 1);
    tag(tags, 45, head);
    tag(tags, 45, head);
    word(pcm, 100);
    word(pcm, 200);
    tag(tags, 19, pcm);
    tag(tags, 1, empty);
    tag(tags, 45, head);
    tag(tags, 19, pcm);
    Bytes swf = movie(tags);
    swf[12] = 2;
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
           fvm_standard_install(c) == FVM_OK);
    assert(fvm_movieclip_load_swf_ex(c, swf.begin(), swf.size(), "root",
                                     FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) == FVM_OK &&
           fvm_gc_pin(c, clip) == FVM_OK);
    FVM_DesktopAudio audio;
    assert(fvm_desktop_audio_open_paused(c, clip, &audio, 1) == FVM_OK);
    SDL_AtomicSet(&audio.running, 0);
    SDL_WaitThread(audio.thread, 0);
    audio.thread = 0;
    int pending = 91;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && !pending);
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK &&
           fvm_desktop_audio_timeline(c, clip, &audio) == FVM_OK);
    audio.suspended = 0;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && pending);
    FVM_SWFStreamOutputInfo info;
    info.struct_size = sizeof(info);
    assert(fvm_desktop_audio_stream_info(&audio, &info) == FVM_OK && info.submitted_frames == 0 &&
           audio.stream_index == 0);
    assert(fvm_swf_stream_output_pump_until(&audio.stream_output, 1, 0) == FVM_NOT_FOUND);
    assert(fvm_swf_stream_output_continue(&audio.stream_output,
                                          &audio.stream_segments[1].converter) == FVM_OK);
    audio.stream_index = 1;
    assert(fvm_swf_stream_output_pump_until(&audio.stream_output, 1, 0) == FVM_OK);
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && pending);
    assert(fvm_desktop_audio_stream_info(&audio, &info) == FVM_OK && info.queued_frames == 3 &&
           info.paused);
    assert(fvm_swf_stream_output_seek(&audio.stream_output,
                                      audio.stream_segments[1].converter.block_count, 0) == FVM_OK);
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK &&
           !pending); /* Future frame2 is not authorized. */
    assert(fvm_movieclip_advance(c, clip, 1) == FVM_OK &&
           fvm_desktop_audio_timeline(c, clip, &audio) == FVM_OK);
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK &&
           pending); /* Inactive successor starts at block0. */
    audio.suspended = 1;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && !pending);
    audio.suspended = 0;
    audio.stream_movie_paused = 1;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && !pending);
    audio.stream_movie_paused = 0;
    audio.stream_cancelled = 1;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_OK && !pending);
    audio.stream_cancelled = 0;
    SDL_AtomicSet(&audio.error, FVM_ERROR);
    pending = 91;
    assert(fvm_desktop_audio_stream_pending(&audio, &pending) == FVM_ERROR && pending == 91);
    assert(fvm_desktop_audio_stream_pending(0, &pending) == FVM_INVALID_ARGUMENT && pending == 91);
    fvm_desktop_audio_close(&audio);
    assert(fvm_gc_unpin(c, clip) == FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    return 0;
}
