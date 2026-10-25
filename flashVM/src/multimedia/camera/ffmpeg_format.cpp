/* OS: Linux V4L2; Windows DirectShow; macOS AVFoundation; Android camera when enabled in the SDK.
 */
#include "../common/ffmpeg.hpp" /* Import registered FFmpeg device-format declarations. */
#ifdef MEDIA                    /* Keep device-library references out of NO_MEDIA builds. */
/* Resolve aliases through FFmpeg, then require membership in its video-device list. */
const AVInputFormat *                      /* Never probe or open a device while resolving formats. */
fvm_ffmpeg_camera_format(const char *name) /* Never probe or open a device while resolving formats. */
{                                          /* Begin device-format validation. */
    if (!name || !*name)                   /* Require an explicit host-supplied format name. */
    {                                      /* Begin conditional branch: Require an explicit host-supplied format name. */
        return 0;                          /* Require an explicit host-supplied format name. */
    } /* End conditional branch: Require an explicit host-supplied format name. */
    const AVInputFormat *wanted =   /* Resolve official names and aliases such as video4linux2/v4l2. */
        av_find_input_format(name); /* Resolve official names and aliases such as video4linux2/v4l2. */
    if (!wanted)                    /* Reject unavailable formats before iteration. */
    {                               /* Begin conditional branch: Reject unavailable formats before iteration. */
        return 0;                   /* Reject unavailable formats before iteration. */
    } /* End conditional branch: Reject unavailable formats before iteration. */
    const AVInputFormat *candidate = 0;                              /* Start registered video-input device enumeration. */
    while ((candidate = av_input_video_device_next(candidate)) != 0) /* Ask the actual target SDK for device formats. */
    {                                                                /* Check format identity rather than guessing from the requested name. */
        if (candidate == wanted)                                     /* Accept only a compiled native video-input format. */
        {                                                            /* Begin conditional branch: Accept only a compiled native video-input format. */
            return candidate;                                        /* Accept only a compiled native video-input format. */
        } /* End conditional branch: Accept only a compiled native video-input format. */
    } /* Finish device enumeration. */
    return 0; /* Reject regular file demuxers and unsupported devices. */
} /* Finish device-format resolution. */
#endif /* Finish MEDIA-only implementation. */
