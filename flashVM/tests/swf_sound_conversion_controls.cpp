/* Buffered last samples remain active for SyncNoMultiple and are discarded by immediate stop. */
#include "swf_sound_conversion_fixture.hpp"
#include "flashvm/gc.h"
using namespace conversion_fixture;
int main()
{
    FVM_Config config;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&config) == FVM_OK && fvm_runtime_create(&config, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    Samples samples;
    samples.push_back(-12345);
    Bytes swf = movie(samples, 3, 1);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFSoundMixer *mixer = create(192000, 2, 1);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    short out[16];
    size_t written = 0;
    for (unsigned i = 0; i < 4; ++i)
    {
        assert(fvm_swf_sound_mixer_read(mixer, 1, out, 2, &written) == FVM_OK && written == 2);
        assert(out[0] == -12345 && out[1] == -12345 && active(mixer) == 1);
        assert(fvm_swf_sound_mixer_submit(mixer, context, 1) == FVM_OK && active(mixer) == 1);
    }
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OUT_OF_MEMORY &&
           active(mixer) == 1);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 2) == FVM_OK && !active(mixer));
    assert(fvm_swf_sound_mixer_read(mixer, 1, out, 2, &written) == FVM_OK && !out[0] && !out[1]);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    assert(fvm_gc_collect(context, 0) == FVM_OK);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    /* Slot reuse resets phase and buffered frames; source VM is no longer needed. */
    assert(fvm_swf_sound_mixer_read(mixer, 8, out, 16, &written) == FVM_OK && written == 16);
    for (unsigned i = 0; i < 8; ++i)
        assert(out[i * 2] == (i < 5 ? -12345 : 0) && out[i * 2 + 1] == out[i * 2]);
    assert(!active(mixer));
    fvm_swf_sound_mixer_close(mixer);
    return 0;
}
