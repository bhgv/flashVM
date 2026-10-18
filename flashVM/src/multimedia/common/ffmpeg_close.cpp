/* OS: FFmpeg-supported targets. Release native decoder/capture resources. */
#include "ffmpeg.hpp" /* Import native state only for MEDIA builds. */
#ifdef MEDIA /* Avoid third-party symbols in NO_MEDIA builds. */
/* Release both complete and partially constructed streams. */
void fvm_ffmpeg_close(void *opaque) /* Accept null for shared failure cleanup. */
{ /* Begin native cleanup. */
    FVM_FFmpegStream *stream = static_cast<FVM_FFmpegStream *>(opaque); /* Recover the owned native state. */
    if (!stream) /* Tolerate a missing partial handle. */
        return; /* Tolerate a missing partial handle. */
    sws_freeContext(stream->scaler); /* Release the cached image converter first. */
    av_free(stream->buffer); /* Release all converted PCM/RGBA storage. */
    av_frame_free(&stream->frame); /* Release the decoded native frame and its references. */
    av_packet_free(&stream->packet); /* Release the reusable compressed packet. */
    avcodec_free_context(&stream->codec); /* Stop and release the real decoder. */
    avformat_close_input(&stream->format); /* Close the file or native capture device. */
    delete stream; /* Release the unmanaged ownership wrapper. */
} /* Finish native cleanup. */
#endif /* Finish MEDIA-only implementation. */
