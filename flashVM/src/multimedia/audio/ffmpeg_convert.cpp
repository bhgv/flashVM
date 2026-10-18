/* OS: FFmpeg-supported targets. Convert actual decoded audio to interleaved native S16. */
#include "../common/ffmpeg.hpp" /* Import the decoder frame and output view. */
#ifdef MEDIA /* Build the converter only with the static FFmpeg SDK. */
#include <errno.h> /* Identify native allocation failures. */
#include <limits.h> /* Bound the byte count accepted by av_fast_malloc. */
/* Convert at the source rate and layout; no implicit playback or sample-rate policy. */
FVM_Result fvm_ffmpeg_audio_convert(FVM_FFmpegStream *stream, /* Keep the converted buffer owned by the stream. */
                                    FVM_MediaFrame *out) /* Keep the converted buffer owned by the stream. */
{ /* Begin PCM layout conversion. */
    AVFrame *frame = stream->frame; /* Borrow the current decoder output. */
    if (frame->sample_rate <= 0 || frame->nb_samples <= 0 || frame->ch_layout.nb_channels <= 0 || sizeof(short) != 2 || /* Require the documented PCM representation. */
        SHRT_MAX != 32767) /* Require the documented PCM representation. */
        return FVM_UNSUPPORTED; /* Require the documented PCM representation. */
    SwrContext *converter = 0; /* Own a converter for this native sample format/layout. */
    int result = swr_alloc_set_opts2(&converter, &frame->ch_layout, AV_SAMPLE_FMT_S16, frame->sample_rate, /* Preserve rate/layout while converting planar or packed samples. */
                                     &frame->ch_layout, (AVSampleFormat)frame->format, frame->sample_rate, 0, /* Preserve rate/layout while converting planar or packed samples. */
                                     0); /* Preserve rate/layout while converting planar or packed samples. */
    if (result < 0 || !converter) /* Release partial state even if SDK initialization failed. */
    { /* Begin allocation-failure cleanup. */
        swr_free(&converter); /* Tolerate null and free partial converter ownership. */
        return result == AVERROR(ENOMEM) ? FVM_OUT_OF_MEMORY /* Return the result produced by the following backend operation. */
                                         : FVM_ERROR; /* Preserve native allocation versus setup failure. */
    } /* Finish converter-allocation cleanup. */
    result = swr_init(converter); /* Initialize the real FFmpeg sample converter. */
    int bytes = av_samples_get_buffer_size(0, frame->ch_layout.nb_channels, frame->nb_samples, AV_SAMPLE_FMT_S16, /* Compute checked packed-PCM storage. */
                                           1); /* Compute checked packed-PCM storage. */
    if (result < 0 || bytes <= 0) /* Reject unsupported layouts or overflowing frame sizes. */
    { /* Release the temporary converter before returning. */
        swr_free(&converter); /* Free any initialized converter state. */
        return FVM_ERROR; /* Do not expose partial PCM output. */
    } /* Finish conversion setup failure. */
    av_fast_malloc(&stream->buffer, &stream->capacity, /* Reuse or grow the library-owned output allocation. */
                   (size_t)bytes); /* Reuse or grow the library-owned output allocation. */
    if (!stream->buffer) /* Check native output allocation. */
    { /* Release temporary converter ownership. */
        swr_free(&converter); /* Avoid leaking conversion state on allocation failure. */
        return FVM_OUT_OF_MEMORY; /* Report absent output storage. */
    } /* Finish output allocation failure. */
    unsigned char *planes[1] = {stream->buffer}; /* Describe one packed interleaved PCM plane. */
    result = swr_convert(converter, planes, frame->nb_samples, (const unsigned char **)frame->extended_data, /* Convert actual decoder samples without resampling. */
                         frame->nb_samples); /* Convert actual decoder samples without resampling. */
    swr_free(&converter); /* Same-rate format conversion has no delayed resampling tail. */
    if (result < 0) /* Reject conversion failure before publishing the view. */
        return FVM_ERROR; /* Reject conversion failure before publishing the view. */
    out->kind = FVM_MEDIA_AUDIO; /* Mark the borrowed buffer as audio. */
    out->data = stream->buffer; /* Borrow the converted allocation until the next operation. */
    out->channels = /* Preserve the source channel count and native layout order. */
        (unsigned int)frame->ch_layout.nb_channels; /* Preserve the source channel count and native layout order. */
    out->sample_rate = (unsigned int)frame->sample_rate; /* Preserve the source sample rate. */
    out->samples = (size_t)result; /* Report actual output sample frames. */
    out->size = out->samples * out->channels * sizeof(short); /* Report only initialized packed PCM bytes. */
    return FVM_OK; /* Publish converted PCM without opening a playback device. */
} /* Finish native audio conversion. */
#endif /* Finish MEDIA-only implementation. */
