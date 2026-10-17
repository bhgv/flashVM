/* OS: all supported targets. Mix borrowed equal-geometry native S16 blocks without allocation or callbacks. */
#include "../common/session.hpp" /* Validate each populated borrowed media view before touching output. */
#include "mix_constants.h" /* Keep representation boundaries and native byte geometry named. */
#include <limits.h> /* Require exact portable S16 host bounds. */
#include <cstring> /* Read unaligned native channel values and initialize output metadata. */
/* Accumulate all inputs in signed long, then saturate once so input order cannot change clipping. */
FVM_Result fvm_audio_mix_s16(const FVM_MediaFrame *inputs, size_t input_count, /* Borrow the bounded source table. */
                              unsigned int sample_rate, unsigned int channels, size_t frames, /* Select common output geometry. */
                              short *destination, size_t capacity, FVM_MediaFrame *out) /* Borrow caller storage; transfer no ownership. */
{ /* Begin validation before publishing any output samples or metadata. */
    if (!out || !sample_rate || !channels || input_count > FVM_MEDIA_AUDIO_MIX_MAX_INPUTS || /* Require explicit output geometry and a bounded source count. */
        (input_count && !inputs)) /* A nonempty table must be accessible. */
        return FVM_INVALID_ARGUMENT; /* Leave the caller's PCM and output view unchanged. */
    if (CHAR_BIT != FVM_AUDIO_MIX_BYTE_BITS || sizeof(short) != FVM_AUDIO_MIX_S16_BYTES || /* Require octet-based native sample geometry. */
        SHRT_MIN != FVM_AUDIO_MIX_S16_MIN || SHRT_MAX != FVM_AUDIO_MIX_S16_MAX) /* Require exact signed-16 host bounds. */
        return FVM_UNSUPPORTED; /* Do not reinterpret an incompatible host's short values. */
    if (frames > capacity / channels || frames > (size_t)-1 / sizeof(short) / channels || /* Bound both channel-value capacity and published bytes. */
        (frames && !destination)) /* Require accessible output for every nonempty request. */
        return FVM_INVALID_ARGUMENT; /* An empty destination may be null only for zero requested frames. */
    for (size_t i = 0; i < input_count; ++i) /* Validate every input, including records whose tails will be ignored. */
    { /* Require a common clock/layout before summing any native value. */
        const FVM_MediaFrame &input = inputs[i]; /* Borrow this metadata record during the synchronous operation. */
        if (input.kind != FVM_MEDIA_AUDIO || input.sample_rate != sample_rate || input.channels != channels) /* Never silently resample or remix layouts. */
            return FVM_INVALID_ARGUMENT; /* Reject mismatched geometry atomically. */
        if (input.samples ? !fvm_media_frame_valid(&input, FVM_MEDIA_AUDIO) : input.size != 0) /* Permit empty zero-byte views for exhausted voices; populated views must be entirely bounded. */
            return FVM_INVALID_ARGUMENT; /* Do not write an earlier valid input before discovering a later invalid one. */
    } /* Finish complete input validation. */
    for (size_t frame = 0; frame < frames; ++frame) /* Produce exactly the requested destination frame count. */
    { /* Treat every absent short tail as digital silence. */
        for (unsigned int channel = 0; channel < channels; ++channel) /* Preserve existing channel order without downmixing. */
        { /* Sum this destination channel independently. */
            long sum = 0; /* At most 1024 S16 values fit exactly inside every supported signed long. */
            size_t index = frame * channels + channel; /* Earlier checks bound this complete destination channel value. */
            for (size_t i = 0; i < input_count; ++i) /* Include every voice before making a clipping decision. */
            { /* Inspect one source's current time-aligned channel value. */
                if (frame < inputs[i].samples) /* Omit exhausted input tails rather than reading beyond them. */
                { /* Read native S16 even when the borrowed bytes are unaligned. */
                    short value; /* Own one bounded native value on the stack. */
                    std::memcpy(&value, inputs[i].data + index * sizeof(short), sizeof(value)); /* Validated matching geometry bounds this read. */
                    sum += value; /* The public input-count limit prevents signed overflow. */
                } /* Finish a populated source value. */
            } /* Finish the exact complete sum. */
            if (sum < FVM_AUDIO_MIX_S16_MIN) /* Saturate only after cancellation across all sources. */
                sum = FVM_AUDIO_MIX_S16_MIN; /* Preserve the most negative representable output. */
            if (sum > FVM_AUDIO_MIX_S16_MAX) /* Saturate positive overflow independently. */
                sum = FVM_AUDIO_MIX_S16_MAX; /* Preserve the largest representable output. */
            destination[index] = (short)sum; /* Store one fully summed native S16 value. */
        } /* Finish every channel of this output frame. */
    } /* Finish the complete destination block. */
    FVM_MediaFrame result; /* Build output metadata only after all input reads and PCM writes succeed. */
    std::memset(&result, 0, sizeof(result)); /* Clear timestamps/video fields because this operation performs no scheduling. */
    result.kind = FVM_MEDIA_AUDIO; /* Mark the returned view as native interleaved S16. */
    result.data = reinterpret_cast<const unsigned char *>(destination); /* Borrow caller-owned PCM until that caller reuses it. */
    result.size = frames * channels * sizeof(short); /* Publish the bounded initialized byte count. */
    result.sample_rate = sample_rate; /* Retain the explicitly selected common source clock. */
    result.channels = channels; /* Retain the explicitly selected common layout. */
    result.samples = frames; /* Include requested silence after short or missing inputs. */
    *out = result; /* Commit the complete output view after successful mixing. */
    return FVM_OK; /* [GC:] No allocation, VM access, collection, host callback or device operation occurred. */
} /* Finish portable native PCM mixing. */
