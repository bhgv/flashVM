/* Empty/paused priming, atomic retry, monotonic dequeue clock and explicit reset. */
#include "swf_sound_output_fixture.hpp"
using namespace output_fixture;
int main()
{
    FVM_Config cfg;
    FVM_Runtime *runtime = 0;
    FVM_Context *context = 0;
    assert(fvm_config_init(&cfg) == FVM_OK && fvm_runtime_create(&cfg, &runtime) == FVM_OK);
    assert(fvm_context_create(runtime, &context) == FVM_OK);
    conversion_fixture::Samples samples;
    for (unsigned i = 0; i < 17; ++i)
        samples.push_back((short)((long)i * 3000 - 24000));
    conversion_fixture::Bytes swf = conversion_fixture::movie(samples, 3, 1);
    assert(fvm_load_memory(context, swf.begin(), swf.size()) == FVM_OK);
    FVM_SWFSoundMixer *mixer = conversion_fixture::create(48000, 2);
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    Device device;
    FVM_AudioQueueBackend table = backend(device);
    FVM_SWFSoundOutput *output = 0;
    assert(fvm_swf_sound_output_create(mixer, &table, 4, 8, &output) == FVM_OK && output &&
           device.paused);
    device.output = output;
    device.reenter = true;
    size_t accepted = 99;
    device.failure = 1;
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_ERROR && !accepted &&
           !device.queued);
    FVM_SWFSoundOutputInfo state = info(output);
    assert(state.pending_frames == 4 && !state.submitted_frames && !state.played_frames);
    for (int failure = 5; failure <= 6; ++failure)
    {
        device.failure = failure;
        assert(fvm_swf_sound_output_pump(output, &accepted) ==
               (failure == 5 ? FVM_OUT_OF_MEMORY : FVM_ERROR));
        assert(!accepted && !device.queued && info(output).pending_frames == 4);
    }
    device.failure = 4;
    assert(fvm_swf_sound_output_stop(output) == FVM_ERROR);
    assert(info(output).pending_frames == 4 && conversion_fixture::active(mixer));
    device.failure = 0;
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4 &&
           device.queued == 4);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4 &&
           device.queued == 8);
    size_t count = device.values;
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_NOT_FOUND && !accepted &&
           device.values == count);
    state = info(output);
    assert(state.submitted_frames == 8 && !state.played_frames && state.paused);
    consume(device, 8);
    assert(device.queued == 8); /* Pause keeps real FIFO state unchanged. */
    assert(fvm_swf_sound_output_pause(output, 0) == FVM_OK && !device.paused);
    consume(device, 3);
    state = info(output);
    assert(state.queued_frames == 5 && state.played_frames == 3 && state.seconds == 3.0 / 48000 &&
           !state.starved);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 3);
    consume(device, 8);
    state = info(output);
    assert(state.starved && state.played_frames == 11);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4);
    for (size_t frame = 0; frame < device.values / 2; ++frame)
        for (unsigned channel = 0; channel < 2; ++channel)
            assert(device.history[frame * 2 + channel] ==
                   conversion_fixture::oracle(samples, 44100, 1, 48000, 2, frame, channel));
    /* Callback errors never overwrite caller info; busy guard is restored after exceptions. */
    FVM_SWFSoundOutputInfo before;
    std::memset(&before, 0x7e, sizeof(before));
    before.struct_size = sizeof(before);
    state = before;
    device.failure = 2;
    assert(fvm_swf_sound_output_info(output, &state) == FVM_ERROR &&
           !std::memcmp(&state, &before, sizeof(state)));
    device.failure = 0;
    for (int thrown = 1; thrown <= 2; ++thrown)
    {
        device.thrown = thrown;
        state = before;
        assert(fvm_swf_sound_output_info(output, &state) ==
               (thrown == 1 ? FVM_OUT_OF_MEMORY : FVM_ERROR));
        assert(!std::memcmp(&state, &before, sizeof(state)));
        device.thrown = 0;
        assert(info(output).submitted_frames == 15);
    }
    device.failure = 3;
    assert(fvm_swf_sound_output_stop(output) == FVM_ERROR && !device.paused);
    device.failure = 4;
    assert(fvm_swf_sound_output_stop(output) == FVM_ERROR && device.paused);
    device.failure = 0;
    assert(fvm_swf_sound_output_stop(output) == FVM_OK && !device.queued &&
           !conversion_fixture::active(mixer));
    state = info(output);
    assert(!state.submitted_frames && !state.played_frames && !state.pending_frames &&
           state.paused);
    /* Successful stop starts a fresh independent dequeue-clock epoch. */
    assert(fvm_swf_sound_mixer_submit(mixer, context, 0) == FVM_OK);
    assert(fvm_swf_sound_output_pump(output, &accepted) == FVM_OK && accepted == 4);
    assert(fvm_swf_sound_output_pause(output, 0) == FVM_OK);
    consume(device, 4);
    assert(info(output).played_frames == 4);
    /* An impossible queue increase is rejected rather than moving the clock backwards. */
    device.queued = 1;
    state = before;
    assert(fvm_swf_sound_output_info(output, &state) == FVM_ERROR &&
           !std::memcmp(&state, &before, sizeof(state)));
    device.queued = 0;
    assert(fvm_swf_sound_output_stop(output) == FVM_OK);
    fvm_swf_sound_output_close(output);
    fvm_swf_sound_mixer_close(mixer);
    fvm_context_destroy(context);
    fvm_runtime_destroy(runtime);
    assert(fvm_swf_sound_output_pump(0, &accepted) == FVM_INVALID_ARGUMENT && !accepted);
    assert(fvm_swf_sound_output_pause(0, 1) == FVM_INVALID_ARGUMENT &&
           fvm_swf_sound_output_stop(0) == FVM_INVALID_ARGUMENT);
    fvm_swf_sound_output_close(0);
    return 0;
}
