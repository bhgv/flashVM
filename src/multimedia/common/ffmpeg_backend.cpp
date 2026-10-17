/* OS: Windows/Linux/macOS/Android. Construct the real FFmpeg decoder bridge. */
#include "ffmpeg.hpp" /* Import callbacks only when the static SDK is enabled. */
#include <string.h> /* Clear unavailable backend tables deterministically. */
/* Initialize one callback table; call on the host setup thread before concurrent opens. */
FVM_Result fvm_media_ffmpeg_backend(FVM_MediaBackend *out) /* Do not open files, devices or networks here. */
{ /* Begin backend construction. */
    if (!out) /* Require writable table storage. */
        return FVM_INVALID_ARGUMENT; /* Require writable table storage. */
    memset(out, 0, sizeof(*out)); /* Clear every field even in NO_MEDIA builds. */
#ifdef MEDIA /* Bind only real compiled FFmpeg callbacks. */
    avdevice_register_all(); /* Register device formats provided by this target's SDK. */
    out->struct_size = sizeof(*out); /* Identify the complete callback layout. */
    out->abi_version = FVM_MEDIA_ABI_VERSION; /* Identify the current bridge ABI. */
#if defined(_WIN32) /* Label the target rather than the build host. */
    out->os_name = "Windows/FFmpeg"; /* Device formats depend on the supplied Windows SDK. */
#elif defined(__ANDROID__) /* Android must precede the generic Linux test. */
    out->os_name = "Android/FFmpeg"; /* Camera permissions remain the host's responsibility. */
#elif defined(__APPLE__) /* Select the Apple target label. */
    out->os_name = "macOS/FFmpeg"; /* AVFoundation availability depends on the SDK configuration. */
#else /* Label the supported Linux target. */
    out->os_name = "Linux/FFmpeg"; /* The reproducible Linux SDK includes V4L2 capture. */
#endif /* Finish target labeling. */
    out->open = fvm_ffmpeg_open; /* Delegate demuxing and decoder setup to libavformat/libavcodec. */
    out->read = fvm_ffmpeg_read; /* Delegate synchronous decoding and conversion. */
    out->seek = fvm_ffmpeg_seek; /* Delegate stream repositioning. */
    out->close = fvm_ffmpeg_close; /* Delegate complete native-resource cleanup. */
    return FVM_OK; /* Report an available real backend. */
#else /* Keep the public bridge available without the optional SDK. */
    return FVM_UNSUPPORTED; /* Custom host backends still work in NO_MEDIA builds. */
#endif /* Finish optional FFmpeg selection. */
} /* Finish backend construction. */
