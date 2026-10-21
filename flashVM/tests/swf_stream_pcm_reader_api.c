/* C89 header/link contract for the native prepared streaming PCM reader. */
#include "flashvm/swf_stream_pcm.h"
#include "flashvm/swf_stream_convert.h"
#include "flashvm/swf_stream_output.h"
#include "flashvm/movieclip.h"
/* Invalid input must remain callable without initializing a VM or audio device. */
int main(void)
{
    FVM_SWFStreamSoundSnapshot *snapshot = (FVM_SWFStreamSoundSnapshot *)1;
    FVM_SWFStreamSoundStorageInfo storage;
    FVM_MovieClipPlaybackInfo playback;
    unsigned long navigation = 91;
    FVM_SWFStreamConverter converter;
    FVM_SWFStreamPCMReader reader;
    FVM_SWFStreamPCMChunk chunk;
    size_t written = 999;
    if (fvm_swf_stream_sound_snapshot_open_pcm(0, 0, &snapshot) != FVM_INVALID_ARGUMENT || snapshot)
        return 1;
    if (fvm_swf_stream_sound_snapshot_open_bounded(0, 0, 0, &snapshot) != FVM_INVALID_ARGUMENT || snapshot)
        return 1;
    storage.struct_size = sizeof(storage);
    storage.retained_bytes = 91;
    if (fvm_swf_stream_sound_snapshot_storage_info(0, &storage) != FVM_INVALID_ARGUMENT ||
        storage.retained_bytes != 91 ||
        fvm_swf_stream_sound_snapshot_open_pcm_bounded(0, 0, 0, &snapshot) != FVM_INVALID_ARGUMENT || snapshot)
        return 1;
    playback.struct_size = sizeof(playback);
    if (fvm_movieclip_get_playback_info(0, 0, &playback) != FVM_INVALID_ARGUMENT ||
        fvm_movieclip_get_navigation(0, 0, &navigation) != FVM_INVALID_ARGUMENT || navigation != 91)
        return 1;

    if (fvm_swf_stream_converter_init_continuous(&converter, 0, 48000, 2) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_converter_init(&converter, 0, 48000, 2) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_converter_next(0, 1, 0, 0, 0, 0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_converter_seek(0, 0, 0) != FVM_INVALID_ARGUMENT) return 1;
    if (fvm_swf_stream_output_init(0,0,0,0,0,0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_pump(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_pump_until(0,0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_next_frame(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_pause(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_info(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_seek(0,0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_seek_frame(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_continue(0,0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_output_bind_frame(0,0,0) != FVM_INVALID_ARGUMENT) return 1;
    chunk.struct_size = sizeof(chunk);
    if (fvm_swf_stream_pcm_reader_init(&reader, 0) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_pcm_reader_next(0, 1, 0, 0, &chunk, &written) != FVM_INVALID_ARGUMENT ||
        fvm_swf_stream_pcm_reader_seek(0, 0, 0) != FVM_INVALID_ARGUMENT || written != 999)
        return 1;
    return 0;
}
