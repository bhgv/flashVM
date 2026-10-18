/* OS: FFmpeg-supported targets. Convert decoded video/camera frames into RGBA8. */
#include "../common/ffmpeg.hpp" /* Import the private native decoder and scaler types. */
#ifdef MEDIA /* Build conversion only when FFmpeg is enabled. */
/* Convert one image with the SDK's default swscale color policy. */
FVM_Result fvm_ffmpeg_video_convert(FVM_FFmpegStream *stream, /* Return a borrowed tightly packed image. */
                                    FVM_MediaFrame *out) /* Return a borrowed tightly packed image. */
{ /* Begin native image conversion. */
    AVFrame *frame = stream->frame; /* Borrow the current decoder image. */
    int bytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, /* Check dimensions and compute overflow-safe RGBA capacity. */
                                         1); /* Check dimensions and compute overflow-safe RGBA capacity. */
    if (bytes <= 0) /* Reject invalid or unrepresentable images. */
        return FVM_ERROR; /* Reject invalid or unrepresentable images. */
    av_fast_malloc(&stream->buffer, &stream->capacity, (size_t)bytes); /* Reuse or grow native output storage. */
    if (!stream->buffer) /* Require real storage before scaling. */
        return FVM_OUT_OF_MEMORY; /* Require real storage before scaling. */
    stream->scaler = sws_getCachedContext(stream->scaler, frame->width, frame->height, (AVPixelFormat)frame->format, /* Reconfigure the actual scaler when source properties change. */
                                          frame->width, frame->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, 0, 0, /* Reconfigure the actual scaler when source properties change. */
                                          0); /* Reconfigure the actual scaler when source properties change. */
    if (!stream->scaler) /* Reject unavailable conversion state. */
        return FVM_OUT_OF_MEMORY; /* Reject unavailable conversion state. */
    unsigned char *planes[4] = {stream->buffer, 0, 0, 0}; /* Describe one packed RGBA destination plane. */
    int strides[4] = {frame->width * FVM_MEDIA_RGBA_BYTES_PER_PIXEL, 0, 0, /* Use exactly four bytes per destination pixel. */
                      0}; /* Use exactly four bytes per destination pixel. */
    int rows = sws_scale(stream->scaler, frame->data, frame->linesize, 0, frame->height, planes, /* Convert the real decoded image. */
                         strides); /* Convert the real decoded image. */
    if (rows != frame->height) /* Do not expose incomplete destination rows. */
        return FVM_ERROR; /* Do not expose incomplete destination rows. */
    out->kind = FVM_MEDIA_VIDEO; /* Camera and file decoding share this video representation. */
    out->data = stream->buffer; /* Borrow the converted bytes until the next session operation. */
    out->size = (size_t)bytes; /* Expose precisely the initialized image buffer. */
    out->width = (unsigned int)frame->width; /* Preserve source width. */
    out->height = (unsigned int)frame->height; /* Preserve source height. */
    out->stride = (size_t)strides[0]; /* Preserve the tightly packed RGBA row distance. */
    return FVM_OK; /* Leave presentation timing and window ownership to the host. */
} /* Finish native image conversion. */
#endif /* Finish MEDIA-only implementation. */
