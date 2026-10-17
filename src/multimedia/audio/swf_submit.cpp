/* OS: host-selected output. Explicitly submit a bounded PCM range from loaded SWF bytes. */
#include "channel.hpp" /* Validate channel ownership and retain the ordinary output callback barrier. */
#include "flashvm/swf_sound.h" /* Reuse bounded root sound lookup and caller-buffer PCM conversion. */
#include <cstring> /* Initialize unused frame fields before submission. */
#include <new> /* Contain temporary PCM allocation failures at the C ABI. */
/* Decode into native temporary storage before entering a host callback which may reload its context. */
FVM_Result fvm_swf_sound_submit(FVM_Context *context, unsigned int id, FVM_AudioChannel *channel, unsigned long first, size_t frames) /* Borrow the live channel only during this call. */
{ /* Begin explicit SWF PCM submission. */
    if (!channel || channel->context != context || !fvm_audio_context_valid(context)) /* Reject foreign owners and callback mutation. */
        return FVM_INVALID_ARGUMENT; /* Do not inspect or open output for an invalid channel. */
    FVM_SWFSoundInfo info; /* Borrow header metadata only before entering host output. */
    info.struct_size = sizeof(info); /* Declare the exact current inspection ABI. */
    FVM_Result result = fvm_swf_sound_info(context, id, &info); /* Validate the selected root sound and PCM payload bounds. */
    if (result != FVM_OK) /* Preserve missing, unsupported-context and malformed-data statuses. */
        return result; /* Nothing has been allocated or submitted. */
    if (first > info.sample_count || frames > info.sample_count - first) /* Bound the requested sample-frame interval. */
        return FVM_INVALID_ARGUMENT; /* Avoid overflow before sizing the temporary buffer. */
    size_t validated = 0; /* Check codec and host representation before allocating conversion storage. */
    result = fvm_swf_sound_read_pcm(context, id, first, 0, 0, 0, &validated); /* A zero-length read validates the selected PCM format without touching output. */
    if (result != FVM_OK) /* Unsupported codec/width is independent of temporary memory limits. */
        return result; /* Do not allocate or enter output for an unsupported sound. */
    size_t limit = context->runtime->config.memory_limit_bytes ? context->runtime->config.memory_limit_bytes : FVM_SWF_DEFAULT_LIMIT; /* Bound native temporary storage independently of managed GC. */
    if (frames > limit / sizeof(short) / info.channels) /* Avoid multiplication overflow and excessive temporary PCM. */
        return FVM_OUT_OF_MEMORY; /* The host may request smaller explicit chunks. */
    try /* Contain temporary native-buffer allocation failure. */
    { /* Construct a callback-independent copied PCM view. */
        fvm_detail::vector<short> samples(frames * info.channels); /* Own converted samples until host write returns. */
        size_t written = 0; /* Count converted channel values, not sample frames. */
        result = fvm_swf_sound_read_pcm(context, id, first, frames, samples.empty() ? 0 : samples.begin(), samples.size(), &written); /* Convert without calling host code. */
        if (result != FVM_OK || !frames) /* Empty valid PCM ranges do not activate an output. */
            return result; /* Preserve unsupported codec/width errors even for an empty range. */
        FVM_MediaFrame frame; /* Build a synchronous borrowed native-S16 frame. */
        std::memset(&frame, 0, sizeof(frame)); /* Clear geometry fields not used by audio. */
        frame.kind = FVM_MEDIA_AUDIO; /* Select the shared PCM bridge representation. */
        frame.data = reinterpret_cast<const unsigned char *>(samples.begin()); /* Borrow the native copy rather than reloadable SWF bytes. */
        frame.size = written * sizeof(short); /* Publish only the accessible converted channel values. */
        frame.sample_rate = info.sample_rate; /* Preserve the nominal SWF header rate. */
        frame.channels = info.channels; /* Preserve mono/stereo interleaving. */
        frame.samples = frames; /* Count complete sample frames. */
        frame.has_timestamp = 1; /* This explicit range has a source-relative timestamp. */
        frame.seconds = (double)first / info.sample_rate; /* Report source offset, not a scheduled playback clock. */
        return fvm_audio_channel_write(channel, &frame); /* [GC:] Enter the guarded real host output callback exactly once. */
    } /* Finish copied-range submission. */
    catch (const std::bad_alloc &) /* Translate temporary allocation failure without output replay. */
    { /* Leave the channel's active state unchanged. */
        return FVM_OUT_OF_MEMORY; /* The caller can retry a smaller range explicitly. */
    } /* Finish allocation failure containment. */
} /* Finish explicit SWF PCM output. */
