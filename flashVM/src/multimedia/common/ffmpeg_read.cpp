/* OS: FFmpeg-supported targets. Drive the send/receive API and drain delayed frames. */
#include "ffmpeg.hpp" /* Import native decoding and conversion contracts. */
#ifdef MEDIA          /* Exclude decoder calls when multimedia dependencies are disabled. */
#include <errno.h>    /* Interpret FFmpeg's EAGAIN result. */


/* Receive exactly one output frame; unrelated streams are skipped and EOF is drained. */
FVM_Result fvm_ffmpeg_read(void *opaque, FVM_MediaFrame *out)                  /* Native buffers remain owned by the stream. */
{                                                                              /* Begin synchronous decoding. */
    FVM_FFmpegStream *stream = static_cast<FVM_FFmpegStream *>(opaque);        /* Recover the selected decoder state. */
    av_frame_unref(stream->frame);                                             /* Invalidate the previous native frame view. */
    for (;;)                                                                   /* Feed packets until one decoded frame or a terminal result is available. */
    {                                                                          /* Begin one receive/feed cycle. */
        int result =                                                           /* Ask the real decoder for its next output. */
            avcodec_receive_frame(stream->codec, stream->frame);               /* Ask the real decoder for its next output. */
        if (result == 0)                                                       /* Convert a successfully decoded native frame. */
        {                                                                      /* Set timestamps before format-specific buffer conversion. */
            out->has_timestamp =                                               /* Preserve missing timestamps explicitly. */
                stream->frame->best_effort_timestamp != AV_NOPTS_VALUE;        /* Preserve missing timestamps explicitly. */
            if (out->has_timestamp)                                            /* Check this precondition before entering the following operation. */
            {                                                                  /* Begin conditional branch: Check this precondition before entering the following operation. */
                out->seconds =                                                 /* Convert the stream clock to seconds. */
                    stream->frame->best_effort_timestamp *                     /* Convert the stream clock to seconds. */
                    av_q2d(                                                    /* Convert the stream clock to seconds. */
                           stream->format->streams[stream->index]->time_base); /* Convert the stream clock to seconds. */
            } /* End conditional branch: Check this precondition before entering the following operation. */
            return stream->kind == FVM_MEDIA_AUDIO              /* Return the result produced by the following backend operation. */
                       ? fvm_ffmpeg_audio_convert(stream, out)  /* Produce the documented PCM or RGBA layout. */
                       : fvm_ffmpeg_video_convert(stream, out); /* Produce the documented PCM or RGBA layout. */
        } /* Finish successful frame conversion. */
        if (result == AVERROR_EOF) /* Report EOF only after delayed frames have drained. */
        {                          /* Begin conditional branch: Report EOF only after delayed frames have drained. */
            return FVM_NOT_FOUND;  /* Report EOF only after delayed frames have drained. */
        } /* End conditional branch: Report EOF only after delayed frames have drained. */
        if (result != AVERROR(EAGAIN)) /* Preserve hard decoder failures. */
        {                              /* Begin conditional branch: Preserve hard decoder failures. */
            return FVM_ERROR;          /* Preserve hard decoder failures. */
        } /* End conditional branch: Preserve hard decoder failures. */
        if (stream->input_eof)    /* Flush delayed codec output after demuxer EOF. */
        {                         /* Send the final null packet once. */
            if (stream->draining) /* A draining decoder must not request additional input. */
            {                     /* Begin conditional branch: A draining decoder must not request additional input. */
                return FVM_ERROR; /* A draining decoder must not request additional input. */
            } /* End conditional branch: A draining decoder must not request additional input. */
            result = avcodec_send_packet(stream->codec, 0); /* Start the real decoder's EOF drain. */
            if (result < 0 && result != AVERROR_EOF)        /* Reject an invalid drain transition. */
            {                                               /* Begin conditional branch: Reject an invalid drain transition. */
                return FVM_ERROR;                           /* Reject an invalid drain transition. */
            } /* End conditional branch: Reject an invalid drain transition. */
            stream->draining = true; /* Remember that no more compressed input may be sent. */
            continue;                /* Receive remaining delayed output. */
        } /* Finish decoder-drain setup. */
        do                                                          /* Skip packets belonging to the other audio/video streams. */
        {                                                           /* Read the next compressed input packet. */
            av_packet_unref(stream->packet);                        /* Release the previous packet before reuse. */
            result = av_read_frame(stream->format, stream->packet); /* Read from the actual file or capture device. */
        } while (result >= 0 &&                                     /* Preserve only the stream owned by this session. */
                 stream->packet->stream_index != stream->index); /* Preserve only the stream owned by this session. */
        if (result == AVERROR(EAGAIN)) /* Check this precondition before entering the following operation. */
        {                              /* Begin conditional branch: Check this precondition before entering the following operation. */
            return FVM_NOT_FOUND;      /* Let a nonblocking source be polled again without losing decoder
                                     state. */
        } /* End conditional branch: Check this precondition before entering the following operation. */
        if (result == AVERROR_EOF)    /* Switch to draining only for genuine end-of-input. */
        {                             /* Record terminal demuxer EOF. */
            stream->input_eof = true; /* Remember that subsequent reads must drain the codec. */
            continue;                 /* Enter the final-packet path on the next cycle. */
        } /* Finish demuxer EOF handling. */
        if (result < 0)       /* Reject I/O and malformed-input failures. */
        {                     /* Begin conditional branch: Reject I/O and malformed-input failures. */
            return FVM_ERROR; /* Reject I/O and malformed-input failures. */
        } /* End conditional branch: Reject I/O and malformed-input failures. */
        result = avcodec_send_packet(stream->codec,   /* Submit this stream's compressed packet to the decoder. */
                                     stream->packet); /* Submit this stream's compressed packet to the decoder. */
        av_packet_unref(stream->packet);              /* The decoder retains its own packet references as needed. */
        if (result < 0)                               /* Check this precondition before entering the following operation. */
        {                                             /* Begin conditional branch: Check this precondition before entering the following operation. */
            return FVM_ERROR;                         /* Receiving EAGAIN immediately before sending guarantees input can be
                                 accepted. */
        } /* End conditional branch: Check this precondition before entering the following operation. */
    } /* Finish the receive/feed loop. */
} /* Finish the native read callback. */
#endif /* Finish MEDIA-only implementation. */
