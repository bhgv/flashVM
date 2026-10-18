/* Explicit FIFO model: copied PCM, injectable callbacks and controlled consumption. */
#ifndef FVM_TEST_SOUND_OUTPUT_FIXTURE_HPP
#define FVM_TEST_SOUND_OUTPUT_FIXTURE_HPP
#include "flashvm/swf_sound_output.h"
#include "swf_sound_conversion_fixture.hpp"
#include <new>
namespace output_fixture
{
struct Device
{
    size_t queued, values;
    short history[32768];
    bool paused, reenter;
    int failure, thrown;
    FVM_SWFSoundOutput *output;
    Device() : queued(0), values(0), paused(true), reenter(false), failure(0), thrown(0), output(0)
    {
    }
};
inline void reentry(Device *d)
{
    if (!d->reenter || !d->output)
        return;
    size_t written = 99;
    FVM_SWFSoundOutputInfo info;
    info.struct_size = sizeof(info);
    assert(fvm_swf_sound_output_pump(d->output, &written) == FVM_INVALID_ARGUMENT && !written);
    assert(fvm_swf_sound_output_info(d->output, &info) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_pause(d->output, 1) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_stop(d->output) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_set_release(d->output, 2) == FVM_INVALID_ARGUMENT);
    assert(fvm_swf_sound_output_stop_release(d->output) == FVM_INVALID_ARGUMENT);
    fvm_swf_sound_output_close(d->output); /* Busy close is deliberately a no-op. */
}
inline void exception(Device *d)
{
    if (d->thrown == 1)
        throw std::bad_alloc();
    if (d->thrown == 2)
        throw 7;
}
inline FVM_Result write(void *raw, const short *pcm, size_t frames)
{
    Device *d = (Device *)raw;
    reentry(d);
    exception(d);
    if (d->failure == 5)
        throw std::bad_alloc();
    if (d->failure == 6)
        throw 42;
    if (d->failure == 1)
        return FVM_ERROR;
    assert(d->values + frames * 2 <= sizeof(d->history) / sizeof(d->history[0]));
    for (size_t i = 0; i < frames * 2; ++i)
        d->history[d->values++] = pcm[i];
    d->queued += frames;
    return FVM_OK;
}
inline FVM_Result queued(void *raw, size_t *frames)
{
    Device *d = (Device *)raw;
    reentry(d);
    exception(d);
    if (d->failure == 2)
        return FVM_ERROR;
    *frames = d->queued;
    return FVM_OK;
}
inline FVM_Result pause(void *raw, int paused)
{
    Device *d = (Device *)raw;
    reentry(d);
    exception(d);
    if (d->failure == 3)
        return FVM_ERROR;
    d->paused = paused != 0;
    return FVM_OK;
}
inline FVM_Result clear(void *raw)
{
    Device *d = (Device *)raw;
    reentry(d);
    exception(d);
    if (d->failure == 4)
        return FVM_ERROR;
    d->queued = 0;
    return FVM_OK;
}
inline FVM_AudioQueueBackend backend(Device &d)
{
    FVM_AudioQueueBackend table = {
        sizeof(table), FVM_AUDIO_QUEUE_ABI_VERSION, 48000, 2, &d, write, queued, pause, clear};
    return table;
}
inline void consume(Device &d, size_t frames)
{
    if (!d.paused)
        d.queued -= frames < d.queued ? frames : d.queued;
}
inline FVM_SWFSoundOutputInfo info(FVM_SWFSoundOutput *output)
{
    FVM_SWFSoundOutputInfo result;
    result.struct_size = sizeof(result);
    assert(fvm_swf_sound_output_info(output, &result) == FVM_OK);
    return result;
}
} // namespace output_fixture
#endif
