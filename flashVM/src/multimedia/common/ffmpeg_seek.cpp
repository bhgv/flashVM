/* OS: FFmpeg-supported targets; files must expose a seekable demuxer. */
#include "ffmpeg.hpp" /* Import native time-base and decoder APIs. */
#ifdef MEDIA          /* Omit FFmpeg references from NO_MEDIA builds. */


/* Seek to a preceding keyframe and discard all old decoder references. */
FVM_Result fvm_ffmpeg_seek(void *opaque, double seconds)                /* The common layer already rejects nonfinite/negative time. */
{                                                                       /* Begin real demuxer seeking. */
    FVM_FFmpegStream *stream = static_cast<FVM_FFmpegStream *>(opaque); /* Recover this session's native state. */
    if (seconds >= (double)INT64_MAX / AV_TIME_BASE)                    /* Check this precondition before entering the following operation. */
    {                                                                   /* Begin conditional branch: Check this precondition before entering the following operation. */
        return FVM_INVALID_ARGUMENT;                                    /* Avoid overflowing FFmpeg's signed timestamp representation.
                                      */
    } /* End conditional branch: Check this precondition before entering the following operation. */
    int64_t timestamp = (int64_t)(seconds * AV_TIME_BASE);                      /* Convert seconds to FFmpeg's global microsecond clock. */
    if (av_seek_frame(stream->format, -1, timestamp, AVSEEK_FLAG_BACKWARD) < 0) /* Report an unavailable or unsuccessful seek explicitly. */
    {                                                                           /* Begin conditional branch: Report an unavailable or unsuccessful seek explicitly. */
        return FVM_UNSUPPORTED;                                                 /* Report an unavailable or unsuccessful seek explicitly. */
    } /* End conditional branch: Report an unavailable or unsuccessful seek explicitly. */
    avcodec_flush_buffers(stream->codec); /* Drop every queued decoded/compressed codec frame. */
    av_packet_unref(stream->packet);      /* Release any previous demuxed packet. */
    av_frame_unref(stream->frame);        /* Release the previous native decoded frame. */
    stream->input_eof = false;            /* Allow fresh demuxer input after EOF. */
    stream->draining = false;             /* Permit a new EOF-drain cycle after seeking. */
    return FVM_OK;                        /* The next read starts from the selected preceding keyframe. */
} /* Finish native seeking. */
#endif /* Finish MEDIA-only implementation. */
