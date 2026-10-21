/* Small configured aggregate budget: retained ownership and temporary ADPCM expansion both count.
 */
#include "../common/audio.h"
#include "../../tests/swf_sound_fixture.hpp"
#include "flashvm/movieclip.h"
#include "flashvm/standard.h"
#include "flashvm/gc.h"
#include <cassert>
using namespace sound_fixture;
static Bytes adpcm(unsigned packets)
{
    Bytes data;
    unsigned bits = 2 + packets * (22 + 4095 * 4) * 2;
    data.resize((bits + 7) / 8);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = 0;
    data[0] = 0x80;
    return data;
}
static void append(Bytes &tags, unsigned format, const Bytes &data)
{
    Bytes head, empty;
    head.push_back(0x0e);
    head.push_back((format << 4) | 14 | (format == 1 ? 1 : 0));
    word(head, 1);
    tag(tags, 45, head);
    tag(tags, 19, data);
    tag(tags, 1, empty);
}
static void check(const Bytes &tags, unsigned frames, FVM_Result expected, bool partial)
{
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    FVM_Object *clip = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    Bytes swf = movie(tags);
    swf[12] = (unsigned char)(frames + 1);
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK &&
           fvm_standard_install(c) == FVM_OK);
    assert(fvm_desktop_audio_validate(c) == expected);
    assert(fvm_movieclip_load_swf_ex(c, swf.begin(), swf.size(), "root",
                                     FVM_SWF_IMPORT_MUTE_ROOT_STREAMS, &clip) == FVM_OK &&
           fvm_gc_pin(c, clip) == FVM_OK);
    FVM_DesktopAudio audio;
    FVM_Result result = fvm_desktop_audio_open_paused(c, clip, &audio, 1);
    assert(result == expected);
    if (result != FVM_OK)
    {
        assert(!audio.device && !audio.thread && !audio.mutex && audio.stream_segments);
        assert((audio.stream_segments[0].source != 0) == partial);
    }
    fvm_desktop_audio_close(&audio);
    assert(!audio.stream_segments && !audio.stream_source);
    assert(fvm_gc_unpin(c, clip) == FVM_OK);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
}
int main()
{
    assert(FVM_DESKTOP_AUDIO_PREPARED_BYTES == 65536UL);
    Bytes tags, packet = adpcm(1);
    for (unsigned i = 0; i < 3; ++i)
        append(tags, 1, packet);
    check(tags, 3, FVM_OK,
          false); /* Three retained stereo packets and a final preparation peak fit. */
    append(tags, 1, packet);
    check(tags, 4, FVM_OUT_OF_MEMORY, true);
    /* Final retained bytes fit, but old PCM plus raw+decoded ADPCM exceeds the transient ceiling.
     */
    Bytes pcm;
    pcm.resize(24576);
    for (size_t i = 0; i < pcm.size(); ++i)
        pcm[i] = 0;
    tags.clear();
    append(tags, 3, pcm);
    append(tags, 1, adpcm(2));
    FVM_Config cfg;
    FVM_Runtime *rt = 0;
    FVM_Context *c = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &rt) == FVM_OK &&
           fvm_context_create(rt, &c) == FVM_OK);
    Bytes swf = movie(tags);
    swf[12] = 3;
    assert(fvm_load_memory(c, swf.begin(), swf.size()) == FVM_OK);
    size_t retained = 2 * sizeof(FVM_DesktopStreamSegment), peak = retained;
    for (size_t index = 0; index < 2; ++index)
    {
        FVM_SWFStreamSoundSnapshot *s = 0;
        FVM_SWFStreamSoundStorageInfo info;
        info.struct_size = sizeof(info);
        assert(fvm_swf_stream_sound_snapshot_open_pcm(c, index, &s) == FVM_OK &&
               fvm_swf_stream_sound_snapshot_storage_info(s, &info) == FVM_OK);
        peak = retained + info.peak_bytes;
        retained += info.retained_bytes;
        fvm_swf_stream_sound_snapshot_close(s);
    }
    assert(retained < FVM_DESKTOP_AUDIO_PREPARED_BYTES && peak > FVM_DESKTOP_AUDIO_PREPARED_BYTES);
    fvm_context_destroy(c);
    fvm_runtime_destroy(rt);
    check(tags, 2, FVM_OUT_OF_MEMORY, true);
    return 0;
}
