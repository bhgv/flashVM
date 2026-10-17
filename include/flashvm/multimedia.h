/* Multimedia bridge ABI: host-owned sessions, independent of VM managed objects. */
#ifndef FLASHVM_MULTIMEDIA_H /* Protect the public C89 declarations. */
#define FLASHVM_MULTIMEDIA_H /* Declare the include guard. */
#include "flashvm.h" /* Reuse result codes, export attributes and size_t. */
#ifdef __cplusplus /* Keep the same symbols for C and C++ callers. */
extern "C" /* Keep public symbols compatible with the C ABI. */
{ /* Begin the public C ABI. */
#endif /* End the C++ linkage condition. */
#define FVM_MEDIA_ABI_VERSION 1UL /* Exact callback-table version accepted by this release. */
#define FVM_MEDIA_RGBA_BYTES_PER_PIXEL 4 /* Stored byte width of one RGBA8 pixel. */
#define FVM_DEVICE_WINDOWS_VK_MAX /* Continue the same operation using the explicitly listed arguments. */ \
    255U /* Highest native Windows virtual-key value accepted here.  \
                                        */
#define FVM_DEVICE_MICROSECONDS_PER_SECOND /* Continue the same operation using the explicitly listed arguments. */ \
    1000000.0 /* Native evdev timestamp conversion factor.  \
                                                      */
#define FVM_DEVICE_NANOSECONDS_PER_SECOND 1000000000.0 /* Native Android timestamp conversion factor. */
#define FVM_MEDIA_AUDIO_MIX_MAX_INPUTS 1024U /* Keep summed S16 values inside a portable signed-long accumulator. */
#define FVM_MEDIA_AUDIO 1 /* Decode an audio stream as native-endian signed 16-bit PCM. */
#define FVM_MEDIA_VIDEO 2 /* Decode a video stream as tightly packed RGBA8. */
#define FVM_MEDIA_CAMERA 3 /* Explicitly open a camera with a named FFmpeg device format. */
#define FVM_DEVICE_KEY 1 /* A native keyboard key or button transition. */
#define FVM_DEVICE_MOUSE 2 /* A pointer axis update; native_code identifies the axis. */
#define FVM_DEVICE_TOUCH 3 /* A touch axis, slot or contact update in native units. */
#define FVM_DEVICE_ACCELEROMETER 4 /* Complete XYZ acceleration sample in metres per second squared. */
#define FVM_DEVICE_COMPASS 5 /* Complete XYZ magnetic field sample in microtesla. */
#define FVM_DEVICE_GYROSCOPE 6 /* Complete XYZ angular velocity sample in radians per second. */
#define FVM_DEVICE_ORIENTATION 7 /* Host-defined orientation sample; convention must be documented. */
    typedef struct FVM_MediaSession FVM_MediaSession; /* Own a copied backend table and its private stream. */
    typedef struct FVM_MediaRequest /* Borrow these strings only during the synchronous open callback. */
    { /* Borrow these strings only during the synchronous open callback. */
        unsigned long struct_size; /* Set to sizeof(FVM_MediaRequest). */
        int kind; /* Select AUDIO, VIDEO or CAMERA. */
        const char *url; /* Supply a filename or an explicit device identifier; never opened implicitly. */
        const char *format; /* Optional demuxer name; mandatory device-format name for CAMERA. */
    } FVM_MediaRequest; /* Finish the immutable open request. */
    typedef struct FVM_MediaFrame /* Borrow the buffer until the next read, seek or close on this session. */
    { /* Borrow the buffer until the next read, seek or close on this session. */
        int kind; /* AUDIO or VIDEO; camera frames use VIDEO. */
        const unsigned char *data; /* Non-null contiguous decoded data on success. */
        size_t size; /* Number of accessible buffer bytes. */
        unsigned int width; /* Video width; zero for audio. */
        unsigned int height; /* Video height; zero for audio. */
        size_t stride; /* RGBA byte distance between video rows; zero for audio. */
        unsigned int sample_rate; /* Audio samples per second; zero for video. */
        unsigned int channels; /* Number of interleaved audio channels; zero for video. */
        size_t samples; /* Number of audio sample frames, not channel values; zero for video. */
        double seconds; /* Presentation timestamp in stream time base converted to seconds. */
        int has_timestamp; /* Nonzero only when seconds is meaningful. */
    } FVM_MediaFrame; /* Finish the decoded buffer view. */
    typedef FVM_Result (*FVM_MediaOpen)(void *, const FVM_MediaRequest *, /* Create stream state; failure must clean its allocations. */
                                        void **); /* Create stream state; failure must clean its allocations. */
    typedef FVM_Result (*FVM_MediaRead)(void *, FVM_MediaFrame *); /* Return OK, EOF as NOT_FOUND, or an error. */
    typedef FVM_Result (*FVM_MediaSeek)(void *, double); /* Seek in seconds; optional, null means UNSUPPORTED. */
    typedef void (*FVM_MediaClose)(void *); /* Release one successfully opened stream exactly once. */
    typedef struct FVM_MediaBackend /* Supply real library or OS callbacks; this table is copied on open. */
    { /* Supply real library or OS callbacks; this table is copied on open. */
        unsigned long struct_size; /* Set to sizeof(FVM_MediaBackend). */
        unsigned long abi_version; /* Set to FVM_MEDIA_ABI_VERSION. */
        const char *os_name; /* Borrow a static platform/backend label for diagnostics. */
        void *userdata; /* Host-owned open-callback argument; must outlive every session. */
        FVM_MediaOpen open; /* Required synchronous stream factory. */
        FVM_MediaRead read; /* Required synchronous decoder. */
        FVM_MediaSeek seek; /* Optional repositioning implementation. */
        FVM_MediaClose close; /* Required stream finalizer. */
    } FVM_MediaBackend; /* Finish the replaceable decoding/capture bridge. */
    typedef struct FVM_MediaOutput /* Delegate playback/presentation to the host OS or library. */
    { /* Delegate playback/presentation to the host OS or library. */
        unsigned long struct_size; /* Set to sizeof(FVM_MediaOutput). */
        void *userdata; /* Host-owned output state; no ownership transfer. */
        FVM_Result (*write)(void *, const FVM_MediaFrame *); /* Consume or copy the borrowed frame synchronously. */
        FVM_Result (*stop)(void *); /* Stop and discard pending output; optional until stop is requested. */
    } FVM_MediaOutput; /* Finish the output bridge shared by audio and video. */
    typedef struct FVM_AlsaOutput FVM_AlsaOutput; /* Host-owned Linux PCM device; close only after unregistering its channel. */
    /* Explicitly open/configure nonblocking interleaved S16 playback; disabled builds return UNSUPPORTED. */
    FVM_API FVM_Result fvm_audio_linux_alsa_open(const char *, unsigned int, unsigned int, unsigned int, FVM_AlsaOutput **, FVM_MediaOutput *); /* OS: Linux/static ALSA. */
    /* Drop pending PCM and close the device; always invalidates a non-null live handle, even on I/O error. */
    FVM_API FVM_Result fvm_audio_linux_alsa_close(FVM_AlsaOutput *); /* OS: Linux/static ALSA; null succeeds. */
    typedef struct FVM_AudioChannel FVM_AudioChannel; /* Context-owned host output registration; invalid after context destruction. */
    /* Copy a host output table with required write/stop callbacks; publish an inactive channel. */
    FVM_API FVM_Result fvm_audio_channel_create(FVM_Context *, const FVM_MediaOutput *, FVM_AudioChannel **); /* OS: host callback. */
    /* Validate and synchronously submit PCM; a successful write marks the channel active. */
    FVM_API FVM_Result fvm_audio_channel_write(FVM_AudioChannel *, const FVM_MediaFrame *); /* OS: host callback. */
    /* Stop an active channel; success marks inactive, failure retains it for retry. */
    FVM_API FVM_Result fvm_audio_channel_stop(FVM_AudioChannel *); /* OS: host callback. */
    /* Stop then unregister; failure keeps the handle valid, null is accepted. */
    FVM_API FVM_Result fvm_audio_channel_close(FVM_AudioChannel *); /* OS: host callback. */
    /* Return a copied active flag, without entering host callbacks. */
    FVM_API FVM_Result fvm_audio_channel_active(FVM_AudioChannel *, int *); /* OS: portable state query. */
    /* Stop every active registered output in this context, attempting all after failures. */
    FVM_API FVM_Result fvm_audio_stop_all(FVM_Context *); /* OS: host callback; AVM1 StopSounds uses this operation. */
    typedef struct FVM_DeviceEvent /* Carry one host/device event independently of ActionScript dispatch. */
    { /* Carry one host/device event independently of ActionScript dispatch. */
        int kind; /* One FVM_DEVICE_* category. */
        unsigned int native_code; /* Native key/axis code; host translates it before script dispatch. */
        double x; /* First axis or key value; units depend on kind. */
        double y; /* Second axis for complete host sensor samples; zero for evdev fragments. */
        double z; /* Third axis for complete host sensor samples; zero for evdev fragments. */
        double seconds; /* Native device timestamp; not automatically converted to the VM clock. */
    } FVM_DeviceEvent; /* Finish the extensible device event. */
    typedef struct FVM_DeviceBridge /* Poll the host event queue without creating a hidden device thread. */
    { /* Poll the host event queue without creating a hidden device thread. */
        unsigned long struct_size; /* Set to sizeof(FVM_DeviceBridge). */
        void *userdata; /* Borrow the host queue or device state. */
        FVM_Result (*poll)(void *, FVM_DeviceEvent *); /* Return NOT_FOUND when no event is available. */
    } FVM_DeviceBridge; /* Finish the replaceable device polling table. */
    /* Initialize a complete backend table; NO_MEDIA returns UNSUPPORTED and clears the output. */
    FVM_API FVM_Result fvm_media_ffmpeg_backend( /* OS: Windows/Linux/macOS/Android with a matching static FFmpeg SDK. */
        FVM_MediaBackend *backend); /* OS: Windows/Linux/macOS/Android with a matching static FFmpeg SDK. */
    /* Open an audio file using the selected backend; clear out_session before work. */
    FVM_API FVM_Result fvm_audio_open(const FVM_MediaBackend *, const char *, /* OS: backend-selected. */
                                      FVM_MediaSession **); /* OS: backend-selected. */
    /* Open a video file using the selected backend; audio in the same file is skipped. */
    FVM_API FVM_Result fvm_video_open(const FVM_MediaBackend *, const char *, /* OS: backend-selected. */
                                      FVM_MediaSession **); /* OS: backend-selected. */
    /* Open a camera only when the host supplies its explicit format and device identifier. */
    FVM_API FVM_Result fvm_camera_open(const FVM_MediaBackend *, const char *, const char *, /* OS: backend-selected device format. */
                                       FVM_MediaSession **); /* OS: backend-selected device format. */
    /* Decode one buffer; clear the output first and validate the callback's successful frame. */
    FVM_API FVM_Result fvm_media_read(FVM_MediaSession *, FVM_MediaFrame *); /* OS: backend-selected. */
    /* Reposition to the first decodable packet at/before seconds; actual frame timestamps may
     * precede it. */
    FVM_API FVM_Result fvm_media_seek(FVM_MediaSession *, double); /* OS: backend-selected. */
    /* Close a session; null is accepted and buffers become invalid. */
    FVM_API void fvm_media_close(FVM_MediaSession *); /* OS: backend-selected. */
    /* Send a valid audio buffer to a real host output callback; no buffering or conversion is
     * implicit. */
    FVM_API FVM_Result fvm_audio_write(const FVM_MediaOutput *, const FVM_MediaFrame *); /* OS: host callback. */
    /* Sum equal-geometry native S16 inputs once, pad short tails with silence and saturate after the full sum. */
    FVM_API FVM_Result fvm_audio_mix_s16(const FVM_MediaFrame *inputs, size_t input_count, /* OS: portable native PCM arithmetic; inputs are borrowed. */
                                          unsigned int sample_rate, unsigned int channels, size_t frames, /* Select the complete destination block geometry. */
                                          short *destination, size_t capacity_values, FVM_MediaFrame *out_frame); /* Borrow caller storage in the returned output view. */
    /* Stop only this standalone output; context channels use fvm_audio_stop_all. */
    FVM_API FVM_Result fvm_audio_stop(const FVM_MediaOutput *); /* OS: host callback. */
    /* Present a valid RGBA frame through the host's output callback. */
    FVM_API FVM_Result fvm_video_present(const FVM_MediaOutput *, const FVM_MediaFrame *); /* OS: host callback. */
    /* Poll one device event; the host remains responsible for permissions, normalization and VM
     * dispatch. */
    FVM_API FVM_Result fvm_device_poll(const FVM_DeviceBridge *, FVM_DeviceEvent *); /* OS: host callback. */
    /* Read one native evdev record from a host-opened nonblocking fd; other targets return
     * UNSUPPORTED. */
    FVM_API FVM_Result fvm_input_linux_evdev_read(int, /* OS: Linux; no device is opened or closed. */
                                                  FVM_DeviceEvent *); /* OS: Linux; no device is opened or closed. */
    /* Poll global screen coordinates through GetCursorPos; other targets return UNSUPPORTED. */
    FVM_API FVM_Result /* OS: Windows; x/y are desktop pixels, no timestamp. */
    fvm_input_windows_pointer(FVM_DeviceEvent *); /* OS: Windows; x/y are desktop pixels, no timestamp. */
    /* Poll a virtual key through GetAsyncKeyState; key-up and inaccessible desktop both yield zero.
     */
    FVM_API FVM_Result fvm_input_windows_key(unsigned int, /* OS: Windows; x is current physical down state. */
                                             FVM_DeviceEvent *); /* OS: Windows; x is current physical down state. */
    /* Poll global display coordinates through Core Graphics; other targets return UNSUPPORTED. */
    FVM_API FVM_Result fvm_input_macos_pointer(FVM_DeviceEvent *); /* OS: macOS; x/y are global display coordinates. */
    /* Borrow one AInputEvent and decode one pointer; the host retains queue/event ownership. */
    FVM_API FVM_Result fvm_input_android_motion( /* OS: Android; local x/y, z pointer ID, native_code full action. */
        const void *, size_t, FVM_DeviceEvent *); /* OS: Android; local x/y, z pointer ID, native_code full action. */
#ifdef __cplusplus /* Close C linkage for C++ callers. */
} /* End the public C ABI. */
#endif /* Finish the linkage condition. */
#endif /* Finish the header guard. */
