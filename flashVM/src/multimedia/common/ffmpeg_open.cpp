/* OS: FFmpeg-supported targets. Open one selected audio/video stream or explicit camera. */
#include "ffmpeg.hpp" /* Keep all third-party types private. */
#ifdef MEDIA          /* Exclude FFmpeg symbols from NO_MEDIA archives. */
#include <new>        /* Allocate private native state without exceptions. */
#include <string.h>   /* Initialize native ownership fields. */


/* Open through actual FFmpeg library calls; every failure releases partial state. */
FVM_Result fvm_ffmpeg_open(void *userdata, const FVM_MediaRequest *request, /* Borrow the request only during this call. */
                           void **out)                                      /* Borrow the request only during this call. */
{                                                                           /* Begin native stream creation. */
    (void)userdata;                                                         /* The builtin backend has no shared host state. */
    *out = 0;                                                               /* Publish no native handle until initialization succeeds. */
    const AVInputFormat *input = 0;                                         /* Let file requests use normal demuxer probing. */
    if (request->kind == FVM_MEDIA_CAMERA)                                  /* Camera access requires an explicit registered device format. */
    {                                                                       /* Resolve the format before opening the named device. */
        input = fvm_ffmpeg_camera_format(request->format);                  /* Reject file demuxers masquerading as camera formats. */
        if (!input)                                                         /* Report an unavailable target device backend. */
        {                                                                   /* Begin conditional branch: Report an unavailable target device backend. */
            return FVM_UNSUPPORTED;                                         /* Report an unavailable target device backend. */
        } /* End conditional branch: Report an unavailable target device backend. */
    } /* Finish explicit camera selection. */
    FVM_FFmpegStream *stream = new (std::nothrow) FVM_FFmpegStream; /* Own one decoder independently of the VM heap. */
    if (!stream)                                                    /* Avoid library calls after allocation failure. */
    {                                                               /* Begin conditional branch: Avoid library calls after allocation failure. */
        return FVM_OUT_OF_MEMORY;                                   /* Avoid library calls after allocation failure. */
    } /* End conditional branch: Avoid library calls after allocation failure. */
    memset(stream, 0, sizeof(*stream));                                     /* Make partial cleanup safe for every native pointer. */
    stream->kind = request->kind == FVM_MEDIA_AUDIO ? FVM_MEDIA_AUDIO       /* Camera frames share video conversion. */
                                                    : FVM_MEDIA_VIDEO;      /* Camera frames share video conversion. */
    if (avformat_open_input(&stream->format, request->url, input, 0) < 0 || /* Open and inspect the real input source. */
        avformat_find_stream_info(stream->format, 0) < 0)                   /* Open and inspect the real input source. */
    {                                                                       /* Clean failed demuxer initialization. */
        fvm_ffmpeg_close(stream);                                           /* Release all native resources allocated so far. */
        return FVM_ERROR;                                                   /* Preserve a simple public failure category; FFmpeg may also log
                                     diagnostics. */
    } /* Finish demuxer failure cleanup. */
    const AVCodec *decoder = 0;                                                                                        /* Receive the actual decoder selected for this stream. */
    stream->index =                                                                                                    /* Skip unrelated streams rather than mixing ownership. */
        av_find_best_stream(stream->format, stream->kind == FVM_MEDIA_AUDIO ? AVMEDIA_TYPE_AUDIO : AVMEDIA_TYPE_VIDEO, /* Skip unrelated streams rather than mixing ownership. */
                            -1, -1, &decoder, 0);                                                                      /* Skip unrelated streams rather than mixing ownership. */
    if (stream->index < 0 || !decoder)                                                                                 /* Require a supported stream and decoder. */
    {                                                                                                                  /* Release an input with no decodable stream of the requested kind. */
        fvm_ffmpeg_close(stream);                                                                                      /* Close the demuxer and wrapper. */
        return FVM_UNSUPPORTED;                                                                                        /* Distinguish missing codec/stream support from valid decoding. */
    } /* Finish stream-selection failure cleanup. */
    stream->codec = avcodec_alloc_context3(decoder);         /* Allocate the native codec context. */
    stream->packet = av_packet_alloc();                      /* Allocate a reusable compressed-packet wrapper. */
    stream->frame = av_frame_alloc();                        /* Allocate a reusable decoded-frame wrapper. */
    if (!stream->codec || !stream->packet || !stream->frame) /* Check every native allocation. */
    {                                                        /* Release a partial native allocation set. */
        fvm_ffmpeg_close(stream);                            /* Free successfully allocated siblings too. */
        return FVM_OUT_OF_MEMORY;                            /* Report allocation failure accurately. */
    } /* Finish allocation failure cleanup. */
    if (avcodec_parameters_to_context(stream->codec, stream->format->streams[stream->index]->codecpar) < 0 || /* Configure and start the actual decoder. */
        avcodec_open2(stream->codec, decoder, 0) < 0)                                                         /* Configure and start the actual decoder. */
    {                                                                                                         /* Clean codec initialization failure. */
        fvm_ffmpeg_close(stream);                                                                             /* Release decoder, input and both wrappers. */
        return FVM_ERROR;                                                                                     /* Do not expose an unusable stream. */
    } /* Finish codec failure cleanup. */
    *out = stream; /* Transfer native stream ownership to the common session wrapper. */
    return FVM_OK; /* Report a ready decoder or capture source. */
} /* Finish the native open callback. */
#endif /* Finish MEDIA-only implementation. */
