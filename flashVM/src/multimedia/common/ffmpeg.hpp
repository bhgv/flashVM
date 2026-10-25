/* OS: Windows/Linux/macOS/Android with a matching static FFmpeg 6.1 SDK. */
#ifndef FVM_MULTIMEDIA_FFMPEG_HPP     /* Protect private FFmpeg declarations. */
#define FVM_MULTIMEDIA_FFMPEG_HPP     /* Define the private include guard. */
#include "flashvm/multimedia.h"       /* Keep FFmpeg types out of the public C ABI. */
#ifdef MEDIA                          /* Compile library-specific details only for the MEDIA profile. */
#ifndef __STDC_CONSTANT_MACROS        /* FFmpeg integer constants must be visible in C++98. */
#define __STDC_CONSTANT_MACROS        /* Request standard fixed-width integer constant macros. */
#endif                                /* Finish the integer-constant compatibility condition. */
extern "C"                            /* Keep public symbols compatible with the C ABI. */
{                                     /* Import the libraries with C linkage. */
#include <libavcodec/avcodec.h>       /* Use real decoder contexts and frames. */
#include <libavdevice/avdevice.h>     /* Enumerate native capture formats. */
#include <libavformat/avformat.h>     /* Use real demuxer, packet and seek APIs. */
#include <libavutil/imgutils.h>       /* Validate and lay out RGBA images. */
#include <libavutil/mem.h>            /* Allocate library-compatible output storage. */
#include <libswresample/swresample.h> /* Convert planar/native audio into packed signed-16 PCM. */
#include <libswscale/swscale.h>       /* Convert decoder image formats into RGBA8. */

} /* Finish FFmpeg C linkage. */

struct FVM_FFmpegStream      /* Own all native resources for exactly one selected stream. */
{                            /* Begin the private FFmpeg state. */
    AVFormatContext *format; /* Own the opened file or device demuxer. */
    AVCodecContext *codec;   /* Own the selected stream decoder. */
    AVPacket *packet;        /* Reuse one demuxed compressed packet. */
    AVFrame *frame;          /* Reuse one decoded native frame. */
    SwsContext *scaler;      /* Own a cached image conversion context. */
    unsigned char *buffer;   /* Own the converted PCM or RGBA byte buffer. */
    unsigned int capacity;   /* Track av_fast_malloc capacity in bytes. */
    int index;               /* Identify the selected demuxer stream. */
    int kind;                /* Select AUDIO or VIDEO conversion. */
    bool input_eof;          /* Remember terminal demuxer EOF. */
    bool draining;           /* Remember that the decoder accepted its final null packet. */
}; /* Finish the private native state. */


/* Open a real demuxer and decoder with complete partial-failure cleanup. */
FVM_Result fvm_ffmpeg_open(void *, const FVM_MediaRequest *, void **); /* OS: FFmpeg-supported target. */


/* Receive a decoded native frame, feeding demuxed packets and draining EOF. */
FVM_Result fvm_ffmpeg_read(void *, FVM_MediaFrame *); /* OS: FFmpeg-supported target. */


/* Seek and flush pending decoder state. */
FVM_Result fvm_ffmpeg_seek(void *, double); /* OS: demuxer-dependent. */


/* Release every native allocation; also used for incomplete opens. */
void fvm_ffmpeg_close(void *); /* OS: FFmpeg-supported target. */


/* Convert one native audio frame into the documented borrowed PCM view. */
FVM_Result fvm_ffmpeg_audio_convert(FVM_FFmpegStream *, FVM_MediaFrame *); /* OS: libswresample. */


/* Convert one native image into the documented borrowed RGBA view. */
FVM_Result fvm_ffmpeg_video_convert(FVM_FFmpegStream *, FVM_MediaFrame *); /* OS: libswscale. */


/* Resolve only a registered video-input device format for explicit camera requests. */
const AVInputFormat *fvm_ffmpeg_camera_format(const char *); /* OS: platform-specific libavdevice formats. */
#endif                                                       /* Finish MEDIA-only declarations. */
#endif                                                       /* Finish the include guard. */
